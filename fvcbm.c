/*
 * fvcbm.c
 *
 * File View for Commodore 64 and 128 self dissolving and non-SDA archives
 * Displays a list of the file names and file info contained within a SDA file
 * Supports ARC230 (C64 & C128 versions), ARC230 SDA, Lynx, LHA and LHA SFX
 *   archive formats
 * Inspired by Vernon D. Buerg's FV for MS-DOS archives
 *
 * Written under MS-DOS; compiled under Turbo C ver. 2.0; use -a- option to
 *   align structures on bytes; link with WILDARGS.OBJ for wildcard
 *   pattern matching
 * Microsoft C support; compile with -DMSC and -Zp1 option to align structures
 *   on bytes  (note: this support has not been properly tested)
 * Support for 386 SCO UNIX System V/386 Release 3.2; compile fvcbm.c with
 *   -Zp1 option to align structures on bytes; compile filelength.c without
 *   -Zp1 and link with fvcbm
 * There are many dependencies on little-endian architecture -- porting to
 *   big-endian machines will take some work
 * Source file tab size is 4
 *
 * Things to do:
 *	- search for all supported file extensions before giving up
 *	- fix display of LHA archives with long path names
 *  - add display of Lynx oc'ult and REL record lengths
 *  - put global vars into a structure which is passed to each routine
 *
 * Version:
 *	93-01-05  ver. 1.0  by Daniel Fandrich
 *		Domain: <dan@fch.wimsey.bc.ca>; CompuServe: 72365,306
 *		Initial release
 *	93-04-14  ver. 1.1 (unreleased)
 *		Moved code around to make adding new archive types easier
 *		Added Lynx & LHA archive support
 *	93-07-24  ver. 1.2 (currently unreleased)
 *		Added support for 386 SCO UNIX
 *		Added support for Microsoft C (untested)
 *		Added support for multiple files on command line
 *		Fixed Lynx IX file lengths
 *		Added Lynx XVII support, including more reliable last file lengths
 *		Fixed LHA 0 length files
 *
 * This program is in the public domain.  Use it as you see fit.
 */

/******************************************************************************
* Include files
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__TURBOC__)
#include <dir.h>
#include <io.h>

#elif defined(MSC)
#include <io.h>

#else /* UNIX */
#include "filelength.h"
/* #include <endian.h> */
#endif

#ifdef BIG_ENDIAN
#error fvcbm requires a little-endian CPU
#endif

/******************************************************************************
* Constants
******************************************************************************/
#define VERSION "1.2"
#define VERDATE "93-07-24"

#if defined(__TURBOC__)
unsigned _stklen = 8000;	/* printf() does strange things sometimes with the
							   default 4k stack */
#define READ_BINARY "rb"

#elif defined(MSC)
#define MAXPATH 80			/* length of longest permissible file path */
#define READ_BINARY "rb"

#else /* UNIX */
#define MAXPATH 260			/* length of longest permissible file path */
#define READ_BINARY "r"
#endif

typedef unsigned char BYTE;		/* 8 bits */
typedef unsigned short WORD;	/* 16 bits */
typedef unsigned long LONG;		/* 32 bits */

char *ProgName = "fvcbm";	/* this should be changed to argv[0] for Unix */
#define DefaultExt ".sda"

BYTE MagicHeaderC64[10] = {0x9e,'(','2','0','6','3',')',0x00,0x00,0x00};
BYTE MagicHeaderC128[10] = {0x9e,'(','7','1','8','3',')',0x00,0x00,0x00};
BYTE MagicHeaderARC = 2;
BYTE MagicHeaderLHASFX[10] = {0x97,0x32,0x30,0x2C,0x30,0x3A,0x8B,0xC2,0x28,0x32};
BYTE MagicHeaderLHA[3] = {'-','l','h'};
BYTE MagicHeaderLynx[10] = {' ','1',' ',' ',' ','L','Y','N','X',' '};
BYTE MagicHeaderLynxNew[27] = {0x0A,0x00,0x97,'5','3','2','8','0',',','0',0x3A,
							   0x97,'5','3','2','8','1',',','0',0x3A,
							   0x97,'6','4','6',',',0xC2,0x28};

