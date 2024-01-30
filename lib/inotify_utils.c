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
    fprintf(stderr, "Failed to add watch for: %s\n", path);
    exit(EXIT_FAILURE);
  }
  return watch_fd;
}

__attribute__((always_inline)) static inline int is_excluded(const char* path, int num_excluded,
                                                             char* exclude_patterns[]) {
  if (exclude_patterns == NULL) {
    return 0;
  }

  for (int i = 0; i < num_excluded; ++i) {
    if (strcmp(path, exclude_patterns[i]) == 0) {
      printf("Excluding pattern: %s\n", path);
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
      printf("skipping excluded pattern: %s\n", p);
      continue;
    }

    // Add watch for the pattern
    int watch_fd = add_watch(inotify_fd, p, IN_MODIFY | IN_CREATE | IN_DELETE);
    list[i] = (struct WatchList){.watch_fd = watch_fd, .index = i, .name = patterns[i]};
    ++(*list_size);
  }
}

void get_filename(struct WatchList* list, int list_size, int watch_fd, char* name) {
  for (int i = 0; i < list_size; i++) {
    if (list[i].watch_fd == watch_fd) {
      strncpy(name, list[i].name, PATH_MAX - 1);
      printf("Matching pattern: %s\n", name);
      return;
    }
  }
  name = NULL;
}

void run_in_background(void) {
  pthread_t thread;
  pthread_create(&thread, NULL, (void*)reload_process, NULL);
  pthread_detach(thread);
  safe_update_time(&last_reload_time);
}

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
        char name[PATH_MAX] = {0};
        get_filename(list, list_size, event->wd, name);
        time_t now = time(NULL);
        if (now - last_reload_time > RELOAD_TIMEOUT) {
          if ((char*)name != NULL) {
            fprintf(stdout, "Detected changes in %s", name);
          }
          run_in_background();
        }
      }
      p += EVENT_SIZE + event->len;
    }
  }

  free(list);
}
