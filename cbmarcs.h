/*
 * cbmarcs.h
 *
 * Commodore archive formats directory display routines
 *
 */

#include <stdio.h>

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
	X64,

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

enum ArchiveTypes DetermineArchiveType(FILE *InFile);
int DirArchive(FILE *InFile, enum ArchiveTypes SDAType,
		struct ArcTotals *Totals,
			int (DisplayFile)(const char *Name, const char *Type, unsigned long Length,
			unsigned Blocks, const char *Storage, int Compression,
			unsigned BlocksNow, long Checksum));

int DirARC(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirLynx(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirLHA(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirT64(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
	int (DisplayFunction)());
int DirD64(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
	int (DisplayFunction)());
