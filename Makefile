BINARY_NAME = watchdog
SRCS = lib/inotify_utils.c lib/process_utils.c
OBJS = $(SRCS:lib/%.c=lib/%.o)
LIBS = $(OBJS:.o=.a)

build: bin/$(BINARY_NAME)

bin/$(BINARY_NAME): $(OBJS) $(LIBS)
	@if [ -z "$$(which musl-gcc)" ]; then \
		echo "musl-gcc not found, using gcc to build dynamic library"; \
		CGO_ENABLED=1 CC=gcc go build -ldflags='-w -s -linkmode=external' -o bin/$(BINARY_NAME); \
	else \
		echo "using musl-gcc to build static binary"; \
		CGO_ENABLED=1 CC=musl-gcc go build -ldflags='-w -s -linkmode=external -extldflags="-static"' -o bin/$(BINARY_NAME);\
	fi

lib/%.o: lib/%.c
	gcc -c -o $@ $<

lib/%.a: lib/%.o
	ar rcs $@ $<

clean:
	@rm -rf bin lib/*.o lib/*.a

.PHONY: build clean
