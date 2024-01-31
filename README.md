# watchdog

Watchdog is a Go project that uses inotify to watch for file changes and execute a command when a change is detected. It uses C for the inotify handling and Go for the command execution.

## Installation

To build the project, run the following command:

```sh
make build
```

This will create a binary named `watchdog` in the `bin` directory.

Install watch dog in your go project:
```bash
go get -u github.com/abiiranathan/watchdog
```

## Usage
First, import the [`watchdog`] package in your Go file:

```go
import "github.com/abiiranathan/watchdog/watchdog"
```

Then, create a new instance of `WatchDog`:

```go
wd := watchdog.NewWatchDog()
```

To handle events, use the `HandleEvents` method. This method takes a command to execute when a change is detected, a slice of patterns to watch for changes, and a slice of patterns to exclude from watching:

```go
wd.HandleEvents("echo 'File changed'", []string{"*.go"}, []string{"*_test.go"})
```

In this example, the `echo 'File changed'` command will be executed whenever a `.go` file that does not end with `_test.go` is changed.

To enable verbose logging, use the `VerboseMode` function with `true` as the argument:

```go
watchdog.VerboseMode(true)
```

To disable verbose logging, use the `VerboseMode` function with `false` as the argument:

```go
watchdog.VerboseMode(false)
```

Finally, to clean up and close the inotify instance when you're done, use the `Close` method:

```go
wd.Close()
```

## Contributing

Contributions are welcome. Please submit a pull request or create an issue to discuss the changes you want to make.

## License

This project is licensed under the MIT License. See the LICENSE file for details.