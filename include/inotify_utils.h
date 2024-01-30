#ifndef E1F9938A_DC83_43ED_BE05_3E2D1528784C
#define E1F9938A_DC83_43ED_BE05_3E2D1528784C

#include <limits.h>
#include <sys/inotify.h>

typedef struct EventArgs {
  int inotify_fd;
  int num_patterns;
  char** patterns;
  int num_excluded;
  char** exclude_patterns;
} EventArgs;

struct WatchList {
  int watch_fd;  // watch file descriptor
  int index;     // Index in include patterns
  char* name;
};

// Initialize inotify
int watchdog_init();
void watchdog_cleanup(int inotify_fd);
void handle_events(EventArgs* args);

#endif /* E1F9938A_DC83_43ED_BE05_3E2D1528784C */
