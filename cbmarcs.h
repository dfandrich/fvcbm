/*
 * cbmarcs.h
 *
 * Commodore archive formats directory display routines
 *
 * fvcbm is copyright 1993-2023 Dan Fandrich, et. al.
 * fvcbm is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * fvcbm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with fvcbm; if not, see <https://www.gnu.org/licenses/>.
 */

/*
 * System type defined upon the following
 * __TURBOC__ : All Borland C/C++ versions
 * __ZTC__    : Zortech
 * __SC__     : Symantec
 * __WATCOM__ : Watcom
 * MSDOS      : Set by various DOS compilers... ( Watcom 386, Microsoft )
 */

#if defined(__TURBOC__) || defined(__ZTC__) || defined(MSDOS) || \
	defined(__SC__) || defined(__WATCOMC__)
#ifndef __MSDOS__
#define __MSDOS__
#endif
#endif

#include <stdio.h>
#if (!defined(SUNOS) && !defined(sun))
#ifdef __GNUC__
#include <endian.h>
#endif
#endif

#if (__BYTE_ORDER == __LITTLE_ENDIAN) && !defined(IS_BIG_ENDIAN)
/* little-endian conversion macros */
#define CF_LE_W(n) (n)
#define CF_LE_L(n) (n)
#else
/* big-endian conversion macros */
#define CF_LE_W(n) ((((n) & 0xff) << 8) | (((n) & 0xff00) >> 8))
#define CF_LE_L(n) ((((n) & 0xff) << 24) | (((n) & 0xff00) << 8) | \
					(((n) & 0xff0000L) >> 8) | (((n) & 0xff000000L) >> 24))
#endif

typedef unsigned char BYTE;		/* 8 bits */
typedef unsigned short WORD;	/* 16 bits */
typedef long LONG;				/* 32 bits */

/* Codes for each identifiable archive type */
/* Remember to change ArchiveFormats[], DirFunctions[] and TestFunctions[]
   if you change these enums */
enum ArchiveTypes {
	C64_ARC,
	C64_10,
	C64_13,
	C64_15,
	C128_15,
	LHA_SFX,
	LHA,
	Lynx,
	LynxNew,
	T64,
	D64,
	C1581,
	X64,
	P00,
	S00,
	U00,
	R00,
	D00,
	X00,
	N64,
	LBR,

	UnknownArchive
};

extern const char * const ArchiveFormats[];
extern int WideFormat;

struct ArcTotals {
	int ArchiveEntries;
	int TotalBlocks;
	int TotalBlocksNow;
	long TotalLength;
	int DearcerBlocks;

	int Version;		/* Not a total, but still interesting info */
						/* Version > 0 is an integer
						   Version < 0 is a fixed point integer to 1 decimal
						   Version = 0 is unknown or n/a */
};


enum ArchiveTypes DetermineArchiveType(FILE *InFile, const char *FileName);
int DirArchive(FILE *InFile, enum ArchiveTypes SDAType,
		struct ArcTotals *Totals,
		void (*DisplayStart)(enum ArchiveTypes ArchiveType, const char *Name),
		int (*DisplayEntry)(const char *Name, const char *Type, unsigned long Length,
			unsigned Blocks, const char *Storage, int Compression,
			unsigned BlocksNow, long Checksum));
