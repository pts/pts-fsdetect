CC = gcc
FSDETECT_SOURCES = fsdetect_main.c fsdetect.c fsdetect_ext.c

.PHONY: clean

fsdetect: $(FSDETECT_SOURCES)
	gcc -s -O2 -W -Wall -Wextra -Werror -ansi -pedantic -o $@ $(FSDETECT_SOURCES)

fsdetect.xstatic: $(FSDETECT_SOURCES)
	xstatic gcc -s -O2 -W -Wall -Wextra -Werror -ansi -pedantic -o $@ $(FSDETECT_SOURCES)

clean:
	rm -f fsdetect fsdetect.xstatic

