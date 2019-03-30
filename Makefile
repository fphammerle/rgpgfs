CC ?= gcc
LD = gcc

CFLAGS := ${CFLAGS} -Wall -Werror -I"$(CURDIR)"
CFLAGS += $(shell gpgme-config --cflags)
CFLAGS += $(shell pkg-config fuse3 --cflags)

LIBS += $(shell gpgme-config --libs)
LIBS += $(shell pkg-config fuse3 --libs)

.PHONY = default format

default : rgpgfs

src/fs.o : src/fs.c src/fs.h
	$(CC) $(CFLAGS) -c $< -o $@

src/gpgme.o : src/gpgme.c src/gpgme.h
	$(CC) $(CFLAGS) -c $< -o $@

src/main.o : src/main.c src/fs.h src/gpgme.h
	$(CC) $(CFLAGS) -c $< -o $@

rgpgfs : src/*.o
	$(LD) $^ -o $@ $(LIBS)

format : src/*.h src/*.c
	clang-format -i -verbose $^
