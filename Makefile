# Unix Makefile for fvcbm
# Dan Fandrich
#
# Tested using GNU make & SCO's make

PREFIX=		/usr/local
BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/share/man

# All the flags in $(CC) are optional and only used to generate warnings

# Linux
LINUX_CC=	gcc
LINUX_CFLAGS=	-O2 -DUNIX -Wall -Wshadow -Wpedantic -Wcast-qual -Wcast-align -Wwrite-strings -Wno-attributes

# SunOS (other than i386)
SUN4_CC=	gcc
# This line allows more strict warnings, but gives lots on some systems
#SUN4_CC=	gcc -pipe -ansi -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
SUN4_CFLAGS=	-O -DUNIX -DSUNOS -DIS_BIG_ENDIAN -pipe -ansi -pedantic -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

# SCO UNIX (tested with SYSV/386 Rel. 3.2 with Microsoft C)
SCO_CC=		cc
SCO_CFLAGS=	-O -DUNIX -DSCO -W2
SCO_PACKFLAG=	-Zp1

# 64-bit Windows using mingw64
WIN_CC=	x86_64-w64-mingw32-gcc
WIN_CFLAGS=	-O2 -Wall -Wshadow -Wpedantic -Wcast-qual -Wcast-align -Wwrite-strings -Wno-attributes

# generic big-endian machine with gcc (untested)
BIG_CC=		gcc
BIG_CFLAGS=	-O -DUNIX -DIS_BIG_ENDIAN -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

# generic little-endian machine with gcc (untested)
LITTLE_CC=	gcc
LITTLE_CFLAGS=	-O -DUNIX -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

# CP/M with Z88DK
CPM_CC=	zcc
CPM_CFLAGS=	+cpm -create-app

#
# Items below this line need not be changed
#

all:
	@echo ""
	@echo "Please run make with one of the following arguments"
	@echo "cpm        -- for CP/M machines with Z88DK cross-compiler"
	@echo "dos        -- for MS-DOS with Turbo C"
	@echo "linux      -- for Linux with gcc"
	@echo "sun4       -- for SUN 4 OS"
	@echo "sco        -- for SCO machines with Microsoft cc"
	@echo "sgi        -- for SGI machines"
	@echo "win        -- for 64-bit Windows with mingw"
	@echo "big        -- for other big-endian machines with gcc (untested)"
	@echo "little     -- for other little-endian (or unknown) machines with gcc (untested)"
	@echo "unknown    -- for other unknown-endian machines with gcc (untested)"
	@echo "test       -- run regression tests"
	@echo ""

cpm:
	make targets CC="$(CPM_CC)" CFLAGS="$(CPM_CFLAGS) $(CFLAGS)" PACKFLAG=""

dos:
	make -fmakefile.dos

linux:
	make targets CC="$(LINUX_CC)" CFLAGS="$(LINUX_CFLAGS) $(CFLAGS)" PACKFLAG=""

sun4:
	make targets CC="$(SUN4_CC)" CFLAGS="$(SUN4_CFLAGS) $(CFLAGS)" PACKFLAG=""

sco:
	make targets CC="$(SCO_CC)" CFLAGS="$(SCO_CFLAGS) $(CFLAGS)" PACKFLAG="$(SCO_PACKFLAG)"

sgi:
	@echo "Sorry, it doesn't seem to be possible to pack structures with SGI's"
	@echo "compiler.  If you have gcc installed, try \"make big\"."

win:
	make targets CC="$(WIN_CC)" CFLAGS="$(WIN_CFLAGS) $(CFLAGS)" PACKFLAG=""

big:
	make targets CC="$(BIG_CC)" CFLAGS="$(BIG_CFLAGS) $(CFLAGS)" PACKFLAG=""

little unknown:
	make targets CC="$(LITTLE_CC)" CFLAGS="$(LITTLE_CFLAGS) $(CFLAGS)" PACKFLAG=""

# It is expected that one of the above targets was used to build first
test:
	./fvcbm testdata/* > generate.txt
	diff expect.txt generate.txt
	./fvcbm -d testdata/* > generate.txt
	diff expect-d.txt generate.txt

#
# fvcbm targets below this line
#

targets: fvcbm fvcbm.man

fvcbm:	fvcbm.o cbmarcs.o
	$(CC) $(CFLAGS) -o $@ fvcbm.o cbmarcs.o

cbmarcs.o:	cbmarcs.c cbmarcs.h
	$(CC) $(CFLAGS) $(PACKFLAG) -c $<

fvcbm.o:	fvcbm.c cbmarcs.h
	$(CC) $(CFLAGS) -c $<

fvcbm.man:	fvcbm.1
	nroff -man -c $? > $@

install:
	install -m 755 -o root -g bin fvcbm $(BINDIR)
	install -m 644 -o root -g root fvcbm.1 $(MANDIR)/man1

clean:
	rm -f fvcbm fvcbm.exe fvcbm.com fvcbm.o cbmarcs.o fvcbm.man core generate.txt

zip:
	zip -9z fvcbm.zip README desc.sdi file_id.diz descript.ion fvcbm.1 Makefile makefile.dos fvcbm.c cbmarcs.c cbmarcs.h fvcbm.exe COPYING < desc.sdi
