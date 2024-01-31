package watchdog

/*
#cgo CFLAGS: -I${SRCDIR}/../include
#cgo LDFLAGS: -L${SRCDIR}/../lib -l:inotify_utils.a -l:process_utils.a
#include <stdlib.h>
#include "inotify_utils.h"
#include "process_utils.h"
*/
import "C"
import (
	"log"
	"unsafe"
)

type WatchDog struct {
	InotifyFd int
}

func NewWatchDog() *WatchDog {
	notifyFd := C.watchdog_init()
	return &WatchDog{InotifyFd: int(notifyFd)}
}

func (w *WatchDog) Close() {
	C.watchdog_cleanup(C.int(w.InotifyFd))
}

func (w *WatchDog) HandleEvents(command string, patterns, excludePatterns []string) {
	// Convert the patterns to C strings
	num_patterns := C.int(len(patterns))
	c_patterns := make([]*C.char, len(patterns))
	for i, arg := range patterns {
		c_patterns[i] = C.CString(arg)
	}

	defer func() {
		for _, arg := range c_patterns {
			C.free(unsafe.Pointer(arg))
		}
	}()

	// Convert the exclude patterns to C strings
	num_exclude := C.int(len(excludePatterns))
	c_exclude := make([]*C.char, len(excludePatterns))
	for i, arg := range excludePatterns {
		c_exclude[i] = C.CString(arg)
	}

	defer func() {
		for _, arg := range c_exclude {
			C.free(unsafe.Pointer(arg))
		}
	}()

	// Register command
	c_cmd := C.CString(command)
	defer C.free(unsafe.Pointer(c_cmd))
	C.register_command(C.int(len(command)), c_cmd)

	// Cater for the case where there are no exclude patterns
	// otherwise, we get a segfault doing &c_exclude[0]
	var c_exclude_ptr **C.char
	if len(excludePatterns) > 0 {
		c_exclude_ptr = &c_exclude[0]
	}

	var c_patterns_ptr **C.char = &c_patterns[0]

	args := C.calloc(1, C.sizeof_struct_EventArgs)
	// check for nil
	if args == nil {
		log.Fatalf("Error: Unable to allocate memory for args\n")
	}
	defer C.free(args)

	// Set the values of the struct
	(*C.EventArgs)(args).inotify_fd = C.int(w.InotifyFd)
	(*C.EventArgs)(args).num_patterns = num_patterns
	(*C.EventArgs)(args).patterns = c_patterns_ptr
	(*C.EventArgs)(args).num_excluded = num_exclude
	(*C.EventArgs)(args).exclude_patterns = c_exclude_ptr

	C.handle_events((*C.EventArgs)(args))
}

func VerboseMode(flag bool) {
	if flag {
		C.enable_verbose_logging()
	} else {
		C.disable_verbose_logging()
	}
}