BYTE MagicC64_10[3] = {0x85,0xfd,0xa9};
BYTE MagicC64_13[3] = {0x85,0x2f,0xa9};
BYTE MagicC64_15[4] = {0x8d,0x21,0xd0,0x4c};
BYTE MagicC128_15 = 0x4c;

enum {MagicARCEntry = 2};
BYTE MagicLHAEntry[3] = {'-','l','h'};

/* Remember to change SDAFormats[] */
enum SDATypes {
	C64__0,
	C64_10,
	C64_13,
	C64_15,
	C128_15,

	LHA_SFX,
	LHA,
	Lynx,
	LynxNew,

	UnknownSDA
};

char *SDAFormats[] = {
/* C64__0 */	" ARC",
/* C64_10 */	" C64",
/* C64_13 */	" C64",
/* C64_15 */	" C64",
/* C128_15 */	"C128",
/* LHA_SFX */	" LHA",
/* LHA */		" LHA",
/* Lynx */		"Lynx",
/* New Lynx */	"Lynx",
};

char *ARCEntryTypes[] = {
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

char *LHAEntryTypes[] = {
/* 0 */	"Stored",
/* 1 */ "lh1",
/* 2 */ "lh2",
/* 3 */ "lh3",
/* 4 */ "lh4",
/* 5 */ "lh5",
/* 6 */ "lh6",
/* 7 */ "lh7",
/* 8 */ "lh8",
/* 9 */ "lh9",
/* A */ "lhA",
/* B */ "lhB"
};

/******************************************************************************
* Structures
******************************************************************************/
struct SDAHeader {
	union {
		struct {
			WORD StartAddress;
			BYTE Filler1[2];
			WORD Version;
			BYTE Magic1[10];

			BYTE Filler2;
			BYTE FirstOffL;
			BYTE Magic2[3];
			BYTE FirstOffH;
		} C64_10;
		struct {
			WORD StartAddress;
			BYTE Filler1[2];
			WORD Version;
			BYTE Magic1[10];

			BYTE Filler2[11];
			BYTE FirstOffL;
			BYTE Magic2[3];
			BYTE FirstOffH;
		} C64_13;
		struct {
			WORD StartAddress;
			BYTE Filler1[2];
			WORD Version;
			BYTE Magic1[10];

			BYTE Filler2[7];
			BYTE Magic2[4];
			WORD StartPointer;
		} C64_15;
		struct {
			WORD StartAddress;
			BYTE Filler1[2];
			WORD Version;
			BYTE Magic1[10];

			BYTE Magic2;
			WORD StartPointer;
		} C128_15;
		struct {
			BYTE Magic;
			BYTE EntryType;
		} C64_ARC;
		struct {
			WORD StartAddress;
			BYTE Filler[4];
			BYTE Magic[10];
		} LHA_SFX;
		struct {
			BYTE Filler[2];
			BYTE Magic[3];
		} LHA;
		struct {
			BYTE Magic[10];
		} Lynx;
		struct {
			WORD StartAddress;
			WORD EndHeaderAddr;
			BYTE Magic[27];
		} LynxNew;
	} Type;
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
	BYTE FileType;
	BYTE FileNameLen;
};

struct LHAEntryHeader {
	BYTE HeadSize;
	BYTE HeadChk;
	BYTE HeadID[3];
	BYTE EntryType;
	BYTE Magic;
	LONG PackSize;
	LONG OrigSize;
	struct	{			/* DOS format time */
		WORD ft_tsec : 5;	/* Two second interval */
		WORD ft_min	 : 6;	/* Minutes */
		WORD ft_hour : 5;	/* Hours */
		WORD ft_day	 : 5;	/* Days */
		WORD ft_month: 4;	/* Months */
		WORD ft_year : 7;	/* Year */
	} FileTime;
	WORD Attr;
	BYTE FileNameLen;
	BYTE FileName[64];
};

/******************************************************************************
* Global variables (ecch)
******************************************************************************/
int ArchiveEntries;
int TotalBlocks;
int TotalBlocksNow;
long TotalLength;

/******************************************************************************
* Functions
******************************************************************************/

/******************************************************************************
* Return smallest of 2 numbers
******************************************************************************/
#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif

/******************************************************************************
* Strips the high bit from a string
******************************************************************************/
void StripBit7(char *InString)
{
	while (*InString)
		*InString++ = *InString & 0x7F;
}

/******************************************************************************
* Convert Roman numeral to decimal
* Note: input string is cleared
******************************************************************************/
int RomanToDec(char *Roman)
{
	int Digit;
	char Last = 0;
	int Value = 0;

	while (*Roman) {
		switch (*Roman) {
			case 'I': Digit = 1; break;
			case 'V': Digit = 5; break;
			case 'X': Digit = 10; break;
			case 'L': Digit = 50; break;
			case 'C': Digit = 100; break;
		}
		Value = Last < Digit ? Digit - Value: Value + RomanToDec(Roman);
		Last = Digit;
		*Roman++ = 0;
	}
	return Value;
}

/******************************************************************************
* Display file type
******************************************************************************/
char *FileTypes(char TypeCode)
{
	switch (TypeCode) {
		case 'P': return "PRG";
		case 'S': return "SEQ";
		case 'U': return "USR";
		case 'R': return "REL";
		case ' ': return "   ";
		default:  return "???";
	}
}

/******************************************************************************
* ARC reading routines
******************************************************************************/
int DisplayARC(FILE *InFile, long CurrentPos) {
	char EntryName[17];
	long FileLen;
	struct SDAEntryHeader FileHeader;

	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		printf("%s: Problem reading archive\n", ProgName);
		fclose(InFile);
		return 2;
	}

	while (1) {
		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;
		if (FileHeader.Magic != MagicARCEntry)
			break;
		fread(&EntryName, FileHeader.FileNameLen, 1, InFile);
		EntryName[FileHeader.FileNameLen] = 0;

		FileLen = (long) (FileHeader.LengthH << 16L) | FileHeader.LengthL;
		printf("%-16s  %s  %7lu  %4u  %-8s %4d%%  %4u   %04X\n",
			EntryName,
			FileTypes(FileHeader.FileType),
			(long) FileLen,
			(unsigned) (FileLen-1) / 254 + 1,
			ARCEntryTypes[FileHeader.EntryType],
			(int) (100 - (FileHeader.BlockLength * 100L / (FileLen / 254 + 1))),
			(unsigned) FileHeader.BlockLength,
			FileHeader.Checksum
		);

		CurrentPos += FileHeader.BlockLength * 254;
		fseek(InFile, CurrentPos, SEEK_SET);
		++ArchiveEntries;
		TotalLength += FileLen;
		TotalBlocks += (FileLen-1) / 254 + 1;
		TotalBlocksNow += FileHeader.BlockLength;
	};
	return 0;
}

