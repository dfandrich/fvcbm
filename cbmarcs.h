/*
 * cbmarcs.h
 *
 * Commodore archive formats directory display routines
 * for fvcbm ver. 2.1
 *
 */

#include <stdio.h>
#ifdef __GNUC__
#include <endian.h>
#endif

#if ENDIAN==_LITTLE_ENDIAN
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

/* Remember to change ArchiveFormats[] if you change these enums */
enum ArchiveTypes {
	C64__0,
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

	UnknownArchive
};

extern char *ArchiveFormats[];

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
			int (DisplayFile)(const char *Name, const char *Type, unsigned long Length,
			unsigned Blocks, const char *Storage, int Compression,
			unsigned BlocksNow, long Checksum));

int DirARC(FILE *InFile, enum ArchiveTypes ArchiveType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirLynx(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirLHA(FILE *InFile, enum ArchiveTypes ArchiveType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirT64(FILE *InFile, enum ArchiveTypes ArchiveType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirD64(FILE *InFile, enum ArchiveTypes D64Type, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirP00(FILE *InFile, enum ArchiveTypes ArchiveType, struct ArcTotals *Totals,
	int (DisplayFunction)());
