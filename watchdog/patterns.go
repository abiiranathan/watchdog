package watchdog

import (
	"log"
	"os"
	"path/filepath"
	"slices"
)

var (
	DefaultExcludes = []string{
		".git",
		"node_modules",
		".idea",
		".cache",
		".vscode",
		".DS_Store",
		".gitignore",
		".gitmodules",
		".gitattributes",
		".travis.yml",
		"vendor",
	}
)

func GlobExpand(patterns []string, watchCurrentDir bool) []string {
	var expanded []string
	var wdAdded bool = false

	cwd, err := os.Getwd()
	if err != nil {
		log.Fatalf("Getwd(): %v\n", err)
	}

	workingDir, err := filepath.Abs(cwd)
	if err != nil {
		log.Fatalf("filepath.Abs(): %v\n", err)
	}

	for _, pattern := range patterns {
		if pattern == "." {
			expanded = append(expanded, workingDir)
			wdAdded = true
			continue
		}

		// glob the pattern
		joinedPattern := filepath.Join(workingDir, pattern)
		matches, err2 := filepath.Glob(joinedPattern)
		if err2 != nil {
			log.Fatalf("Invalid glob pattern or path: %v\n", err2)
		}

		// append matches to expanded patterns if not already present
		for _, match := range matches {
			if !slices.Contains(expanded, match) {
				expanded = append(expanded, match)
			}
		}
	}

	// add working directory if not already present
	if !wdAdded && watchCurrentDir {
		expanded = append(expanded, workingDir)
		wdAdded = true
	}
	return expanded
}