/******************************************************************************
* Lynx reading routines
******************************************************************************/
int DisplayLynx(FILE *InFile, long CurrentPos, char ExpectLastLength) {
	char EntryName[17];
	char FileType[2];
	int NumFiles;
	int FileBlocks;
	int LastBlockSize;
	long FileLen;

	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		printf("%s: Problem reading archive\n", ProgName);
		fclose(InFile);
		return 2;
	}

	fscanf(InFile, "%d%*[^\015]\015", &NumFiles);

	for (; NumFiles--;) {
		fscanf(InFile, "%16[^\015]%*[^\015]\015", EntryName);
		fscanf(InFile, "%d%*[^\015]\015", &FileBlocks);
		fscanf(InFile, "%1s%*[^\015]", FileType);
		getc(InFile);	/* eat the CR (\015) without killing whitespace so
						   ftell() will be correct, below */
		StripBit7(EntryName);

/******************************************************************************
* Find the exact length of the file.
* For the first n-1 entries (for all n entries in newer Lynx versions, like
*  XVII), the length in bytes of the last block of the file is specified.
* For the last entry in older Lynx versions, like IX, we must find out how many
*  bytes in the file and subtract everything not belonging to the last file.
******************************************************************************/
		if (NumFiles || ExpectLastLength) {
			fscanf(InFile, "%d%*[^\015]\015", &LastBlockSize);
			FileLen = (long) ((FileBlocks-1) * 254L + LastBlockSize - 1);
		} else				/* last entry -- calculate based on file size */
			FileLen = filelength(fileno(InFile)) - TotalBlocksNow * 254L -
							(((ftell(InFile) - 1) / 254) + 1) * 254L;

		printf("%-16s  %s  %7lu  %4u  %-8s %4d%%  %4u\n",
			EntryName,
			FileTypes(FileType[0]),
			(long) FileLen,
			FileBlocks,
			"Stored",
			0,
			FileBlocks
		);

		++ArchiveEntries;
		TotalLength += FileLen;
		TotalBlocks += (FileLen-1) / 254 + 1;	/*  \  These two values		*/
		TotalBlocksNow += FileBlocks;			/*  /  should be equal		*/
	};
	return 0;
}

