/*
 * fvcbm.c
 *
 * File View for Commodore 64 and 128 self dissolving archives
 * Displays a list of the file names and file info contained within a SDA file
 * Inspired by Vernon D. Buerg's FV for MS-DOS archives
 *
 * Written for MS-DOS; compiled under Turbo C ver. 2.0
 * There are many dependencies on little-endian architecture -- porting to
 *   big-endian machines will take some work
 * Structures must be byte aligned
 * Source file tab size is 4
 *
 * Version:
 *	93-01-05	by Daniel Fandrich
 *				Internet: shad04@ccu.umanitoba.ca; CompuServe: 72365,306
 *	(currently unreleased version 1.0)
 *
 * This program is in the public domain.  Use it as you see fit.
 */

/******************************************************************************
* Include files
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __MSDOS__
#include <dir.h>
#else
#include <sys/param.h>
#endif

/******************************************************************************
* Constants
******************************************************************************/
#ifdef __MSDOS__
unsigned _stklen = 8000;	/* printf() does strange things sometimes with the
							   default 4k stack */
#define READ_BINARY "rb"
#else
#define READ_BINARY "r"
#endif

typedef unsigned int WORD;
typedef unsigned char BYTE;

char *ProgName = "fvcbm";
#define DefaultExt ".sda"

BYTE MagicHeaderC64[10] = {0x9e, '(', '2', '0', '6', '3', ')', 0x00, 0x00, 0x00};
BYTE MagicHeaderC128[10] = {0x9e, '(', '7', '1', '8', '3', ')', 0x00, 0x00, 0x00};
BYTE MagicHeaderSFX[10] = {0x97, 0x32, 0x30, 0x2C, 0x30, 0x3A, 0x8B, 0xC2, 0x28, 0x32};
BYTE MagicC64_10[3] = {0x85, 0xfd, 0xa9};
BYTE MagicC64_13[3] = {0x85, 0x2f, 0xa9};
BYTE MagicC64_15[4] = {0x8d, 0x21, 0xd0, 0x4c};
BYTE MagicC128_15 = 0x4c;

enum {MagicEntry = 2};

/* Remember to change SDAFormats[] */
enum SDATypes {
	C64__0,
	C64_10,
	C64_13,
	C64_15,
	C128_15,
	SFX,

	UnknownSDA
};

char *EntryTypes[] = {
/* 0 */	"Stored",
/* 1 */ "Packed",
/* 2 */ "Squeezed",
/* 3 */ "Crunched",
/* 4 */ "Squashed",
/* 5 */ "?5",
/* 6 */ "?6",
/* 7 */ "?7",
/* 8 */ "?8"
};

char *SDAFormats[] = {
/* C64__0 */	" ARC",
/* C64_10 */	" C64",
/* C64_13 */	" C64",
/* C64_15 */	" C64",
/* C128_15 */	"C128"
};

/******************************************************************************
* Structures
******************************************************************************/
struct SDAHeader {
	WORD StartAddress;
	BYTE Filler[2];
	WORD Version;
	BYTE Magic[10];
	union {
		struct {
			BYTE Filler;
			BYTE FirstOffL;
			BYTE Magic[3];
			BYTE FirstOffH;
		} C64_10;
		struct {
			BYTE Filler[11];
			BYTE FirstOffL;
			BYTE Magic[3];
			BYTE FirstOffH;
		} C64_13;
		struct {
			BYTE Filler[7];
			BYTE Magic[4];
			WORD StartPointer;
		} C64_15;
		struct {
			BYTE Magic;
			WORD StartPointer;
		} C128_15;
	} Pointers;
};

struct SDAHeaderNew {
	BYTE Filler1;
	BYTE FirstOffL;
	BYTE Filler2[3];
	BYTE FirstOffH;
};

struct SDAEntryHeader {
	BYTE Magic;
	BYTE EntryType;
	WORD Checksum;
	WORD LengthL;
	BYTE LengthH;
	BYTE BlockLength;
	BYTE Filler;
	char FileType;
	char FileNameLen;
};

char *FileTypes(char TypeCode)
{
	switch (TypeCode) {
		case 'P': return "PRG";
		case 'S': return "SEQ";
		case 'U': return "USR";
		case 'R': return "REL";
		default:  return "???";
	}
}

