#include "../include/inotify_utils.h"
#include "../include/process_utils.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_EVENTS 1024
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + 16))

#define RELOAD_TIMEOUT 1  // 2 seconds

// Track last time the project was reloaded
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t last_reload_time = 0;

// Global debug flag
int verbose = 0;

void enable_verbose_logging() {
  verbose = 1;
}

void disable_verbose_logging() {
  verbose = 0;
}

static void safe_update_time(time_t* t) {
  // mutex
  pthread_mutex_lock(&mutex);
  *t = time(NULL);
  pthread_mutex_unlock(&mutex);
}

int watchdog_init() {
  int inotify_fd = inotify_init();
  if (inotify_fd == -1) {
    perror("inotify_init");
    exit(EXIT_FAILURE);
  }
  return inotify_fd;
}

void watchdog_cleanup(int inotify_fd) {
  close(inotify_fd);
}

static int add_watch(int inotify_fd, const char* path, uint32_t mask) {
  int watch_fd = inotify_add_watch(inotify_fd, path, mask);
  if (watch_fd == -1) {
    perror("inotify_add_watch");
    debug_log("Failed to add watch for: %s\n", path);
    exit(EXIT_FAILURE);
  }
  return watch_fd;
}

static int is_excluded(const char* path, int num_excluded, char* exclude_patterns[]) {
  if (exclude_patterns == NULL || strlen(path) == 0) {
    return 0;
  }

  for (int i = 0; i < num_excluded; ++i) {
    if (strcmp(path, exclude_patterns[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static void watch_events(int inotify_fd, int num_patterns, char* patterns[], int num_excluded,
                         char* exclude_patterns[], struct WatchList list[], int* list_size) {
  for (int i = 0; i < num_patterns; ++i) {
    const char* p = patterns[i];

    // Check if the pattern is excluded
    if (is_excluded(p, num_excluded, exclude_patterns)) {
      continue;
    }

    // Add watch for the pattern
    int watch_fd = add_watch(inotify_fd, p, IN_MODIFY | IN_CREATE | IN_DELETE);
    list[i] = (struct WatchList){.watch_fd = watch_fd, .index = i, .name = patterns[i]};
    ++(*list_size);
  }
}

void get_filename(struct WatchList* list, int list_size, int watch_fd, char* name, int name_size) {
  for (int i = 0; i < list_size; i++) {
    if (list[i].watch_fd == watch_fd) {
      strncpy(name, list[i].name, name_size - 1);
      name[name_size - 1] = '\0';
      return;
    }
  }
  name[0] = '\0';
}

static int is_directory(const char* path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISDIR(path_stat.st_mode);
}

void run_in_background(void) {
  pthread_t thread;
  pthread_create(&thread, NULL, (void*)reload_process, NULL);
  pthread_detach(thread);
  safe_update_time(&last_reload_time);
}

// Handle events
void handle_events(EventArgs* args) {
  int list_size;
  struct WatchList* list = malloc(sizeof(struct WatchList) * args->num_patterns);
  if (list == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  watch_events(args->inotify_fd, args->num_patterns, args->patterns, args->num_excluded,
               args->exclude_patterns, list, &list_size);

  // initial load.
  run_in_background();

  char buf[BUF_LEN];
  ssize_t bytes_read;
  uint32_t mask =
    IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO;

  debug_log("Watching %d files or directories\n", list_size);
  while (1) {
    bytes_read = read(args->inotify_fd, buf, BUF_LEN);
    if (bytes_read == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }


    // see man inotify
    for (char* p = buf; p < buf + bytes_read;) {
      struct inotify_event* event = (struct inotify_event*)p;
      if (event->mask & mask) {
        char name[PATH_MAX] = "";
        char complete_path[PATH_MAX] = "";
        int is_dir = 0;

        get_filename(list, list_size, event->wd, name, sizeof(name));

        if ((is_dir = is_directory(name))) {
          // construct complete path as event->name is only the name of the file
          strcat(complete_path, name);
          strcat(complete_path, "/");
          strcat(complete_path, event->name);

          if (is_excluded(complete_path, args->num_excluded, args->exclude_patterns)) {
            p += EVENT_SIZE + event->len;
            inotify_rm_watch(args->inotify_fd, event->wd);
            debug_log("[Skipping]: Removed watch for %s\n", complete_path);
            continue;
          }
        }

        if (is_excluded(name, args->num_excluded, args->exclude_patterns)) {
          p += EVENT_SIZE + event->len;
          inotify_rm_watch(args->inotify_fd, event->wd);
          debug_log("[Skipping]: Removed watch for %s\n", name);
          continue;
        }

        time_t now = time(NULL);
        if (now - last_reload_time > RELOAD_TIMEOUT) {
          debug_log("%s has changed\n", is_dir ? complete_path : name);
          run_in_background();
        }
      }
      p += EVENT_SIZE + event->len;
    }
  }

  free(list);
}
