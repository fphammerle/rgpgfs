.PHONY = default format

default : rgpgfs

src/fs.o : src/fs.c src/fs.h
	gcc -Wall -Werror -I"$(CURDIR)" -c $< -o $@

src/main.o : src/main.c src/fs.h
	gcc -Wall -Werror -I"$(CURDIR)" -c $< -o $@ \
		$(shell pkg-config fuse3 --cflags) \
		$(shell gpgme-config --cflags)

rgpgfs : src/*.o
	gcc $^ -o $@ \
		$(shell pkg-config fuse3 --libs) \
		$(shell gpgme-config --libs)

format : src/*.c
	clang-format -i -verbose $^
