CC = gcc
CFLAGS =
FSDETECT_SOURCES = fsdetect_main.c fsdetect.c fsdetect_ext.c fsdetect_ntfs.c

.PHONY: clean

fsdetect: $(FSDETECT_SOURCES)
	gcc -s -O2 -W -Wall -Wextra -Werror -ansi -pedantic $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

fsdetect.yes: $(FSDETECT_SOURCES)
	gcc -g -W -Wall -Wextra -Werror -ansi -pedantic -DDEBUG $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

fsdetect.xstatic: $(FSDETECT_SOURCES)
	xstatic gcc -s -O2 -W -Wall -Wextra -Werror -ansi -pedantic $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

clean:
	rm -f fsdetect fsdetect.xstatic

