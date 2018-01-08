CC = gcc
CFLAGS =
FSDETECT_SOURCES = fsdetect_main.c fsdetect.c fsdetect_ext.c fsdetect_ntfs.c fsdetect_fat.c fsdetect_btrfs.c
TCC = tcc

.PHONY: clean

fsdetect: $(FSDETECT_SOURCES)
	gcc -s -O2 -W -Wall -Wextra -Werror -ansi -pedantic $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

fsdetect.yes: $(FSDETECT_SOURCES)
	gcc -g -W -Wall -Wextra -Werror -ansi -pedantic -DDEBUG $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

fsdetect.xstatic: $(FSDETECT_SOURCES)
	xstatic gcc -s -O2 -W -Wall -Wextra -Werror -ansi -pedantic $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

fsdetect.xtiny: $(FSDETECT_SOURCES)
	xtiny gcc -s -Os -W -Wall -Wextra -Werror -ansi -pedantic $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

fsdetect.tcc: $(FSDETECT_SOURCES)
	$(TCC) -m32 -s -Os -W -Wall -Wextra -Werror -pedantic $(CFLAGS) -o $@ $(FSDETECT_SOURCES)

clean:
	rm -f fsdetect fsdetect.yes fsdetect.xstatic fsdetect.xtiny fsdetect.tcc
