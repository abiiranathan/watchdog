#include "../include/process_utils.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int prev_child_pid = -1;

static void safe_update_pid(pid_t child_pid) {
  pthread_mutex_lock(&mutex);
  prev_child_pid = child_pid;
  pthread_mutex_unlock(&mutex);
}

void kill_previous_process(int prev_child_pid) {
  // Kill previous child process
  if (prev_child_pid != -1) {
    if ((kill(prev_child_pid, SIGKILL)) != -1) {
      printf("Killed previous child process with pid: %d\n", prev_child_pid);
      safe_update_pid(-1);
    }
  }
}

void reload_process(void) {
  printf("Reloading process...\n");
  kill_previous_process(prev_child_pid);

  pid_t child_pid = fork();
  if (child_pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  // Child process
  if (child_pid == 0) {
    // Execute the file. This will replace the current child process
    execl("/bin/sh", "sh", "-c", command, (char*)NULL);
    perror("execl");
    exit(EXIT_FAILURE);
  } else {
    // Parent process. Update the previous child pid so that
    // we can kill it later on reload.
    safe_update_pid(child_pid);

    // Wait for child to complete
    int status;
    waitpid(child_pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      printf("Process exited with status: %d\n", WEXITSTATUS(status));
    }
  }
}

void register_command(int cmd_len, char* cmd) {
  if (cmd_len >= sizeof(command)) {
    printf("Command is too long. It must be less than %ld characters.\n", sizeof(command));
    exit(EXIT_FAILURE);
  }
  strncpy(command, cmd, cmd_len);
  command[cmd_len] = '\0';
}
