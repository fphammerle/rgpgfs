SOURCES = *.c

.PHONY = format

rgpgfs : $(SOURCES)
	gcc -Wall -Werror $^ -o $@ \
		$(shell pkg-config fuse3 --cflags --libs) \
		$(shell gpgme-config --cflags --libs)

format : $(SOURCES)
	clang-format -i -verbose $^
