package main

import (
	"log"
	"os"

	"github.com/abiiranathan/goflag"
	"github.com/abiiranathan/watchdog/watchdog"
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

	// Watch directories recursively
	recursive bool = true
)

func main() {
	ctx := goflag.NewContext()

	ctx.AddFlag(goflag.FlagStringSlice, "patterns", "p", &patterns, "Patterns to watch for changes", true)
	ctx.AddFlag(goflag.FlagString, "command", "c", &command, "Command to execute when a change is detected", true)
	ctx.AddFlag(goflag.FlagStringSlice, "exclude", "e", &exclude, "Exclude patterns from watching", false)
	ctx.AddFlag(goflag.FlagBool, "verbose", "v", &verbose, "Enable debug mode", false)
	ctx.AddFlag(goflag.FlagBool, "watchcur", "w", &watchCWD, "Watch current directory", false)
	ctx.AddFlag(goflag.FlagBool, "recursive", "r", &recursive, "Watch directories recursively", false)

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
	expandedPatterns := watchdog.GlobExpand(patterns, watchCWD, recursive)
	expandedExclude := watchdog.GlobExpand(exclude, watchCWD, recursive)

	// Filter the expanded patterns and exclude patterns
	expandedPatterns = watchdog.Filter(expandedPatterns, expandedExclude)
	expandedExclude = watchdog.Unique(expandedExclude)

	if verbose {
		log.Printf("Main Process ID: %v\n", os.Getpid())
		watchdog.VerboseMode(verbose)
	}
	wd.HandleEvents(command, expandedPatterns, expandedExclude)
}
