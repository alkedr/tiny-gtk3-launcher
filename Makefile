SHELL := bash

tiny-gtk3-launcher: main.cpp Makefile
	clang++ -std=c++11 -Oz -s -Weverything -Wno-c++98-compat -Wno-global-constructors -Wno-exit-time-destructors $(shell pkg-config --cflags --libs gtk+-3.0 | sed 's/-I\//-isystem\ \//g') $< -o $@

clean:
	rm -f tiny-gtk3-launcher
