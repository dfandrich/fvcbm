# Unix Makefile for fvcbm ver. 2.1
# Dan Fandrich
#
# Tested using GNU make

BINDIR=		/usr/local/bin
MANDIR=		/usr/local/man

# Linux (tested with i386 gcc 2.5.8)
# All the flags in $(CC) are optional
CC=		gcc -pipe -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
CFLAGS=	-O -DUNIX

# SunOS (other than i386)
#CC=		gcc -pipe -ansi -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
#CFLAGS=		-O -DUNIX -DSUNOS -DIS_BIG_ENDIAN

# SCO UNIX (tested with SYSV/386 Rel. 3.2 with Microsoft C)
#CC=		cc
#CFLAGS=	-DUNIX -O
#PACKFLAG=	-Zp1

fvcbm:	fvcbm.o cbmarcs.o
	$(CC) $(CFLAGS) -o $@ fvcbm.o cbmarcs.o

cbmarcs.o:	cbmarcs.c cbmarcs.h
	$(CC) $(CFLAGS) $(PACKFLAG) -c $<

fvcbm.o:	fvcbm.c cbmarcs.h
	$(CC) $(CFLAGS) -c $<

fvcbm.man:	fvcbm.1
	nroff -man $^ > $@

install:
	install -m 755 fvcbm $(BINDIR)
	install -m 644 fvcbm.1 $(MANDIR)/man1

clean:
	rm -f fvcbm fvcbm.o cbmarcs.o fvcbm.shar core

shar:
	shar README desc.sdi file_id.diz descript.ion fvcbm.1 Makefile makefile.dos fvcbm.c cbmarcs.c cbmarcs.h >fvcbm.shar

zip:
	zip -9z fvcbm21.zip README desc.sdi file_id.diz descript.ion fvcbm.1 Makefile makefile.dos fvcbm.c cbmarcs.c cbmarcs.h fvcbm.exe < desc.sdi
