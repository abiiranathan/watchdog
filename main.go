package main

import (
	"golive/watchdog"
	"log"
	"os"

	"github.com/abiiranathan/goflag"
)

var (
	// Patterns to watch for changes. Can be a file or directory
	// Preferably a directory to watch for changes in all files.
	// This is because avoids too many watches being created.
	// The exclusion patterns are applied to the expanded patterns(using glob)
	patterns []string

	// Patterns to exclude from watching. Can be a file or directory or a glob pattern
	exclude []string

	// Command to execute when a change is detected.
	command string

	// Enable verbose logging
	verbose bool = false

	// Watch current directory
	watchCWD bool = false
)

func main() {
	ctx := goflag.NewContext()

	ctx.AddFlag(goflag.FlagStringSlice, "patterns", "p", &patterns, "Patterns to watch for changes", true)
	ctx.AddFlag(goflag.FlagString, "command", "c", &command, "Command to execute when a change is detected", true)
	ctx.AddFlag(goflag.FlagStringSlice, "exclude", "e", &exclude, "Exclude patterns from watching", false)
	ctx.AddFlag(goflag.FlagBool, "verbose", "v", &verbose, "Enable debug mode", false)
	ctx.AddFlag(goflag.FlagBool, "watchcur", "w", &watchCWD, "Watch current directory", false)

	_, err := ctx.Parse(os.Args)
	if err != nil {
		ctx.PrintUsage(os.Stderr)
		log.Fatalf("Error parsing flags: %v\n", err)
		os.Exit(1)
	}

	if command == "" {
		ctx.PrintUsage(os.Stderr)
		log.Fatalf("Command cannot be empty")
	}

	wd := watchdog.NewWatchDog()
	defer wd.Close()

	// Expand the patterns and exclude patterns
	exclude = append(exclude, watchdog.DefaultExcludes...)
	expandedPatterns := watchdog.GlobExpand(patterns, watchCWD)
	expandedExclude := watchdog.GlobExpand(exclude, watchCWD)

	if verbose {
		log.Printf("Main Process ID: %v\n", os.Getpid())
		log.Printf("Expanded patterns: %v\n", expandedPatterns)
		log.Printf("Expanded exclude patterns: %v\n", expandedExclude)
	}

	wd.HandleEvents(command, expandedPatterns, expandedExclude)
}
