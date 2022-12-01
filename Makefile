CFLAGS = -g
LDFLAGS = -Iinclude

sources = $(wildcard src/*)
objects = $(patsubst src/%.c, build/obj/%.o, $(sources))

.PHONY: all clean setup

all: lib/libcserv.a example/bin/cserv_example

clean:
	rm -rf build/*
	rm -rf lib/*
	rm -rf example/bin/*

setup:
	mkdir -p build build/obj example/bin lib

build/obj/%.o: src/%.c setup
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

lib/libcserv.a: $(objects)
	ar rcs $@ $^

example/bin/cserv_example: example/main.c setup
	$(CC) -g -o $@ $< -Iinclude -Llib -lcserv