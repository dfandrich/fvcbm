# Unix Makefile for fvcbm ver. 3.0
# Dan Fandrich
#
# Tested using GNU make & SCO's make

BINDIR=		/usr/local/bin
MANDIR=		/usr/local/man

# All the flags in $(CC) are optional and only used to generate warnings

# Linux (tested with i386 gcc 2.5.8)
LINUX_CC=	gcc -pipe -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
LINUX_CFLAGS=	-O -DUNIX

# SunOS (other than i386)
SUN4_CC=	gcc -pipe -ansi -pedantic -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings
# This line allows more strict warnings, but gives lots on some systems
#SUN4_CC=	gcc -pipe -ansi -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
SUN4_CFLAGS=	-O -DUNIX -DSUNOS -DIS_BIG_ENDIAN

# SCO UNIX (tested with SYSV/386 Rel. 3.2 with Microsoft C)
SCO_CC=		cc -W2
SCO_CFLAGS=	-O -DUNIX -DSCO
SCO_PACKFLAG=	-Zp1

# generic big-endian machine with gcc (untested)
BIG_CC=		gcc -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings
BIG_CFLAGS=	-O -DUNIX -DIS_BIG_ENDIAN

# generic little-endian machine with gcc (untested)
LITTLE_CC=	gcc -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings
LITTLE_CFLAGS=	-O -DUNIX

#
# Items below this line need not be changed
#

all:
	@echo
	@echo "Please run make with one of the following arguments"
	@echo "dos        -- for MS-DOS"
	@echo "linux      -- for PC linux"
	@echo "sun4       -- for SUN 4 OS"
	@echo "sco        -- for SCO machines with Microsoft cc"
	@echo "sgi        -- for SGI machines"
	@echo "big        -- for other big-endian machines with gcc (untested)"
	@echo "little     -- for other little-endian (or unknown) machines with gcc (untested)"
	@echo "unknown    -- for other unknown-endian machines with gcc (untested)"
	@echo

dos:
	make -fmakefile.dos

linux:
	make targets CC="$(LINUX_CC)" CFLAGS="$(LINUX_CFLAGS)" PACKFLAG=""

sun4:
	make targets CC="$(SUN4_CC)" CFLAGS="$(SUN4_CFLAGS)" PACKFLAG=""

sco:
	make targets CC="$(SCO_CC)" CFLAGS="$(SCO_CFLAGS)" PACKFLAG="$(SCO_PACKFLAG)"

sgi:
	@echo "Sorry, it doesn't seem to be possible to pack structures with SGI's"
	@echo "compiler.  If you have gcc installed, try \"make big\"."

big:
	make targets CC="$(BIG_CC)" CFLAGS="$(BIG_CFLAGS)" PACKFLAG=""

little unknown:
	make targets CC="$(LITTLE_CC)" CFLAGS="$(LITTLE_CFLAGS)" PACKFLAG=""

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
	nroff -man $? > $@

install:
	install -m 755 -o root -g bin fvcbm $(BINDIR)
	install -m 644 -o root -g root fvcbm.1 $(MANDIR)/man1

clean:
	rm -f fvcbm fvcbm.o cbmarcs.o core

shar:
	shar README desc.sdi file_id.diz descript.ion fvcbm.1 Makefile makefile.dos fvcbm.c cbmarcs.c cbmarcs.h >fvcbm.shar

zip:
	zip -9z fvcbm30.zip README desc.sdi file_id.diz descript.ion fvcbm.1 Makefile makefile.dos fvcbm.c cbmarcs.c cbmarcs.h fvcbm.exe COPYING < desc.sdi
