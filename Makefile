# Unix Makefile for fvcbm
# Dan Fandrich
#
# Tested using GNU make & SCO's make

PREFIX=		/usr/local
BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/share/man

# Default
CC=cc
CFLAGS=-O2
LDFLAGS=
PACKFLAG=

# Linux
LINUX_CC=	gcc
LINUX_CFLAGS=	-O2 -Wall -Wshadow -Wpedantic -Wcast-qual -Wcast-align -Wwrite-strings -Wno-attributes

# SunOS (other than i386)
SUN4_CC=	gcc
# This line allows more strict warnings, but gives lots on some systems
#SUN4_CC=	gcc -pipe -ansi -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
SUN4_CFLAGS=	-O -DSUNOS -DIS_BIG_ENDIAN -pipe -ansi -pedantic -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

# SCO UNIX (tested with SYSV/386 Rel. 3.2 with Microsoft C)
SCO_CC=		cc
SCO_CFLAGS=	-O -DSCO -W2
SCO_PACKFLAG=	-Zp1

# 64-bit Windows using mingw64
WIN_CC=	x86_64-w64-mingw32-gcc
WIN_CFLAGS=	-O2 -Wall -Wshadow -Wpedantic -Wcast-qual -Wcast-align -Wwrite-strings -Wno-attributes

# generic big-endian machine with gcc (untested)
BIG_CC=		gcc
BIG_CFLAGS=	-O -DIS_BIG_ENDIAN -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

# generic little-endian machine with gcc (untested)
LITTLE_CC=	gcc
LITTLE_CFLAGS=	-O -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

# CP/M with Z88DK
CPM_CC=	zcc
CPM_CFLAGS=	+cpm -create-app

#
# Items below this line need not be changed
#

all: targets

help:
	@echo ""
	@echo "Run make with one of the following arguments for a non-default target:"
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
	$(MAKE) targets CC="$(CPM_CC)" CFLAGS="$(CPM_CFLAGS) $(CFLAGS)" PACKFLAG=""

dos:
	make -fmakefile.dos

linux:
	$(MAKE) targets CC="$(LINUX_CC)" CFLAGS="$(LINUX_CFLAGS) $(CFLAGS)" PACKFLAG=""

sun4:
	$(MAKE) targets CC="$(SUN4_CC)" CFLAGS="$(SUN4_CFLAGS) $(CFLAGS)" PACKFLAG=""

sco:
	$(MAKE) targets CC="$(SCO_CC)" CFLAGS="$(SCO_CFLAGS) $(CFLAGS)" PACKFLAG="$(SCO_PACKFLAG)"

sgi:
	@echo "Sorry, it doesn't seem to be possible to pack structures with SGI's"
	@echo "compiler.  If you have gcc installed, try \"make big\"."

win:
	$(MAKE) targets CC="$(WIN_CC)" CFLAGS="$(WIN_CFLAGS) $(CFLAGS)" PACKFLAG=""

big:
	$(MAKE) targets CC="$(BIG_CC)" CFLAGS="$(BIG_CFLAGS) $(CFLAGS)" PACKFLAG=""

little unknown:
	$(MAKE) targets CC="$(LITTLE_CC)" CFLAGS="$(LITTLE_CFLAGS) $(CFLAGS)" PACKFLAG=""

# It is expected that one of the above targets was used to build first
# This can be used to run a program with something like wine or qemu
TESTWRAPPER=
test:
	$(TESTWRAPPER) ./fvcbm --
	$(TESTWRAPPER) ./fvcbm -d --
	$(TESTWRAPPER) ./fvcbm || test "$$?" = 1
	$(TESTWRAPPER) ./fvcbm > generate.txt || test "$$?" = 1
	$(TESTWRAPPER) ./fvcbm -h > generate.txt || test "$$?" = 1
	$(TESTWRAPPER) ./fvcbm -d > generate.txt || test "$$?" = 1
	$(TESTWRAPPER) ./fvcbm testdata/* > generate.txt 2>&1
	diff expect.txt generate.txt
	$(TESTWRAPPER) ./fvcbm -d testdata/* > generate.txt 2>&1
	diff expect-d.txt generate.txt
	$(TESTWRAPPER) ./fvcbm testdata/test1 > generate.txt 2>&1
	diff expect-x.txt generate.txt
	$(TESTWRAPPER) ./fvcbm -- testdata/test1 > generate.txt 2>&1
	diff expect-x.txt generate.txt

#
# fvcbm targets below this line
#

targets: fvcbm fvcbm.man

fvcbm:	fvcbm.o cbmarcs.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ fvcbm.o cbmarcs.o

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
