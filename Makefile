# Unix Makefile for fvcbm ver. 1.3
# Dan Fandrich
#
# Tested under SCO UNIX System V/386 Release 3.2, Linux 1.1.54

CC=		gcc
CFLAGS=		-DUNIX -O
#Flag to pack structures with Microsoft C (SCO)
#PACKFLAG=	-Zp1

fvcbm:	fvcbm.o cbmarcs.o
	$(CC) $(CFLAGS) -o $@ $^

cbmarcs.o:	cbmarcs.c cbmarcs.h
	$(CC) $(CFLAGS) $(PACKFLAG) -c $<

fvcbm.o:	fvcbm.c cbmarcs.h
	$(CC) $(CFLAGS) -c $<

fvcbm.man:	fvcbm.1
	nroff -man $^ > $@

clean:
	rm -f fvcbm fvcbm.o cbmarcs.o fvcbm.shar core

shar:
	shar fvcbm.1 Makefile makefile.dos fvcbm.c cbmarcs.c cbmarcs.h >fvcbm.shar