/******************************************************************************
* LHA (SFX) reading routines
******************************************************************************/
int DisplayLHA(FILE *InFile, long CurrentPos) {
	struct LHAEntryHeader FileHeader;
	char FileName[80];

	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		printf("%s: Problem reading archive\n", ProgName);
		fclose(InFile);
		return 2;
	}

	while (1) {
		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;
		if (memcmp(FileHeader.HeadID, MagicLHAEntry, sizeof(MagicLHAEntry)) != 0)
			break;

		memcpy(FileName, FileHeader.FileName, min(sizeof(FileName)-1, FileHeader.FileNameLen));
		FileName[min(sizeof(FileName)-1, FileHeader.FileNameLen)] = 0;
		printf("%-16s  %s  %7lu  %4u  %-8s %4d%%  %4u   %04X\n",
			FileName,
			FileTypes(FileHeader.FileName[FileHeader.FileNameLen-2] ? ' ' : FileHeader.FileName[FileHeader.FileNameLen-1]),
			(long) FileHeader.OrigSize,
			FileHeader.OrigSize ? (unsigned) (FileHeader.OrigSize-1) / 254 + 1 : 0,
			LHAEntryTypes[FileHeader.EntryType - '0'],
			FileHeader.OrigSize ? (int) (100 - (FileHeader.PackSize * 100L / FileHeader.OrigSize)) : 100,
			FileHeader.PackSize ? (unsigned) (FileHeader.PackSize-1) / 254 + 1 : 0,
			(unsigned) (FileHeader.FileName[FileHeader.FileNameLen+1] << 8) | FileHeader.FileName[FileHeader.FileNameLen]
		);

		CurrentPos += FileHeader.HeadSize + FileHeader.PackSize + 2;
		fseek(InFile, CurrentPos, SEEK_SET);
		++ArchiveEntries;
		TotalLength += FileHeader.OrigSize;
		TotalBlocks += (FileHeader.OrigSize-1) / 254 + 1;
		TotalBlocksNow += (FileHeader.PackSize-1) / 254 + 1;
	};
	return 0;
}

