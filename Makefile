.PHONY = default format

default : rgpgfs

src/fs.o : src/fs.c src/fs.h
	gcc -Wall -Werror -I"$(CURDIR)" -c $< -o $@

rgpgfs.o : rgpgfs.c src/fs.h
	gcc -Wall -Werror -c $< -o $@ \
		$(shell pkg-config fuse3 --cflags) \
		$(shell gpgme-config --cflags)

rgpgfs : src/fs.o rgpgfs.o
	gcc $^ -o $@ \
		$(shell pkg-config fuse3 --libs) \
		$(shell gpgme-config --libs)

format : src/*.c rgpgfs.c
	clang-format -i -verbose $^