/******************************************************************************
* Main program
******************************************************************************/
int main(int argc, char *argv[])
{
	int ArchiveEntries;
	long CurrentPos;
	int DearcerBlocks;
	char EntryName[17];
	struct SDAEntryHeader FileHeader;
	struct SDAHeaderNew FileHeaderNew;
	long FileLen;
	char FileName[MAXPATH+1];
	struct SDAHeader Header;
	FILE *InFile;
	int TotalBlocks;
	int TotalBlocksNow;
	long TotalLength;
	enum SDATypes SDAType;

	printf("%s  ver. 1.0  93-01-05  by Daniel Fandrich\n\n", ProgName);
	if ((argc < 2) || ((argv[1][0] == '-') && (argv[1][1] == '?'))) {
		printf("Usage: %s filename[" DefaultExt "]\n"
			   "View directory of Commodore 64/128 self dissolving archive files\n"
			   "Placed into the public domain\n", ProgName);
		exit(1);
	}

/******************************************************************************
* Open the SDA file
******************************************************************************/
	strncpy(FileName, argv[1], sizeof(FileName) - sizeof(DefaultExt) - 1);
	if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {
		strcat(FileName, DefaultExt);
		if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {
			perror(ProgName);
			fclose(InFile);
			exit(2);
		}
	}

/******************************************************************************
* Read the SDA and determine which type it is
******************************************************************************/
	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		perror(ProgName);
		fclose(InFile);
		exit(2);
	}

	SDAType = UnknownSDA;
	if (memcmp(Header.Magic, MagicHeaderC64, sizeof(MagicHeaderC64)) == 0) {
		if (memcmp(Header.Pointers.C64_10.Magic, MagicC64_10, sizeof(MagicC64_10)) == 0)
			SDAType = C64_10;
		if (memcmp(Header.Pointers.C64_13.Magic, MagicC64_13, sizeof(MagicC64_13)) == 0)
			SDAType = C64_13;
		if (memcmp(Header.Pointers.C64_15.Magic, MagicC64_15, sizeof(MagicC64_15)) == 0)
			SDAType = C64_15;
	} else {
		if (memcmp(Header.Magic, MagicHeaderC128, sizeof(MagicHeaderC128)) == 0) {
			if (Header.Pointers.C128_15.Magic == MagicC128_15)
				SDAType = C128_15;
		} else {	/* This type does not have a dearcer built in, just the data */
			if ((BYTE) Header.StartAddress == MagicEntry) {
				SDAType = C64__0;
				Header.Version = 0;
			} else {	/* This is an SFX archive in LHA format -- not supported */
				if (memcmp(Header.Magic, MagicHeaderSFX, sizeof(MagicHeaderSFX)) == 0)
						SDAType = SFX;
			}
		}
	}


/******************************************************************************
* Find the first archive entry for each format -- the remainder are the same
* for each SDA type
******************************************************************************/
	switch (SDAType) {
		case C64__0:	/* Not a self dearcer -- just the arc data */
			CurrentPos = 0L;
			break;

		case C64_10:
			CurrentPos = ((Header.Pointers.C64_10.FirstOffH << 8) | Header.Pointers.C64_10.FirstOffL) - Header.StartAddress + 2;
			break;

		case C64_13:
			CurrentPos = ((Header.Pointers.C64_13.FirstOffH << 8) | Header.Pointers.C64_13.FirstOffL) - Header.StartAddress + 2;
			break;

		case C64_15:
			fseek(InFile, Header.Pointers.C64_15.StartPointer - Header.StartAddress + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			CurrentPos = ((FileHeaderNew.FirstOffH << 8) | FileHeaderNew.FirstOffL) - Header.StartAddress + 2;
			break;

		case C128_15:
			fseek(InFile, Header.Pointers.C128_15.StartPointer - Header.StartAddress + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			CurrentPos = ((FileHeaderNew.FirstOffH << 8) | FileHeaderNew.FirstOffL) - Header.StartAddress + 2;
			break;

		case SFX:
			printf("%s: Archive is probably SFX (LZH); use LHA to view contents\n", ProgName);
			fclose(InFile);
			exit(3);

		case UnknownSDA:
			printf("%s: Not a known Commodore SDA\n", ProgName);
			fclose(InFile);
			exit(3);
	}

/******************************************************************************
* Display the SDA contents
******************************************************************************/
	DearcerBlocks = CurrentPos ? CurrentPos / 254 + 1 : 0;
	ArchiveEntries =0;
	TotalBlocks = 0;
	TotalBlocksNow = 0;
	TotalLength = 0;

	printf("Name              Type  Length  Blks  Method     SF   Now   Check\n");
	printf("================  ====  ======  ====  ========  ====  ====  =====\n");

	fseek(InFile, CurrentPos, SEEK_SET);

	while (1) {
		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;
		if (FileHeader.Magic != MagicEntry)
			break;
		fread(&EntryName, FileHeader.FileNameLen, 1, InFile);
		EntryName[FileHeader.FileNameLen] = 0;

		FileLen = (long) (FileHeader.LengthH << 16L) | FileHeader.LengthL;
		printf("%-16s  %s  %7lu  %4u  %-8s %4d%%  %4u   %04X\n",
			EntryName,
			FileTypes(FileHeader.FileType),
			(long) FileLen,
			(unsigned) FileLen / 254 + 1,
			EntryTypes[FileHeader.EntryType],
			(int) (100 - (FileHeader.BlockLength * 100L / (FileLen / 254 + 1))),
			(unsigned) FileHeader.BlockLength,
			FileHeader.Checksum
		);

		CurrentPos += FileHeader.BlockLength * 254;
		fseek(InFile, CurrentPos, SEEK_SET);
		++ArchiveEntries;
		TotalLength += FileLen;
		TotalBlocks += FileLen / 254 + 1;
		TotalBlocksNow += FileHeader.BlockLength;
	};

	printf("================  ====  ======  ====  ========  ====  ====  =====\n");
	printf("*total %5u           %7lu  %4d  %s %u.%u %4d%%  %4d+%d\n",
		ArchiveEntries,
		TotalLength,
		TotalBlocks,
		SDAFormats[SDAType],
		Header.Version / 10,
		Header.Version - 10 * (Header.Version / 10),
		(unsigned) (100 - (TotalBlocksNow * 100L / (TotalBlocks))),
		TotalBlocksNow,
		DearcerBlocks
	);
	fclose(InFile);

	return 0;
}