/******************************************************************************
* Main program
******************************************************************************/
int main(int argc, char *argv[])
{
	int ArgNum;
	long StartPos;
	int DearcerBlocks;
	int Error = 0;
	int DispError;
	struct SDAHeaderNew FileHeaderNew;
	char FileName[MAXPATH+1];
	char LynxVer[10];
	struct SDAHeader Header;
	FILE *InFile;
	enum SDATypes SDAType;
	int VersionNum;

	setvbuf(stdout, NULL, _IOLBF, 82);		/* speed up screen output */
	if ((argc < 2) || ((argv[1][0] == '-') && (argv[1][1] == '?'))) {
		printf("%s  ver. " VERSION "  " VERDATE "  by Daniel Fandrich\n\n", ProgName);
		printf("Usage:\n   %s filename1[" DefaultExt
					"] [filename2[" DefaultExt
					"] [... filenameN[" DefaultExt "]]]\n"
			   "View directory of Commodore 64/128 self dissolving archive files.\n"
			   "Supports ARC230, Lynx & LHA (SFX) archive types.\n"
			   "Placed into the public domain by Daniel Fandrich.\n", ProgName);
		return 1;
	}

/******************************************************************************
* Loop through archive display for each file name
******************************************************************************/
	for (ArgNum=1; ArgNum<argc; ++ArgNum) {

/******************************************************************************
* Open the SDA file
******************************************************************************/
	strncpy(FileName, argv[ArgNum], sizeof(FileName) - sizeof(DefaultExt) - 1);
	printf("Archive: %s\n\n", FileName);
	if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {
		strcat(FileName, DefaultExt);
		if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {
			perror(ProgName);
			fclose(InFile);
			Error = 2;
			continue;		/* go do next file */
		}
	}

/******************************************************************************
* Read the SDA and determine which type it is
******************************************************************************/
	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		perror(ProgName);
		fclose(InFile);
		Error = 2;
		continue;		/* go do next file */
	}

	SDAType = UnknownSDA;
/******************************************************************************
* Is it a C64 format?
******************************************************************************/
	if (memcmp(Header.Type.C64_10.Magic1, MagicHeaderC64, sizeof(MagicHeaderC64)) == 0) {
		if (memcmp(Header.Type.C64_10.Magic2, MagicC64_10, sizeof(MagicC64_10)) == 0)
			SDAType = C64_10;
		if (memcmp(Header.Type.C64_13.Magic2, MagicC64_13, sizeof(MagicC64_13)) == 0)
			SDAType = C64_13;
		if (memcmp(Header.Type.C64_15.Magic2, MagicC64_15, sizeof(MagicC64_15)) == 0)
			SDAType = C64_15;

/******************************************************************************
* Is it a C128 format?
******************************************************************************/
	} else {
		if (memcmp(Header.Type.C128_15.Magic1, MagicHeaderC128, sizeof(MagicHeaderC128)) == 0) {
			if (Header.Type.C128_15.Magic2 == MagicC128_15)
				SDAType = C128_15;

/******************************************************************************
* Is it headerless ARC format?
******************************************************************************/
		} else {	/* This type does not have a dearcer built in, just the data */
			if ((BYTE) Header.Type.C64_ARC.Magic == MagicHeaderARC) {
				SDAType = C64__0;

/******************************************************************************
* Is it LHA SFX format?
******************************************************************************/
			} else {
				if (memcmp(Header.Type.LHA_SFX.Magic, MagicHeaderLHASFX, sizeof(MagicHeaderLHASFX)) == 0) {
					SDAType = LHA_SFX;

/******************************************************************************
* Is it headerless LHA format?
******************************************************************************/
				} else {
					if (memcmp(Header.Type.LHA.Magic, MagicHeaderLHA, sizeof(MagicHeaderLHA)) == 0) {
						SDAType = LHA;

/******************************************************************************
* Is it Lynx format?
******************************************************************************/
					} else {
						if (memcmp(Header.Type.Lynx.Magic, MagicHeaderLynx, sizeof(MagicHeaderLynx)) == 0) {
							SDAType = Lynx;
						} else {
							if (memcmp(Header.Type.LynxNew.Magic, MagicHeaderLynxNew, sizeof(MagicHeaderLynxNew)) == 0)
								SDAType = LynxNew;
						}
					}
				}
			}
		}
	}


/******************************************************************************
* Find the version number and first archive entry offset for each format
******************************************************************************/
	switch (SDAType) {
		case C64__0:	/* Not a self dearcer -- just the arc data */
			StartPos = 0L;
			VersionNum = 0;
			DearcerBlocks = 0;
			break;

		case C64_10:
			StartPos = ((Header.Type.C64_10.FirstOffH << 8) | Header.Type.C64_10.FirstOffL) - Header.Type.C64_10.StartAddress + 2;
			VersionNum = Header.Type.C64_10.Version;
			DearcerBlocks = (StartPos-1) / 254 + 1;
			break;

		case C64_13:
			StartPos = ((Header.Type.C64_13.FirstOffH << 8) | Header.Type.C64_13.FirstOffL) - Header.Type.C64_13.StartAddress + 2;
			VersionNum = Header.Type.C64_13.Version;
			DearcerBlocks = (StartPos-1) / 254 + 1;
			break;

		case C64_15:
			fseek(InFile, Header.Type.C64_15.StartPointer - Header.Type.C64_15.StartAddress + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			StartPos = ((FileHeaderNew.FirstOffH << 8) | FileHeaderNew.FirstOffL) - Header.Type.C64_15.StartAddress + 2;
			VersionNum = Header.Type.C64_15.Version;
			DearcerBlocks = (StartPos-1) / 254 + 1;
			break;

		case C128_15:
			fseek(InFile, Header.Type.C128_15.StartPointer - Header.Type.C128_15.StartAddress + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			StartPos = ((FileHeaderNew.FirstOffH << 8) | FileHeaderNew.FirstOffL) - Header.Type.C128_15.StartAddress + 2;
			VersionNum = Header.Type.C128_15.Version;
			DearcerBlocks = (StartPos-1) / 254 + 1;
			break;

		case LHA_SFX:
			StartPos = 0xe89;		/* Must be a better way than this */
			VersionNum = 0;
			DearcerBlocks = (StartPos-1) / 254 + 1;
			break;

		case LHA:
			StartPos = 0;
			VersionNum = 0;
			DearcerBlocks = 0;
			break;

		case Lynx:
			fseek(InFile, 0, SEEK_SET);
			fscanf(InFile, " %*s LYNX %s %*[^\015]", LynxVer);
			getc(InFile);				/* Get CR without killing whitespace */
			StartPos = ftell(InFile);
			VersionNum = 10 * RomanToDec(LynxVer);
			DearcerBlocks = 0;
			break;

		case LynxNew:
			fseek(InFile, Header.Type.LynxNew.EndHeaderAddr -
						Header.Type.LynxNew.StartAddress + 5, SEEK_SET);
			fscanf(InFile, " %*s *LYNX %s %*[^\015]", LynxVer);
			getc(InFile);				/* Get CR without killing whitespace */
			StartPos = ftell(InFile);
			VersionNum = 10 * RomanToDec(LynxVer);
			DearcerBlocks = 0;
			break;

		case UnknownSDA:
			printf("%s: Not a known Commodore SDA\n", ProgName);
			fclose(InFile);
			Error = 3;
			continue;		/* go do next file */
	}

/******************************************************************************
* Display the SDA contents
******************************************************************************/
	ArchiveEntries = 0;
	TotalBlocks = 0;
	TotalBlocksNow = 0;
	TotalLength = 0;

	printf("Name              Type  Length  Blks  Method     SF   Now   Check\n");
	printf("================  ====  ======  ====  ========  ====  ====  =====\n");

	switch (SDAType) {
		case C64__0:
		case C64_10:
		case C64_13:
		case C64_15:
		case C128_15:
			DispError = DisplayARC(InFile, StartPos);
			break;

		case LHA_SFX:
		case LHA:
			DispError = DisplayLHA(InFile, StartPos);
			break;

		case Lynx:
		case LynxNew:
			DispError = DisplayLynx(InFile, StartPos, (char) (VersionNum >= 100));
			break;
	}
	if (DispError)
		Error = DispError;
	fclose(InFile);

	printf("================  ====  ======  ====  ========  ====  ====  =====\n");
	printf("*total %5u           %7lu  %4d  %s%2u.%u %4d%%  %4d+%d\n",
		ArchiveEntries,
		TotalLength,
		TotalBlocks,
		SDAFormats[SDAType],
		VersionNum / 10,
		VersionNum - 10 * (VersionNum / 10),
		(unsigned) (100 - (TotalBlocksNow * 100L / (TotalBlocks))),
		TotalBlocksNow,
		DearcerBlocks
	);

/******************************************************************************
* Go do next file name on command line
******************************************************************************/
	 if (ArgNum<argc-1)
		printf("\n");
	}

	return Error;
}
