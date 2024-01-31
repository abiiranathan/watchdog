package watchdog

import (
	"log"
	"os"
	"path/filepath"
	"slices"
	"strings"

	"github.com/abiiranathan/walkman"
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

func GlobExpand(patterns []string, watchCurrentDir, recursive bool) []string {
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

	recurseDir := func(dir string) []string {
		wm := walkman.New() // concurrent walker
		fileMap, err := wm.Walk(dir)
		if err != nil {
			log.Fatal(err)
		}

		var filesToReturn []string
		for _, filesList := range fileMap {
			for _, file := range filesList {
				filesToReturn = append(filesToReturn, file.Path)
			}
		}
		return filesToReturn
	}

	for _, pattern := range patterns {
		if pattern == "." {
			expanded = append(expanded, workingDir)
			expanded = append(expanded, recurseDir(workingDir)...)
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
				expanded = append(expanded, recurseDir(match)...)
			}
		}
	}

	// add working directory if not already present
	if !wdAdded && watchCurrentDir {
		expanded = append(expanded, workingDir)
		expanded = append(expanded, recurseDir(workingDir)...)
		wdAdded = true
	}
	return expanded
}

func Filter(list []string, exc []string) []string {
	results := make([]string, 0, len(list))
	endswithExcludedPath := func(lpath string) bool {
		for _, p := range exc {
			if strings.HasSuffix(lpath, p) || strings.HasSuffix(lpath, filepath.Base(p)) {
				return true
			}
		}
		return false
	}

	for _, f := range list {
		if !slices.Contains(exc, f) && !endswithExcludedPath(f) {
			results = append(results, f)
		}
	}
	return results
}

func Unique(list []string) []string {
	uniqueMap := make(map[string]bool)
	for _, item := range list {
		uniqueMap[item] = true
	}

	uniqueList := make([]string, 0, len(uniqueMap))
	for item := range uniqueMap {
		uniqueList = append(uniqueList, item)
	}
	return uniqueList
}
