CC = gcc
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

src/main.o : src/main.c src/fs.h src/gpgme.h src/str.h
	$(CC) $(CFLAGS) -c $< -o $@

src/str.o : src/str.c src/str.h
	$(CC) $(CFLAGS) -c $< -o $@

rgpgfs : src/fs.o src/gpgme.o src/main.o src/str.o
	$(LD) $^ -o $@ $(LIBS)

tests/fs.o : tests/fs.c src/fs.h
	$(CC) $(CFLAGS) -c $< -o $@

tests/fs : tests/fs.o src/fs.o
	$(LD) $^ -o $@

tests/str.o : tests/str.c src/str.h
	$(CC) $(CFLAGS) -c $< -o $@

tests/str : tests/str.o src/str.o
	$(LD) $^ -o $@

format : src/*.h src/*.c tests/*.c
	clang-format -i -verbose $^
