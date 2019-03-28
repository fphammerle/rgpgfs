SOURCES = *.c

.PHONY = format

rgpgfs : $(SOURCES)
	gcc -Wall -Werror $^ $(shell pkg-config fuse3 --cflags --libs) -o $@

format : $(SOURCES)
	clang-format -i -verbose $^
