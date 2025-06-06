/*
 * cbmarcs.c
 *
 * Commodore archive formats directory display routines
 *
 * Compile this file with "pack structures" compiler flag if not GNU C
 *
 * fvcbm is copyright 1993-2025 Dan Fandrich, et. al.
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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "cbmarcs.h"

#if defined(__MSDOS__) || defined(_WIN32)
#include <io.h>
#endif

#if defined(MSC) || defined(SCO)
#define stricmp strcmp  /* this isn't equivalent, but it's good enough */
#elif defined(__GNUC__)
#define stricmp strcasecmp
#endif

#ifdef __GNUC__
#define PACK __attribute__ ((packed))	/* pack structures on byte boundaries */
#else
#define PACK		/* pack using a compiler switch instead */
#endif

#ifdef SUNOS
#include <unistd.h>
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef int bool;
#endif

#ifdef DEBUG
/* Enable low-level debug logs */
#define DEBUGLOG printf
#else
/* Disable low-level debug logs */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define DEBUGLOG(...) do { } while (0)
#elif defined(__GNUC__)
#define DEBUGLOG(x...) do { } while (0)
#else
#define DEBUGLOG (void)
#endif
#endif

extern char *ProgName;

/******************************************************************************
* Constants
******************************************************************************/
/* These descriptions must be in the order encountered in ArchiveTypes */
const char * const ArchiveFormats[] = {
/* C64_ARC */	" ARC",
/* C64_10 */	" C64",
/* C64_13 */	" C64",
/* C64_15 */	" C64",
/* C128_15 */	"C128",
/* LHA_SFX */	" LHA",
/* LHA */		" LHA",
/* Lynx */		"Lynx",
/* New Lynx */	"Lynx",
/* Tape image */" T64",
/* Disk image */" D64",
/* Disk image */"1581",
/* Disk image */" X64",
/* PRG file */	" P00",
/* SEQ file */	" S00",
/* USR file */	" U00",
/* REL file */	" R00",
/* DEL file */	" D00",
/* P00-like */	"P00?",
/* N64 file */	" N64",
/* LBR file */  " LBR",
/* TAP file */  " TAP"
};

/* File types as found on disk (bitwise AND code with CBM_TYPE) */
static const char * const CBMFileTypes[] = {
/* 0 */	"DEL",
/* 1 */	"SEQ",
/* 2 */ "PRG",
/* 3 */ "USR",
/* 4 */ "REL",
/* 5 */ "CBM",	/* 1581 partition type */
/* 6 */ "DJJ",	/* C65 file type */
/* 7 */ "FAB" 	/* C65 file type */
};

/* File type mask bits */
enum {
	CBM_DEL = 0,
	CBM_SEQ,
	CBM_PRG,
	CBM_USR,
	CBM_REL,
	CBM_CBM,
	CBM_DJJ,
	CBM_FAB,

	CBM_TYPE = 0x07,		/* Mask to get preceding 8 file types */
	CBM_CLOSED = 0x80,		/* Mask to get closed bit */
	CBM_LOCKED = 0x40		/* Mask to get locked bit */
};

/* End of 1541 filename character */
enum { CBM_END_NAME = '\xA0' };

/* 1571 double sided flag */
enum { FLAG_DOUBLE_SIDED = 0x80 };

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
* Returns the length of an open file in bytes
******************************************************************************/
#if defined(__Z88DK)
#include <unistd.h>

static long filelength(int handle)
{
    unsigned long l, old;
    old = lseek(handle, 0, SEEK_CUR);
    l = lseek(handle, 0, SEEK_END);
    lseek(handle, old, SEEK_SET);
    return l;
}

#elif !defined(__MSDOS__) && !defined(_WIN32)
#include <sys/stat.h>

static long filelength(int handle)
{
	struct stat statbuf;

	if (fstat(handle, &statbuf))
		return -1;
	return statbuf.st_size;
}
#endif



/******************************************************************************
* Return file type string given letter code
******************************************************************************/
static const char *FileTypes(char TypeCode)
{
	switch (toupper(TypeCode)) {
		case 'P': return "PRG";
		case 'S': return "SEQ";
		case 'U': return "USR";
		case 'R': return "REL";
		case 'D': return "DEL";
		case ' ': return "   ";
		default:  return "???";
	}
}

/******************************************************************************
* Convert CBM file names into ASCII strings
* Converts in place & returns pointer to start of string
******************************************************************************/
static char *ConvertCBMName(char *InString)
{
	char *LastNonBlank = InString;
	char *Ch;

	for (Ch = InString; *Ch; ++Ch)
		if (!isspace(*Ch = *Ch & 0x7F))	/* strip high bit */
			LastNonBlank = Ch;
	*++LastNonBlank = 0;		/* truncate string after last character */
	return InString;
}

/*---------------------------------------------------------------------------*/


/******************************************************************************
* ARC routines
******************************************************************************/
struct ArchiveHeaderNew {
	BYTE Filler1 PACK;
	BYTE FirstOffL PACK;
	BYTE Filler2[3] PACK;
	BYTE FirstOffH PACK;
};

struct ArchiveEntryHeader {
	BYTE Magic PACK;
	BYTE EntryType PACK;
	WORD Checksum PACK;
	WORD LengthL PACK;
	BYTE LengthH PACK;
	BYTE BlockLength PACK;
	BYTE Filler PACK;
	BYTE FileType PACK;
	BYTE FileNameLen PACK;
};

enum {MagicARCEntry = 2};

struct C64_10 {
	WORD StartAddress PACK;
	BYTE Filler1[2] PACK;
	WORD Version PACK;
	BYTE Magic1[10] PACK;

	BYTE Filler2 PACK;
	BYTE FirstOffL PACK;
	BYTE Magic2[3] PACK;
	BYTE FirstOffH PACK;
};

struct C64_13 {
	WORD StartAddress PACK;
	BYTE Filler1[2] PACK;
	WORD Version PACK;
	BYTE Magic1[10] PACK;

	BYTE Filler2[11] PACK;
	BYTE FirstOffL PACK;
	BYTE Magic2[3] PACK;
	BYTE FirstOffH PACK;
};

struct C64_15 {
	WORD StartAddress PACK;
	BYTE Filler1[2] PACK;
	WORD Version PACK;
	BYTE Magic1[10] PACK;

	BYTE Filler2[7] PACK;
	BYTE Magic2[4] PACK;
	WORD StartPointer PACK;
};

struct C128_15 {
	WORD StartAddress PACK;
	BYTE Filler1[2] PACK;
	WORD Version PACK;
	BYTE Magic1[10] PACK;

	BYTE Magic2 PACK;
	WORD StartPointer PACK;
};

struct C64_ARC {
	BYTE Magic PACK;
	BYTE EntryType PACK;
};

/* CBM ARC compression types */
static const char * const ARCEntryTypes[] = {
/* 0 */	"Stored",
/* 1 */ "Packed",
/* 2 */ "Squeezed",
/* 3 */ "Crunched",
/* 4 */ "Squashed",
/* 5 */ "?5",			/* future use */
/* 6 */ "?6",
/* 7 */ "?7"
};
enum {MaxARCEntry = 7};


static const BYTE MagicHeaderC64[10] = {0x9e,'(','2','0','6','3',')',0x00,0x00,0x00};
static const BYTE MagicHeaderC128[10] = {0x9e,'(','7','1','8','3',')',0x00,0x00,0x00};

/******************************************************************************
* Is archive C64 ARC format?
******************************************************************************/
static bool IsC64_10(FILE *InFile, const char *FileName)
{
	static const BYTE MagicC64_10[3] = {0x85,0xfd,0xa9};
	struct C64_10 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic1, MagicHeaderC64, sizeof(MagicHeaderC64)) == 0)
		&& (memcmp(Header.Magic2, MagicC64_10, sizeof(MagicC64_10)) == 0));
}

static bool IsC64_13(FILE *InFile, const char *FileName)
{
	static const BYTE MagicC64_13[3] = {0x85,0x2f,0xa9};
	struct C64_13 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic1, MagicHeaderC64, sizeof(MagicHeaderC64)) == 0)
		&& (memcmp(Header.Magic2, MagicC64_13, sizeof(MagicC64_13)) == 0));
}


static bool IsC64_15(FILE *InFile, const char *FileName)
{
	static const BYTE MagicC64_15[4] = {0x8d,0x21,0xd0,0x4c};
	struct C64_15 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic1, MagicHeaderC64, sizeof(MagicHeaderC64)) == 0)
		&& (memcmp(Header.Magic2, MagicC64_15, sizeof(MagicC64_15)) == 0));
}


static bool IsC128_15(FILE *InFile, const char *FileName)
{
	static const BYTE MagicC128_15 = 0x4c;
	struct C128_15 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic1, MagicHeaderC128, sizeof(MagicHeaderC128)) == 0)
		&& (Header.Magic2 == MagicC128_15));
}

static bool IsC64_ARC(FILE *InFile, const char *FileName)
{
	enum {MagicHeaderARC = 2};
	struct C64_ARC Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& ((BYTE) Header.Magic == MagicHeaderARC)
		&& ((BYTE) Header.EntryType <= MaxARCEntry));
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirARC(FILE *InFile, enum ArchiveTypes ArcType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	long CurrentPos;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

	if (fseek(InFile, 0, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}

/******************************************************************************
* Find the version number and first archive entry offset for each format
******************************************************************************/
	switch (ArcType) {
		case C64_ARC:	/* Not a self dearcer -- just the arc data */
			CurrentPos = 0L;
			break;

		case C64_10: {
			struct C64_10 Header;

			if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}

			CurrentPos = 1016;
/*
			CurrentPos = ((Header.FirstOffH << 8) |
							Header.FirstOffL) -
							CF_LE_W(Header.StartAddress) + 2;
*/
			Totals->Version = -CF_LE_W(Header.Version);
			Totals->DearcerBlocks = (int) ((CurrentPos-1) / 254 + 1);
		}
		break;

		case C64_13: {
			struct C64_13 Header;

			if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}

			CurrentPos = 1778;
/*
			CurrentPos = ((Header.FirstOffH << 8) |
							Header.FirstOffL) -
							CF_LE_W(Header.StartAddress) + 2;
*/
			Totals->Version = -CF_LE_W(Header.Version);
			Totals->DearcerBlocks = (int) ((CurrentPos-1) / 254 + 1);
		}
		break;

		case C64_15: {
			struct C64_15 Header;

			if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}

			CurrentPos = 2286;
/*
			fseek(InFile, CF_LE_W(Header.StartPointer) -
					CF_LE_W(Header.StartAddress) + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			CurrentPos = ((FileHeaderNew.FirstOffH << 8) |
							FileHeaderNew.FirstOffL) -
							CF_LE_W(Header.StartAddress) + 2;
*/
			Totals->Version = -CF_LE_W(Header.Version);
			Totals->DearcerBlocks = (int) ((CurrentPos-1) / 254 + 1);
		}
		break;

		case C128_15: {
			struct C128_15 Header;

			if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}

			CurrentPos = 2285;
/*
			fseek(InFile, CF_LE_W(Header.StartPointer) -
					CF_LE_W(Header.StartAddress) + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			CurrentPos = ((FileHeaderNew.FirstOffH << 8) |
							FileHeaderNew.FirstOffL) -
							CF_LE_W(Header.StartAddress) + 2;
*/
			Totals->Version = -CF_LE_W(Header.Version);
			Totals->DearcerBlocks = (int) ((CurrentPos-1) / 254 + 1);
		}
		break;

		default:
			return 2;		/* wrong archive type */
	}
/*printf("DirArc CurrentPos: %ld\n", CurrentPos);*/

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	DisplayStart(ArcType, NULL);

	while (1) {
		char EntryName[17];
		long FileLen;
		struct ArchiveEntryHeader FileHeader;
/*		struct ArchiveHeaderNew FileHeaderNew;*/

		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;
		if (FileHeader.Magic != MagicARCEntry)
			break;
		if (fread(&EntryName, FileHeader.FileNameLen, 1, InFile) != 1)
			break;
		EntryName[FileHeader.FileNameLen] = 0;

		FileLen = (long) (FileHeader.LengthH << 16L) | CF_LE_W(FileHeader.LengthL);
		DisplayEntry(
			ConvertCBMName(EntryName),
			FileTypes(FileHeader.FileType),
			(long) FileLen,
			(unsigned) ((FileLen-1) / 254 + 1),
			ARCEntryTypes[FileHeader.EntryType],
			(int) (100 - (FileHeader.BlockLength * 100L / (FileLen / 254 + 1))),
			(unsigned) FileHeader.BlockLength,
			(long) CF_LE_W(FileHeader.Checksum)
		);

		CurrentPos += FileHeader.BlockLength * 254;
		if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
			perror(ProgName);
			return 2;
		}
		++Totals->ArchiveEntries;
		Totals->TotalLength += FileLen;
		Totals->TotalBlocks += (int) ((FileLen-1) / 254 + 1);
		Totals->TotalBlocksNow += FileHeader.BlockLength;
	};
	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* Lynx reading routines
******************************************************************************/

/******************************************************************************
* Convert Roman numeral to decimal
* Only works for numerals up to 399
* Note: input string is cleared
******************************************************************************/
static int RomanToDec(char *Roman)
{
	int Last = -1;
	int Value = 0;

	while (*Roman) {
		int Digit;
		switch (toupper(*Roman)) {
			case 'I': Digit = 1; break;
			case 'V': Digit = 5; break;
			case 'X': Digit = 10; break;
			case 'L': Digit = 50; break;
			case 'C': Digit = 100; break;
			default:  Digit = 0; break;
		}
		Value = Last < Digit ? Digit - Value: Value + RomanToDec(Roman);
		Last = Digit;
		*Roman++ = 0;
	}
	return Value;
}

static const BYTE MagicHeaderLynx[10] = {' ','1',' ',' ',' ','L','Y','N','X',' '};
static const BYTE MagicHeaderLynxNew[25] = {0x97,'5','3','2','8','0',',','0',0x3A,
							   0x97,'5','3','2','8','1',',','0',0x3A,
							   0x97,'6','4','6',',',0xC2,0x28};

struct Lynx {
	BYTE Magic[sizeof(MagicHeaderLynx)] PACK;
};

struct LynxNew {
	WORD StartAddress PACK;
	WORD EndHeaderAddr PACK;
	WORD Version PACK;
	BYTE Magic[sizeof(MagicHeaderLynxNew)] PACK;
};

/******************************************************************************
* Is archive Lynx format?
******************************************************************************/
static bool IsLynx(FILE *InFile, const char *FileName)
{
	struct Lynx Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderLynx, sizeof(MagicHeaderLynx)) == 0));
}

static bool IsLynxNew(FILE *InFile, const char *FileName)
{
	struct LynxNew Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderLynxNew, sizeof(MagicHeaderLynxNew)) == 0));
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirLynx(FILE *InFile, enum ArchiveTypes LynxType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	int NumFiles;
	char LynxVer[10];
	char LynxName[16];
	int ExpectLastLength;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

/******************************************************************************
* Find the version number and first archive entry offset for each format
******************************************************************************/
	switch (LynxType) {
		case Lynx:
			if (fseek(InFile, 0, SEEK_SET) != 0) {
				perror(ProgName);
				return 2;
			}
			if (fscanf(InFile, " %*s LYNX %s %*[^\r]", LynxVer) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}
			getc(InFile);				/* Get CR without killing whitespace */
			Totals->Version = RomanToDec(LynxVer);
			Totals->DearcerBlocks = 0;
			ExpectLastLength = Totals->Version >= 10;
			break;

		case LynxNew:
/*			fseek(InFile, CF_LE_W(Header.Type.LynxNew.EndHeaderAddr) -
						CF_LE_W(Header.Type.LynxNew.StartAddress) + 5, SEEK_SET); */

			if (fseek(InFile, 0x5F, SEEK_SET) != 0) {
				perror(ProgName);
				return 2;
			}
			if (fscanf(InFile, " %*s *%15s %s %*[^\r]", LynxName, LynxVer) != 2) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}
			getc(InFile);				/* Get CR without killing whitespace */

			if (isupper(*LynxVer))
				Totals->Version = RomanToDec(LynxVer);	/* Lynx */
			else
				Totals->Version = atoi(LynxVer);		/* Ultra-Lynx */

			/* Only old versions of Lynx need this FALSE */
			/* Ultra-Lynx looks like it always has Version > 10 */
			ExpectLastLength = Totals->Version >= 10;

			Totals->DearcerBlocks = 0;
			break;

		default:
			return 2;		/* wrong archive type */
	}

	if (fscanf(InFile, "%d%*[^\r]\r", &NumFiles) != 1) {
		fprintf(stderr,"%s: Archive format error\n", ProgName);
		return 2;
	}
	DisplayStart(LynxType, NULL);

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	for (; NumFiles--;) {
		char EntryName[17];
		char FileType[2];
		int FileBlocks;
		long FileLen = 0;
		int ReadCount = fscanf(InFile, "%16[^\r]%*[^\r]", EntryName);
		(void) getc(InFile);	/* eat the CR here because Sun won't in scanf */
		ReadCount += fscanf(InFile, "%d%*[^\r]", &FileBlocks);
		(void) getc(InFile);
		ReadCount += fscanf(InFile, "%1s%*[^\r]", FileType);
		(void) getc(InFile);	/* eat the CR without killing whitespace so
						   ftell() will be correct, below */
		if (ReadCount != 3) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}

/******************************************************************************
* Find the exact length of the file.
* For the first n-1 entries (for all n entries in newer Lynx versions, like
*  XVII), the length in bytes of the last block of the file is specified.
* For the last entry in older Lynx versions, like IX, we must find out how many
*  bytes in the file and subtract everything not belonging to the last file.
*  This can give an incorrect result if the file has padding after the last
*  file (which would happen if the file was transferred using XMODEM), but
*  Lynx thinks the padding is part of the file, too.
* Should check for an error return from filelength()
******************************************************************************/
		if (NumFiles || ExpectLastLength) {
			int LastBlockSize = 0;
			if (fscanf(InFile, "%d%*[^\r]\r", &LastBlockSize) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}
			FileLen = (long) ((FileBlocks-1) * 254L + LastBlockSize - 1);
		} else				/* last entry -- calculate based on file size */
			FileLen = filelength(fileno(InFile)) - Totals->TotalBlocksNow * 254L -
							(((ftell(InFile) - 1) / 254) + 1) * 254L;

		DisplayEntry(
			ConvertCBMName(EntryName),
			FileTypes(FileType[0]),
			(long) FileLen,
			FileBlocks,
			"Stored",
			0,
			FileBlocks,
			-1L
		);

		++Totals->ArchiveEntries;
		Totals->TotalLength += FileLen;
		/* The following two values should equal */
		Totals->TotalBlocks += (int) ((FileLen-1) / 254 + 1);
		Totals->TotalBlocksNow += FileBlocks;
	};
	return 0;
}


/*---------------------------------------------------------------------------*/


/******************************************************************************
* LHA (SFX) reading routines
******************************************************************************/
struct LHAEntryHeader {
	BYTE HeadSize PACK;
	BYTE HeadChk PACK;
	BYTE HeadID[3] PACK;
	BYTE EntryType PACK;
	BYTE Magic PACK;
	LONG PackSize PACK;
	LONG OrigSize PACK;
#if 0  /* avoid ANSI warning because WORD != int */
	struct	{			/* DOS format time */
		WORD ft_tsec : 5;	/* Two second interval */
		WORD ft_min	 : 6;	/* Minutes */
		WORD ft_hour : 5;	/* Hours */
		WORD ft_day	 : 5;	/* Days */
		WORD ft_month: 4;	/* Months */
		WORD ft_year : 7;	/* Year */
	} FileTime PACK;
#else
	LONG DosTime PACK;
#endif
	WORD Attr PACK;
	BYTE FileNameLen PACK;
/*
 * File name is variable length and occurs at the end of the header, so it
 * can't be read as part of the header because with a short file name there
 * might not be enough file left to read. */
/*	BYTE FileName[64] PACK;  */
};

struct LHAEntryFileName {
	BYTE FileName[64] PACK;
/*
 *  Checksum is stored after the variable-length filename, but can't be part of
 *  the struct because it's variable length...
 *  BYTE ChecksumL PACK;
 *  BYTE ChecksumH PACK;
 */
};

/* Perform a compile-time check on the size of the struct to ensure that struct
 * element packing (with the PACK macro) is working correctly. If the compiler
 * complains in the following line with an error like: "invalid array size" or
 * "size of array ‘PackStructCompileCheck’ is negative" then fix the PACK macro
 * to perform structure packing for your compiler.
 * Instead of checking all the packed structs in the program, this one example
 * was chosen as a sentinel since it is particularly likely to show problems
 * due to its odd-byte alignment of LONG and WORD elements.  If the compiler
 * errors out on this line, struct alignment is not working on this compiler
 * and it must be fixed before the code will work.
 */
#ifndef __SCCZ80
typedef char PackStructCompileCheck[sizeof(struct LHAEntryHeader) == 22 ? 1 : -1];
#endif

/* LHA compression types */
static const char * const LHAEntryTypes[] = {
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

static const  BYTE MagicLHAEntry[3] = {'-','l','h'};

static const BYTE MagicHeaderLHASFX[10] = {0x97,0x32,0x30,0x2C,0x30,0x3A,0x8B,0xC2,0x28,0x32};
static const BYTE MagicHeaderLHA[3] = {'-','l','h'};

struct LHA_SFX {
	WORD StartAddress PACK;
	BYTE Filler[4] PACK;
	BYTE Magic[sizeof(MagicHeaderLHASFX)] PACK;
};

struct LHA {
	BYTE Filler[2] PACK;
	BYTE Magic[sizeof(MagicHeaderLHA)] PACK;
};

/******************************************************************************
* Is archive LHA format?
******************************************************************************/
static bool IsLHA_SFX(FILE *InFile, const char *FileName)
{
	struct LHA_SFX Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderLHASFX, sizeof(MagicHeaderLHASFX)) == 0));
}

static bool IsLHA(FILE *InFile, const char *FileName)
{
	struct LHA Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderLHA, sizeof(MagicHeaderLHA)) == 0));
}


/******************************************************************************
* Read directory
******************************************************************************/
static int DirLHA(FILE *InFile, enum ArchiveTypes LHAType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	long CurrentPos;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;

/******************************************************************************
* Find the version number and first archive entry offset for each format
******************************************************************************/
	switch (LHAType) {
		case LHA_SFX:
			CurrentPos = 0xE89;		/* Must be a better way than this */
			Totals->Version = 0;
			Totals->DearcerBlocks = (int) ((CurrentPos-1) / 254 + 1);
			break;

		case LHA:
			CurrentPos = 0;
			Totals->Version = 0;
			Totals->DearcerBlocks = 0;
			break;

		default:
			return 2;		/* wrong archive type */
	}

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	DisplayStart(LHAType, NULL);

	while (1) {
		struct LHAEntryHeader FileHeader;
		struct LHAEntryFileName EntryFileName;
		char FileName[80];  /* must be > sizeof(EntryFileName) */

		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;
		if (memcmp(FileHeader.HeadID, MagicLHAEntry, sizeof(MagicLHAEntry)) != 0)
			break;
		/* 2-byte checksum is stored as part of the filename but not counted here */
		if (FileHeader.FileNameLen > sizeof(EntryFileName.FileName)-2)
			break;  /* exceeds limit; probably corrupt */
		if (fread(&EntryFileName, FileHeader.FileNameLen+2, 1, InFile) != 1)
			break;

		memcpy(FileName, EntryFileName.FileName, FileHeader.FileNameLen);
		FileName[min(sizeof(FileName)-1, FileHeader.FileNameLen)] = 0;
		DisplayEntry(
			ConvertCBMName(FileName),
			FileTypes(EntryFileName.FileName[FileHeader.FileNameLen-2] ? ' ' : EntryFileName.FileName[FileHeader.FileNameLen-1]),
			(long) CF_LE_L(FileHeader.OrigSize),
			CF_LE_L(FileHeader.OrigSize) ? (unsigned) ((CF_LE_L(FileHeader.OrigSize)-1) / 254 + 1) : 0,
			LHAEntryTypes[FileHeader.EntryType - '0'],
			CF_LE_L(FileHeader.OrigSize) ? (int) (100 - (CF_LE_L(FileHeader.PackSize) * 100L / CF_LE_L(FileHeader.OrigSize))) : 100,
			CF_LE_L(FileHeader.PackSize) ? (unsigned) ((CF_LE_L(FileHeader.PackSize)-1) / 254 + 1) : 0,
			(long) (unsigned) (EntryFileName.FileName[FileHeader.FileNameLen+1] << 8) | EntryFileName.FileName[FileHeader.FileNameLen]
		);

		CurrentPos += FileHeader.HeadSize + CF_LE_L(FileHeader.PackSize) + 2;
		fseek(InFile, CurrentPos, SEEK_SET);
		++Totals->ArchiveEntries;
		Totals->TotalLength += CF_LE_L(FileHeader.OrigSize);
		Totals->TotalBlocks += (int) ((CF_LE_L(FileHeader.OrigSize)-1) / 254 + 1);
		Totals->TotalBlocksNow += (int) ((CF_LE_L(FileHeader.PackSize)-1) / 254 + 1);
	};
	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* T64 reading routines
******************************************************************************/
struct T64Header {
	BYTE Magic[32] PACK;
	BYTE MinorVersion PACK;
	BYTE MajorVersion PACK;
	WORD Entries PACK;		/* Room for this many files in tape directory */
	WORD Used PACK;			/* Number of files in archive (sometimes just 0) */
	WORD unused PACK;
	BYTE TapeName[24] PACK;
};

struct T64EntryHeader {
	BYTE EntryType PACK;
	BYTE FileType PACK;
	WORD StartAddr PACK;
	WORD EndAddr PACK;
	WORD unused1 PACK;
	LONG FileOffset PACK;
	LONG unused2 PACK;
	BYTE FileName[16] PACK;
};

/* File types in T64 tape archives */
/* At least one archive writing program uses the 1541 file types */
static const char * const T64FileTypes[] = {
/* 0 */	"SEQ",
/* 1 */ "PRG",
/* 2 */ "?2?",
/* 3 */ "?3?",
/* 4 */ "?4?",
/* 5 */ "?5?",
/* 6 */ "?6?",
/* 7 */ "?7?"
};

struct T64 {
	BYTE Magic[20] PACK;  /* room for terminating NUL */
};

/******************************************************************************
* Is archive T64 format?
******************************************************************************/
static bool IsT64(FILE *InFile, const char *FileName)
{
	struct T64 Header;
	(void) FileName;

	rewind(InFile);

	if (fread(&Header, sizeof(Header.Magic) - 1, 1, InFile) != 1)
		return 0;

	/* Zero terminate just in case */
	Header.Magic[sizeof(Header.Magic) - 1] = '\0';

	/* Common headers are 'C64 tape image file' and 'C64S tape file' but
	 * lots of variants exist. The FAQ suggests the following check:
	 */
	return (strstr((char *) Header.Magic, "C64") != NULL &&
			strstr((char *) Header.Magic, "tape") != NULL);
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirT64(FILE *InFile, enum ArchiveTypes ArchiveType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	char TapeName[25];
	int NumFiles;
	struct T64Header Header;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

	if (fseek(InFile, 0, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		fprintf(stderr,"%s: Archive format error\n", ProgName);
		return 2;
	}
	memcpy(TapeName, Header.TapeName, sizeof(TapeName)-1);
	TapeName[sizeof(TapeName)-1] = '\0';
	ConvertCBMName(TapeName);
	DisplayStart(ArchiveType, TapeName);

	Totals->Version = -(Header.MajorVersion * 10 + Header.MinorVersion);
	Totals->ArchiveEntries = CF_LE_W(Header.Used);

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	for (NumFiles = CF_LE_W(Header.Used); NumFiles; --NumFiles) {
		struct T64EntryHeader FileHeader;
		char FileName[17];
		unsigned FileLength;

		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;

		memcpy(FileName, FileHeader.FileName, 16);
		FileName[16] = 0;
		FileLength = CF_LE_W(FileHeader.EndAddr) - CF_LE_W(FileHeader.StartAddr) + 2;
		DisplayEntry(
			ConvertCBMName(FileName),
			FileHeader.FileType & CBM_CLOSED ?
				CBMFileTypes[FileHeader.FileType & CBM_TYPE] :
				T64FileTypes[FileHeader.FileType],
			(long) FileLength,
			(unsigned) (FileLength / 254 + 1),
			"Stored",
			0,
			(unsigned) (FileLength / 254 + 1),
			(long) -1L
		);

		Totals->TotalLength += FileLength;
		Totals->TotalBlocks += (int) (FileLength / 254 + 1);
	}

	Totals->TotalBlocksNow = Totals->TotalBlocks;
	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* D64/X64 reading routines
******************************************************************************/
struct X64Header {
	BYTE Magic[4] PACK;
	BYTE MajorVersion PACK;
	BYTE MinorVersion PACK;
	BYTE DeviceType PACK;
	BYTE MaxTracks PACK;	/* versions >= 1.2 only */
	BYTE reserved[24] PACK;
	BYTE DiskName[32] PACK;	/* null-terminated disk name */
};

/* Also applies to 1571 */
struct Raw1541DiskHeader {
	BYTE FirstTrack PACK;
	BYTE FirstSector PACK;
	BYTE Format PACK;
	BYTE Flag PACK;			/* double sided flag */
	BYTE BAM[140] PACK;
	BYTE DiskName[16] PACK;
	BYTE Filler1[2] PACK;	/* this should probably be Filler1[5] for 1571 */
	BYTE DOSVersion PACK;
	BYTE DOSFormat PACK;
	BYTE Filler2[4] PACK;
  /*BYTE Filler3[85] PACK;*/
};

/* Also applies to 8050 */
struct Raw8250DiskHeader {
	BYTE FirstTrack PACK;
	BYTE FirstSector PACK;
	BYTE Format PACK;
	BYTE Filler1[3] PACK;
	BYTE DiskName[17] PACK;
	BYTE Filler2 PACK;
	BYTE DiskID[2] PACK;
	BYTE Filler3 PACK;
	BYTE DOSVersion PACK;
	BYTE DOSFormat PACK;
};

struct Raw1581DiskHeader {
	BYTE FirstTrack PACK;
	BYTE FirstSector PACK;
	BYTE Format PACK;
	BYTE Flag PACK;
	BYTE DiskName[16] PACK;
	BYTE Filler1[2] PACK;
	BYTE DiskID[2] PACK;
	BYTE Filler2 PACK;
	BYTE DOSVersion PACK;
	BYTE DOSFormat PACK;
	BYTE Filler3[2] PACK;
};

struct D64EntryHeader {
	BYTE FileType PACK;
	BYTE FirstTrack PACK;
	BYTE FirstSector PACK;
	BYTE FileName[16] PACK;
	BYTE FirstSideTrack PACK;
	BYTE FirstSideSector PACK;
	BYTE RecordSize PACK;
	BYTE Filler1[4] PACK;
	BYTE FirstReplacementTrack PACK;
	BYTE FirstReplacementSector PACK;
	WORD FileBlocks PACK;
	WORD Filler2 PACK;
};

enum { D64_ENTRIES_PER_BLOCK = 8 };

struct D64DirBlock {
	BYTE NextTrack PACK;
	BYTE NextSector PACK;
	struct D64EntryHeader Entry[D64_ENTRIES_PER_BLOCK] PACK;
};

struct D64DataBlock {
	BYTE NextTrack PACK;
	BYTE NextSector PACK;
/*	BYTE Data[]; */		/* GNU C doesn't like this line, but we don't need it */
};

#define D64_EXTENSION ".d64"	/* magic file extension for 1541 raw disk images */
#define D71_EXTENSION ".d71"	/* magic file extension for 1571 raw disk images */
#define D81_EXTENSION ".d81"	/* magic file extension for 1581 raw disk images */
#define D80_EXTENSION ".d80"	/* magic file extension for 8050 raw disk images */
#define D82_EXTENSION ".d82"	/* magic file extension for 8250 raw disk images */

/* The following possible x64 disk types are from x64's serial.h (ver.0.3.1) */
/* Disk Drives */

#define DT_1540                  0
#define DT_1541                  1      /* Default */
#define DT_1542                  2
#define DT_1551                  3

#define DT_1570                  4
#define DT_1571                  5
#define DT_1572                  6

#define DT_1581                  8

#define DT_2031                 16
#define DT_2040                 17
#define DT_2041                 18
#define DT_3040                 17      /* Same as 2040 */
#define DT_4031                 16      /* Same as 2031 */

/* #define DT_2081                      */

#define DT_4040                 24

#define DT_8050                 32
#define DT_8060                 33
#define DT_8061                 34

#define DT_SFD1001              48
#define DT_8250                 49
#define DT_8280                 50

enum {BYTES_PER_SECTOR=256};	/* logical bytes per sector on Commodore disks */

/******************************************************************************
* Return disk image offset for 1541 disk
******************************************************************************/
static unsigned long Location1541TS(unsigned char Track, unsigned char Sector)
{
	static const WORD Sectors[42] = {
	/* Tracks number	Offset in sectors of start of track */
	/* tracks 1-17 */	0,21,42,63,84,105,126,147,168,189,
						210,231,252,273,294,315,336,
	/* tracks 18-24 */	357,376,395,414,433,452,471,
	/* tracks 25-30 */	490,508,526,544,562,580,
	/* tracks 31-35 */	598,615,632,649,666,
	/* The rest of the tracks are nonstandard */
	/* tracks 36-42 */	683,700,717,734,751,768,785
	};

	return (Sectors[Track-1] + Sector) * (unsigned long) BYTES_PER_SECTOR;
}
#define MAX_CAPACITY_1541 802  /* disk capacity in blocks, with extra tracks  */

/******************************************************************************
* Return disk image offset for 1571 disk
******************************************************************************/
static unsigned long Location1571TS(unsigned char Track, unsigned char Sector)
{
	static const WORD Sectors[70] = {
	/* Tracks number	Offset in sectors of start of track */
	/* tracks 1-17 */	0,21,42,63,84,105,126,147,168,189,
						210,231,252,273,294,315,336,
	/* tracks 18-24 */	357,376,395,414,433,452,471,
	/* tracks 25-30 */	490,508,526,544,562,580,
	/* tracks 31-35 */	598,615,632,649,666,
	/* The remaining tracks are on the second side of the disk */
	/* tracks 36-52 */	683,704,725,746,767,788,809,830,851,872,893,914,935,
						956,977,998,1019,
	/* tracks 53-59 */	1040,1059,1078,1097,1116,1135,1154,
	/* tracks 60-65 */	1173,1191,1209,1227,1245,1263,
	/* tracks 66-70 */	1281,1298,1315,1332,1349
	};

	return (Sectors[Track-1] + Sector) * (unsigned long) BYTES_PER_SECTOR;
}
#define MAX_CAPACITY_1571 1366  /* disk capacity in blocks */

/******************************************************************************
* Return disk image offset for 8250 disk
* The 8050 mapping is a subset of this, so this function can be also used there
******************************************************************************/
static unsigned long Location8250TS(unsigned char Track, unsigned char Sector)
{
	static const WORD Sectors[154] = {
	/* Tracks number	Offset in sectors of start of track */
	/* tracks 1-39 */	0,29,58,87,116,145,174,203,232,261,290,319,348,377,406,
						435,464,493,522,551,580,609,638,667,696,725,754,783,
						812,841,870,899,928,957,986,1015,1044,1073,1102,
	/* tracks 40-53 */	1131,1158,1185,1212,1239,1266,1293,1320,1347,1374,1401,
						1428,1455,1482,
	/* tracks 54-64 */	1509,1534,1559,1584,1609,1634,1659,1684,1709,1734,1759,
	/* tracks 65-77 */	1784,1807,1830,1853,1876,1899,1922,1945,1968,1991,2014,
						2037,2060,
	/* The remaining tracks are only valid on the 8250 */
	/* tracks 78-116 */	 2083,2112,2141,2170,2199,2228,2257,2286,2315,2344,
						 2373,2402,2431,2460,2489,2518,2547,2576,2605,2634,
						 2663,2692,2721,2750,2779,2808,2837,2866,2895,2924,
						 2953,2982,3011,3040,3069,3098,3127,3156,3185,
	/* tracks 117-130 */ 3214,3241,3268,3295,3322,3349,3376,3403,3430,3457,
						 3484,3511,3538,3565,
	/* tracks 131-141 */ 3592,3617,3642,3667,3692,3717,3742,3767,3792,3817,
						 3842,
	/* tracks 142-154 */ 3867,3890,3913,3936,3959,3982,4005,4028,4051,4074,
						 4097,4120,4143
	};

	return (Sectors[Track-1] + Sector) * (unsigned long) BYTES_PER_SECTOR;
}
#define MAX_CAPACITY_8250 4166  /* disk capacity in blocks */

/******************************************************************************
* Return disk image offset for 1581 disk
******************************************************************************/
static unsigned long Location1581TS(unsigned char Track, unsigned char Sector)
{
	enum {SECTORS_PER_TRACK_1581=40};

	return ((Track-1) * SECTORS_PER_TRACK_1581 + Sector) *
			(unsigned long) BYTES_PER_SECTOR;
}
#define MAX_CAPACITY_1581 3200  /* disk capacity in blocks */

/******************************************************************************
* Follow chain of file sectors in disk image, counting total bytes in the file
******************************************************************************/
static unsigned long CountCBMBytes(FILE *DiskImage, int Type,
		unsigned long Offset, unsigned char FirstTrack,
		unsigned char FirstSector)
{
	struct D64DataBlock DataBlock;
	unsigned int BlockCount = 0, MaxBlocks;

	DataBlock.NextTrack = FirstTrack;		/* prime the track & sector */
	DataBlock.NextSector = FirstSector;
	/* Use the disk capacity as a fail-safe for the largest file that can exist
	 * on the disk. It's slightly larger than the actual value, but it's only
	 * used to detect a track/sector chain loop. */
	if (Type == 1581)
		MaxBlocks = MAX_CAPACITY_1581;
	else if (Type == 8250)
		MaxBlocks = MAX_CAPACITY_8250;
	else if (Type == 1571)
		MaxBlocks = MAX_CAPACITY_1571;
	else /* if (Type == 1541) */
		MaxBlocks = MAX_CAPACITY_1541;
	do {
		long SectorOfs;
		if (Type == 1581)
			SectorOfs = Location1581TS(DataBlock.NextTrack, DataBlock.NextSector);
		else if (Type == 8250)
			SectorOfs = Location8250TS(DataBlock.NextTrack, DataBlock.NextSector);
		else if (Type == 1571)
			SectorOfs = Location1571TS(DataBlock.NextTrack, DataBlock.NextSector);
		else /* if (Type == 1541) */
			SectorOfs = Location1541TS( DataBlock.NextTrack, DataBlock.NextSector);
		if ((fseek(DiskImage, SectorOfs + Offset, SEEK_SET) != 0) ||
			(fread(&DataBlock, sizeof(DataBlock), 1, DiskImage) != 1)) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 0;  /* no better way to indicate error */
		}
		++BlockCount;
		if (BlockCount > MaxBlocks) {
			/* We found a loop in the track/sector chain */
			fprintf(stderr,"%s: File chain loop detected\n", ProgName);
			return 0;  /* no better way to indicate error */
		}
	} while (DataBlock.NextTrack > 0);

	return (BlockCount - 1) * 254L + DataBlock.NextSector - 1;
}


/******************************************************************************
* Returns nonzero if the given sector contains a valid 1541 header block
******************************************************************************/
static int is_1541_header(struct Raw1541DiskHeader *header) {
	return (header->Format == 'A') &&
			/* Flag is for double sided, but some images have '*' there */
			((header->Flag == 0) || (header->Flag == '*')) &&
			(header->Filler2[3] == (BYTE) CBM_END_NAME);
}

/******************************************************************************
* Returns nonzero if the given sector contains a valid 1571 header block
******************************************************************************/
static int is_1571_header(struct Raw1541DiskHeader *header) {
	return (header->Format == 'A') &&
			/* Flag is marked as reserved, but some images have '*' there */
			(header->Flag == FLAG_DOUBLE_SIDED) &&
			(header->Filler2[3] == (BYTE) CBM_END_NAME);
}

/******************************************************************************
* Returns nonzero if the given sector contains a valid 1581 header block
******************************************************************************/
static int is_1581_header(struct Raw1581DiskHeader *header) {
	return (header->Format == 'D') &&
				(header->Flag == 0) &&
				(header->Filler3[0] == (BYTE) CBM_END_NAME) &&
				(header->Filler3[1] == (BYTE) CBM_END_NAME);
}

/******************************************************************************
* Returns nonzero if the given sector contains a valid 8250/8050 header block
******************************************************************************/
static int is_8250_header(struct Raw8250DiskHeader *header) {
	return (header->Format == 'C') && (header->DOSFormat == 'C') &&
		   (header->FirstTrack == 38);
}

static const BYTE MagicHeaderD64[3] = {'C','B','M'};
static const BYTE MagicHeaderImage1[2] = {0x00, 0xff};	/* blank sector */
static const BYTE MagicHeaderImage2[2] = {0x00, 0x00};	/* even blanker sector */
static const BYTE MagicHeaderImage3[2] = {0x01, 0x0a};	/* track 0, sector 1 chain (1541) */
static const BYTE MagicHeaderImage4[2] = {0x01, 0x06};	/* track 0, sector 1 chain (1571) */
static const BYTE MagicHeaderImage5[2] = {0x01, 0x03};	/* track 0, sector 1 chain (1571) */
static const BYTE MagicHeaderImage6[2] = {0x01, 0x01};	/* track 0, sector 1 chain (1581) */
static const BYTE MagicHeaderImage7[2] = {0x01, 0x03};	/* track 0, sector 1 chain (1581 partition) */

struct D64 {
	BYTE Magic[3] PACK;
};

static const BYTE MagicHeaderX64[4] = {0x43,0x15,0x41,0x64};

struct X64 {
	BYTE Magic[sizeof(MagicHeaderX64)] PACK;
};

/******************************************************************************
* Is archive disk image format?
******************************************************************************/
static bool IsX64(FILE *InFile, const char *FileName)
{
	struct X64 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderX64, sizeof(MagicHeaderX64)) == 0));
}

/******************************************************************************
* Check for a D64 archive by first looking at the file extension
* It appears the only good way to detect a D64 archive from its contents is to
*  go look at "track 18, sector 0" (or "track 40, sector 0" for 1581 images)
* Here, we just try a bunch of likely values for the contents of track 1,
*  sector 0, but we could go to tracks 18 and 40 (& 39 & others) instead
******************************************************************************/
static bool IsD64(FILE *InFile, const char *FileName)
{
	char *NameExt;
	struct D64 Header;

	rewind(InFile);
	return ((FileName && (NameExt = strrchr(FileName, '.')) != 0
			&& (!stricmp(NameExt, D64_EXTENSION) || !stricmp(NameExt, D80_EXTENSION) ||
				!stricmp(NameExt, D71_EXTENSION) || !stricmp(NameExt, D82_EXTENSION) ||
				!stricmp(NameExt, D81_EXTENSION)))
		|| ((fread(&Header, sizeof(Header), 1, InFile) == 1)
			&& ((memcmp(Header.Magic, MagicHeaderD64, sizeof(MagicHeaderD64)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage1, sizeof(MagicHeaderImage1)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage2, sizeof(MagicHeaderImage2)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage3, sizeof(MagicHeaderImage3)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage4, sizeof(MagicHeaderImage4)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage5, sizeof(MagicHeaderImage5)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage6, sizeof(MagicHeaderImage6)) == 0)
			||  (memcmp(Header.Magic, MagicHeaderImage7, sizeof(MagicHeaderImage7)) == 0))));
}

/******************************************************************************
* Can't tell a raw 1581 image yet
******************************************************************************/
static bool IsC1581(FILE *InFile, const char *FileName)
{
	(void) InFile;
	(void) FileName;
	return 0;
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirD64(FILE *InFile, enum ArchiveTypes D64Type,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	char DiskLabel[24];  /* Holds the disk label plus filler, version and format */
	long CurrentPos;
	unsigned long HeaderOffset;
	int DiskType = 0;			/* type of disk image--1541, 1581, 8250; 0=unknown */
	struct D64DirBlock DirBlock;
	struct X64Header Header;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

/******************************************************************************
* Find the version number and header size for each format
******************************************************************************/
	switch (D64Type) {
		case D64:
			HeaderOffset = 0;			/* No header on D64 images */
			DiskType = 0;				/* Might be 1541 or 1581 */
			break;

		case X64:
			HeaderOffset = 0x40;		/* X64 header takes 64 bytes */
			if (fseek(InFile, 0, SEEK_SET) != 0) {
				perror(ProgName);
				return 2;
			}
			if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
				fprintf(stderr,"%s: Archive format error\n", ProgName);
				return 2;
			}
			switch (Header.DeviceType) {
				case DT_2031:
				/*case DT_4031:*/
				case DT_2040:
				/*case DT_3040:*/
				case DT_2041:
				case DT_4040:
				case DT_1551:
				case DT_1570:
				case DT_1542:
				case DT_1540:
				case DT_1541:	DiskType = 1541; break;

				case DT_1572:
				case DT_1571:	DiskType = 1571; break;

				case DT_1581:	DiskType = 1581; break;

				case DT_8050:   /* This is treated as a 8250 */
				case DT_SFD1001:
				case DT_8250:	DiskType = 8250; break;

				default:
					fprintf(stderr,"%s: Unsupported X64 disk image type (#%d)\n",
						ProgName, Header.DeviceType);
					return 3;
			}

			Totals->Version = -(Header.MajorVersion * 10 +
								((Header.MinorVersion >= 10) ?
										Header.MinorVersion / 10 :
										Header.MinorVersion));
			break;

		default:
			return 2;		/* wrong archive type */
	}

/******************************************************************************
* Read the disk directory header block and determine the disk type
* Detect these formats in the order of disk capacity, smallest to largest
******************************************************************************/
	/* 1541 capacity: 683 blocks */
	if ((DiskType == 1541) || !DiskType) {
		struct Raw1541DiskHeader DirHeader1541;
		CurrentPos = Location1541TS(18,0) + HeaderOffset;
		if ((fseek(InFile, CurrentPos, SEEK_SET) != 0) ||
			(fread(&DirHeader1541, sizeof(DirHeader1541), 1, InFile) != 1)) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}
		DirBlock.NextTrack = DirHeader1541.FirstTrack;
		DirBlock.NextSector = DirHeader1541.FirstSector;
		if (!is_1541_header(&DirHeader1541)) {
			if (DiskType == 1541)	/* only mark good or bad if we know the type */
				DiskType = -1;		/* Bad archive */
		} else
		{
			DiskType = 1541;		/* Good archive */

			memcpy(DiskLabel, DirHeader1541.DiskName, sizeof(DiskLabel)-1);
		}
	}

	/* 1571 capacity: 1366 blocks */
	if ((DiskType == 1571) || !DiskType) {
		struct Raw1541DiskHeader DirHeader1541;
		CurrentPos = Location1571TS(18,0) + HeaderOffset;
		if ((fseek(InFile, CurrentPos, SEEK_SET) != 0) ||
			(fread(&DirHeader1541, sizeof(DirHeader1541), 1, InFile) != 1)) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}
		DirBlock.NextTrack = DirHeader1541.FirstTrack;
		DirBlock.NextSector = DirHeader1541.FirstSector;
		if (!is_1571_header(&DirHeader1541)) {
			if (DiskType == 1571)	/* only mark good or bad if we know the type */
				DiskType = -1;		/* Bad archive */
		} else
		{
			DiskType = 1571;		/* Good archive */

			memcpy(DiskLabel, DirHeader1541.DiskName, sizeof(DiskLabel)-1);
		}
	}

	/* 8050 capacity: 2083 blocks */
	/* 8250 capacity: 4166 blocks */
	if ((DiskType == 8250) || !DiskType) {
		struct Raw8250DiskHeader DirHeader8250;
		CurrentPos = Location8250TS(39,0) + HeaderOffset;
		if ((fseek(InFile, CurrentPos, SEEK_SET) != 0) ||
			(fread(&DirHeader8250, sizeof(DirHeader8250), 1, InFile) != 1)) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}
		/* DirHeader8250.FirstTrack/Sector points to the BAM, not directory */
		DirBlock.NextTrack = 39;
		DirBlock.NextSector = 1;
		if (!is_8250_header(&DirHeader8250)) {
			if (DiskType == 8250)	/* only mark good or bad if we know the type */
				DiskType = -1;		/* Bad archive */
		} else
		{
			DiskType = 8250;		/* Good archive */

			memcpy(DiskLabel, DirHeader8250.DiskName, sizeof(DiskLabel)-1);
		}
	}

	/* 1581 capacity: 3200 blocks */
	if ((DiskType == 1581) || !DiskType) {
		struct Raw1581DiskHeader DirHeader1581;
		CurrentPos = Location1581TS(40,0) + HeaderOffset;
		if ((fseek(InFile, CurrentPos, SEEK_SET) != 0) ||
			(fread(&DirHeader1581, sizeof(DirHeader1581), 1, InFile) != 1)) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}
		DirBlock.NextTrack = DirHeader1581.FirstTrack;
		DirBlock.NextSector = DirHeader1581.FirstSector;
		if (!is_1581_header(&DirHeader1581))
			DiskType = -1;		/* Bad archive */
		else
      {
			DiskType = 1581;	/* Good archive */

			memcpy(DiskLabel, DirHeader1581.DiskName, sizeof(DiskLabel)-1);
		}
	}


	if (DiskType == -1) {
		fprintf(stderr,"%s: Unsupported disk image format\n",
			ProgName);
		return 3;
	}

	/* Display the diskette label, terminate for safety's sake */
	DiskLabel[sizeof(DiskLabel)-1] = '\0';
	ConvertCBMName(DiskLabel);
	DisplayStart(D64Type, DiskLabel);

/******************************************************************************
* Go through the entire directory
******************************************************************************/
	while (DirBlock.NextTrack > 0) {
		int EntryCount;
		CurrentPos = HeaderOffset;
		if (DiskType == 1581)
			CurrentPos += Location1581TS(DirBlock.NextTrack, DirBlock.NextSector);
		else if (DiskType == 8250)
			CurrentPos += Location8250TS(DirBlock.NextTrack, DirBlock.NextSector);
		else if (DiskType == 1571)
			CurrentPos += Location1571TS( DirBlock.NextTrack, DirBlock.NextSector);
		else /* if (DiskType == 1541) */
			CurrentPos += Location1541TS( DirBlock.NextTrack, DirBlock.NextSector);
		if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
			perror(ProgName);
			return 2;
		}
		if (fread(&DirBlock, sizeof(DirBlock), 1, InFile) != 1) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}

		/* Look at each entry in the block */
		for (EntryCount=0; EntryCount < D64_ENTRIES_PER_BLOCK; ++EntryCount) {
			BYTE FileType = DirBlock.Entry[EntryCount].FileType;
			if ((FileType & CBM_CLOSED) != 0) {
				char FileName[17];
				char *EndName;
				long FileLength;

				if ((FileType & CBM_TYPE) == CBM_CBM)
					/* Can't follow track & sector links for a 1581 partition */
					FileLength = 256 *	/* not 254 because whole partition is data */
								CF_LE_W(DirBlock.Entry[EntryCount].FileBlocks);
				else
				{
					/* Save some time if we're not wide */
					if (WideFormat)
					{
						/* Don't walk the file chain for a zero-length file */
						if (CF_LE_W(DirBlock.Entry[EntryCount].FileBlocks))
							FileLength = CountCBMBytes(
											InFile,
											DiskType,
											HeaderOffset,
											DirBlock.Entry[EntryCount].FirstTrack,
											DirBlock.Entry[EntryCount].FirstSector
										 );
						else
							FileLength = 0;
					}
					else
						/* We could approximate based on blocks, but this will
						 * be completely ignored, so don't bother */
						FileLength = 0;
				}

				strncpy(FileName, (char *) DirBlock.Entry[EntryCount].FileName, sizeof(FileName)-1);
				FileName[sizeof(FileName)-1] = 0;
				if ((EndName = strchr(FileName, CBM_END_NAME)) != NULL)
					*EndName = 0;

				DisplayEntry(
					ConvertCBMName(FileName),
					CBMFileTypes[DirBlock.Entry[EntryCount].FileType & CBM_TYPE],
					FileLength,
					CF_LE_W(DirBlock.Entry[EntryCount].FileBlocks),
					"Stored",
					0,
					CF_LE_W(DirBlock.Entry[EntryCount].FileBlocks),
					(long) -1L
				);
				Totals->TotalLength += FileLength;
				Totals->TotalBlocks += CF_LE_W(DirBlock.Entry[EntryCount].FileBlocks);
				++Totals->ArchiveEntries;
			}

		}
	}

	Totals->TotalBlocksNow = Totals->TotalBlocks;
	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* P00,S00,U00,R00,D00 reading routines
******************************************************************************/
struct X00 {
	BYTE Magic[8] PACK;
	BYTE FileName[17] PACK;
	BYTE RecordSize PACK;		/* REL file record size */
};

static const BYTE MagicHeaderP00[8] = {'C','6','4','F','i','l','e',0};

/******************************************************************************
* Is archive x00 format?
* X00 must be checked after the other _00 types because it is more lenient
******************************************************************************/
static bool IsX00(FILE *InFile, const char *FileName)
{
	struct X00 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderP00, sizeof(MagicHeaderP00)) == 0));
}

static bool IsX00Ext(FILE *InFile, const char *FileName, char Ext)
{
	char *NameExt;

	return (IsX00(InFile, FileName)
		&& (FileName != NULL) && ((NameExt = strrchr(FileName, '.')) != NULL)
		&& (toupper(*++NameExt) == Ext));
}

static bool IsP00(FILE *InFile, const char *FileName)
{
	return IsX00Ext(InFile, FileName, 'P');
}

static bool IsS00(FILE *InFile, const char *FileName)
{
	return IsX00Ext(InFile, FileName, 'S');
}

static bool IsU00(FILE *InFile, const char *FileName)
{
	return IsX00Ext(InFile, FileName, 'U');
}

static bool IsD00(FILE *InFile, const char *FileName)
{
	return IsX00Ext(InFile, FileName, 'D');
}

static bool IsR00(FILE *InFile, const char *FileName)
{
	struct X00 Header;
	char *NameExt;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderP00, sizeof(MagicHeaderP00)) == 0)
		&& (FileName != NULL)
		&& ((NameExt = strrchr(FileName, '.')) != NULL)
		&& ((toupper(*++NameExt) == 'R') || (Header.RecordSize > 0)));
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirP00(FILE *InFile, enum ArchiveTypes ArchiveType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	long FileLength;
	char FileName[17];
	struct X00 Header;
	const char *FileType;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

/******************************************************************************
* P00 is just a regular file with a simple header prepended, so just read the
* header and display the name
******************************************************************************/
	if (fseek(InFile, 0, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		fprintf(stderr,"%s: Archive format error\n", ProgName);
		return 2;
	}
	DisplayStart(ArchiveType, NULL);
	FileLength = filelength(fileno(InFile)) - sizeof(Header);
	strncpy(FileName, (char *) Header.FileName, sizeof(FileName)-1);
	FileName[sizeof(FileName)-1] = 0;		/* never need this on a good P00 file */

	/* If archive type is unknown, see if file is REL */
/*
	if ((ArchiveType == X00) && (Header.RecordSize > 0))
		ArchiveType = R00;
*/

	switch (ArchiveType) {
		case S00: FileType = "SEQ"; break;
		case P00: FileType = "PRG"; break;
		case U00: FileType = "USR"; break;
		case R00: FileType = "REL"; break;
		case D00: FileType = "DEL"; break;
		case X00:
		default:  FileType = "???"; break;
	}

	DisplayEntry(
		ConvertCBMName(FileName),
		FileType,
		(long) FileLength,
		(unsigned) (FileLength / 254 + 1),
		"Stored",
		0,
		(unsigned) (FileLength / 254 + 1),
		(long) -1
	);

	Totals->ArchiveEntries = 1;
	Totals->TotalLength = FileLength;
	Totals->TotalBlocks = Totals->TotalBlocksNow = (int) (FileLength / 254 + 1);
	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* N64 reading routines
******************************************************************************/
struct N64Header {
	BYTE FileType PACK;
	WORD LoadAddr PACK;
	LONG FileLength PACK;
	BYTE NetSecurityLevel PACK;
	BYTE Reserved1[19] PACK;
	BYTE FileName[16] PACK;
/*	BYTE Reserved2[209] PACK; */
};

static const BYTE MagicHeaderN64[3] = {'C','6','4'};

struct N64 {
	BYTE Magic[sizeof(MagicHeaderN64)] PACK;
	BYTE Version PACK;
};

/******************************************************************************
* Is archive N64 format?
* This N64 check must come last because several other formats use a similar,
*  but longer, magic number.
******************************************************************************/
static bool IsN64(FILE *InFile, const char *FileName)
{
	enum {MagicHeaderN64Version = 1};
	struct N64 Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderN64, sizeof(MagicHeaderN64)) == 0)
		&& (Header.Version == MagicHeaderN64Version));
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirN64(FILE *InFile, enum ArchiveTypes ArchiveType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	long FileLength;
	char FileName[17];
	struct N64Header Header;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

/******************************************************************************
* N64 is just a regular file with a simple header prepended, so just read the
* header and display the name
******************************************************************************/
	if (fseek(InFile, 4, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		fprintf(stderr,"%s: Archive format error\n", ProgName);
		return 2;
	}
	DisplayStart(ArchiveType, NULL);

	strncpy(FileName, (char *) Header.FileName, sizeof(FileName)-1);
	FileName[sizeof(FileName)-1] = 0;

	FileLength = CF_LE_L(Header.FileLength);

	DisplayEntry(
		ConvertCBMName(FileName),
		CBMFileTypes[Header.FileType & CBM_TYPE],
		(long) FileLength,
		(unsigned) (FileLength / 254 + 1),
		(const char *) "Stored",
		0,
		(unsigned) (FileLength / 254 + 1),
		(long) -1
	);

	Totals->ArchiveEntries = 1;
	Totals->TotalLength = FileLength;
	Totals->TotalBlocks = Totals->TotalBlocksNow = (int) (FileLength / 254 + 1);
	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* LBR reading routines
* This is NOT the CP/M type of LBR archive
******************************************************************************/

static const BYTE MagicHeaderLBR[3] = {'D','W','B'};

struct LBR {
	BYTE Magic[sizeof(MagicHeaderLBR)] PACK;
};

/******************************************************************************
* Is archive LBR format?
******************************************************************************/
static bool IsLBR(FILE *InFile, const char *FileName)
{
	struct LBR Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderLBR, sizeof(MagicHeaderLBR)) == 0));
}

/******************************************************************************
* Read directory
******************************************************************************/
static int DirLBR(FILE *InFile, enum ArchiveTypes LBRType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	int NumFiles;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

/******************************************************************************
* Get the number of files in the archive
******************************************************************************/
	if (fseek(InFile, 3, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}

	if (fscanf(InFile, " %d%*[^\r]\r", &NumFiles) != 1) {
		fprintf(stderr,"%s: Archive format error\n", ProgName);
		return 2;
	}
	DisplayStart(LBRType, NULL);

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	for (; NumFiles--;) {
		char EntryName[17];
		char FileType[2];
		long FileLen;
		int ReadCount = fscanf(InFile, "%16[^\r]%*[^\r]", EntryName);
		(void) getc(InFile);	/* eat the CR here because Sun won't in scanf */
		ReadCount += fscanf(InFile, "%1s%*[^\r]", FileType);
		(void) getc(InFile);
		ReadCount += fscanf(InFile, " %ld%*[^\r]", &FileLen);
		(void) getc(InFile);	/* eat the CR without killing whitespace so
						   ftell() will be correct, below */
		if (ReadCount != 3) {
			fprintf(stderr,"%s: Archive format error\n", ProgName);
			return 2;
		}

		DisplayEntry(
			ConvertCBMName(EntryName),
			FileTypes(FileType[0]),
			(long) FileLen,
			(unsigned) ((FileLen-1) / 254 + 1),
			"Stored",
			0,
			(unsigned) ((FileLen-1) / 254 + 1),
			-1L
		);

		++Totals->ArchiveEntries;
		Totals->TotalLength += FileLen;
		Totals->TotalBlocks += (int) ((FileLen-1) / 254 + 1);
	};

	Totals->TotalBlocksNow = Totals->TotalBlocks;
	return 0;
}


/*---------------------------------------------------------------------------*/


/******************************************************************************
* TAP reading routines
* This is a low-level format that records the magnetic fluctuations on cassette
* tape. It takes multiple levels of decoding to make sense of.
* One feature that Commodore built-in to the format is that everything is
* written to tape twice, so that if one copy is bad the other one can be used.
* This feature is not used here so this decoder is more susceptible to noisy
* inputs than it could be.
******************************************************************************/

struct TAPHeader {
	char Magic[12] PACK;
	unsigned char Version PACK;
	unsigned char Platform PACK;
	unsigned char Video PACK;
	char Reserved PACK;
	LONG Size PACK;
};
static const BYTE MagicHeaderTAP[12] =
	{'C', '6', '4', '-', 'T', 'A', 'P', 'E', '-', 'R', 'A', 'W'};

/******************************************************************************
* Is archive TAP format?
******************************************************************************/
static bool IsTAP(FILE *InFile, const char *FileName)
{
	struct TAPHeader Header;
	(void) FileName;

	rewind(InFile);
	return ((fread(&Header, sizeof(Header), 1, InFile) == 1)
		&& (memcmp(Header.Magic, MagicHeaderTAP, sizeof(MagicHeaderTAP)) == 0));
}

/* States in the TAP flux reversal decoding state machine */
enum TapState {
	SYNCSEARCH,
	BYTESEARCH,
	BYTELONG,
	GETBIT0,
	GETBITS,
	GETBITL
};

/* Minimum durations
 * These are for PAL and should probably be tweaked for Video == NTSC
 * but it's close enough that it should work anyway.
 */
enum TapDuration {
	TAP_SHORT_DUR = 0x24,
	TAP_LONG_DUR = 0x37,
	TAP_MARK_DUR = 0x4a,
	TAP_INVALID_DUR = 0x65
};

/* Minimum number of short sync pulses to detect.
 * 60 are written but we will accept fewer.
 */
enum { SyncLen = 30 };

enum TapSignal {
	TAP_SHORT,
	TAP_LONG,
	TAP_MARK,
	TAP_INVALID
};

struct TapeHeader {
	unsigned char Countdown[9] PACK;
	char HeaderType PACK;
	WORD StartAddr PACK;
	WORD EndAddr PACK;
	unsigned char FileName[16] PACK;
	unsigned char FileName2[171] PACK;
	unsigned char CheckSum PACK;
};
#define TAPE_HEADER_LEN ((unsigned)sizeof(struct TapeHeader))
/* Minimum header size; Countdown + HeaderType + Checksum */
enum {MinHeaderSize = 11};
static const unsigned char Countdown1[9] =
	{0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81};
static const unsigned char Countdown2[9] =
	{0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

static enum TapSignal SignalDuration(LONG Duration) {
	if(Duration < TAP_SHORT_DUR)
		return TAP_INVALID;
	if (Duration < TAP_LONG_DUR)
		return TAP_SHORT;
	if (Duration < TAP_MARK_DUR)
		return TAP_LONG;
	if (Duration < TAP_INVALID_DUR)
		return TAP_MARK;
	return TAP_INVALID;
}

enum HeaderDataState {
	AwaitingHeader,
	AwaitingData
};

enum HeaderTypes {
	HeaderTypeReloc = 1,
	HeaderTypeSeqData = 2,
	HeaderTypeNonReloc = 3,
	HeaderTypeSeqHead = 4,
	HeaderTypeEndOfTape = 5
};

static const char* TapeType(enum HeaderTypes HeaderType)
{
	switch (HeaderType) {
		case HeaderTypeReloc:      /* relocatable (BASIC) program */
		case HeaderTypeNonReloc:   /* non-relocatable (M/L) program */
			return "PRG";
		case HeaderTypeSeqData:    /* data */
		case HeaderTypeSeqHead:    /* header */
			return "SEQ";
		case HeaderTypeEndOfTape:  /* end-of-tape (EOT) */
		default:                   /* anything else */
			return "???";
	}
}

/* Read a duration value from the TAP file.
 * The value is capped to 255 since we don't care if it's longer than that
 * and it makes the code faster. This is called around 50 times per byte of
 * decoded data, so it needs to be fast.
 * Returns 0 on EOF
 * Version is the TAP file version (0,1)
 * Nread is a pointer to the number of bytes read in (0,1,4) (not accurate on
 * EOF)
 */
static BYTE TapReadDuration(FILE *f, int Version, int *Nread)
{
	int d1, d2, d3, Duration;
	int ch = getc(f);
	if (ch == EOF)
		return 0;
	*Nread = 1;
	if(ch != 0)
		return ch;

	/* Value 0 is a special case but should happen infrequently in normal TAP files */
	if (Version == 0)
		/* 0 means 256 in Version 0, but we cap it to 255 */
		return 255;

	/* For Version==1, 0 means read a 24 bit extended value */
	d1 = getc(f);
	d2 = getc(f);
	d3 = getc(f);
	if (d3 == EOF)
		return 0;
	*Nread += 3;

	/* This long value is in cycles & must be converted */
	if (d3 || d2 >= 8)
		/* Value is definitely at least 256 so cap it */
		return 255;

	/* These remaining cases will probably never be found in a real normal TAP
	 * file because it's an inefficient way to represent these low numbers.
	 * This calculation will never result in a value >255 because larger ones
	 * are handled above.
	 */
	Duration = (d1 | d2 << 8) >> 3;
	if (Duration)
		return Duration;

	/* Don't return 0 because that means EOF */
	return 1;
}

/* Count the number of bits in a byte */
static int CountBits(int Byte)
{
  static const char BitsLookup[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
  return BitsLookup[Byte & 0x0F] + BitsLookup[Byte >> 4];
}

/* Calculate the checksum on a tape block
 * A valid calculated checksum will be 0.
 */
static BYTE TapeChecksum(BYTE *Buf, int Len)
{
	BYTE Csum;
	for (Csum=0; Len; --Len, ++Buf)
		Csum ^= *Buf;
	return Csum;
}

/* Validates that the tape header is correct.
 * Returns 0 on success, nonzero on failure
 */
static int CheckTapeHeader(struct TapeHeader *Header, int Len, int Which)
{
	return memcmp(&Header->Countdown, Which ? Countdown2 : Countdown1, sizeof(Countdown1)) ||
	   TapeChecksum((BYTE *)&Header->HeaderType, Len - sizeof(Countdown1));
}

static int DirTAP(FILE *InFile, enum ArchiveTypes ArchiveType,
		struct ArcTotals *Totals, DisplayStartFunc DisplayStart,
		DisplayEntryFunc DisplayEntry)
{
	struct TAPHeader FileHeader;
	LONG Flen;
	enum HeaderDataState HeadDataState = AwaitingHeader;
	char FileName[17];
	int DelayedFile = 0;
	LONG DelayedFileLen = 0;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

	if (fseek(InFile, 0, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1) {
		fprintf(stderr,"%s: Archive format error\n", ProgName);
		return 2;
	}
	if (FileHeader.Version != 0 && FileHeader.Version != 1) {
		fprintf(stderr,"%s: TAP version %d is unsupported\n", ProgName, FileHeader.Version);
		return 2;
	}
	DisplayStart(ArchiveType, NULL);

	DEBUGLOG("File version %d\n", (int) FileHeader.Version);
	DEBUGLOG("%ld bytes long\n", (long) CF_LE_L(FileHeader.Size));
	DEBUGLOG("%d platform\n", (int) FileHeader.Platform);
	DEBUGLOG("%d video\n", (int) FileHeader.Video);
	Flen = CF_LE_L(FileHeader.Size);
	Totals->Version = FileHeader.Version;

	/* Loop looking for header and data blocks */
	while (Flen > 0) {
		unsigned Bufidx = 0;
		unsigned char Buffer[TAPE_HEADER_LEN * 2]; /* Buffer for both copies of header/data */
		enum TapState State = SYNCSEARCH;
		int GotCopy = 0;
		int Databyte = 0;
		int Bitnum = 0;
		DEBUGLOG("Now reading %s\n", HeadDataState == AwaitingHeader ? "header" : "data");

		/* Loop to read two duplicate blocks to then interpet */
		for (; Flen > 0 &&
			   (HeadDataState == AwaitingData || Bufidx < sizeof(Buffer)) &&
			   GotCopy < 2;) {
			int BytesRead;
			enum TapSignal Signal;
			BYTE Duration = TapReadDuration(InFile, FileHeader.Version, &BytesRead);
			if(Duration == 0) {
				DEBUGLOG("FLEN %ld\n", (long)Flen);
				fprintf(stderr, "Error: corrupt file (too short)\n");
				return 2;
			}
			Flen -= BytesRead;
			Signal = SignalDuration(Duration);
			if(Signal == TAP_INVALID) {
				DEBUGLOG("Warning: too long/short pulse: %d\n", Duration);
			}

			switch (State) {
				case SYNCSEARCH:
					if(Signal == TAP_SHORT) {
						++Bitnum;
						if(Bitnum > SyncLen) {
							/* We found a sync header; now look for the first byte */
							State = BYTESEARCH;
							Bitnum = 0;
						}
					} else
						/* Start counting over */
						Bitnum = 0;
					break;

				case BYTESEARCH:
					if(Signal == TAP_MARK)
						/* Probably the start of a byte */
						State = BYTELONG;
					break;

				case BYTELONG:
					if(Signal == TAP_LONG) {
						/* It is indeed a start of byte signal */
						State = GETBIT0;
						Bitnum = 0;
						Databyte = 0;
					} else if(Signal == TAP_SHORT) {
						/* Between the two copies of the tape header, there are 60 shorts.
						 * Go to the start and wait for the first byte of the next header.
						 * After we read two copies (in header mode) we can examine them */
						++GotCopy;
						State = SYNCSEARCH;
					} else {
						fprintf(stderr, "Error: data decoding error %d @%d\n", Signal, Bufidx);
						return 2;
						/* State = BYTESEARCH; */  /* To attempt to go on, switch states */
					}
					break;

				case GETBIT0:
					/* Get the first of two bit transitions (long or short) */
					if(Signal == TAP_SHORT)
						State = GETBITL;
					else if(Signal == TAP_LONG)
						State = GETBITS;
					else {
						fprintf(stderr, "Error: data decoding error %d @%d\n", Signal, Bufidx);
						return 2;
						/* State = BYTESEARCH; */  /* To attempt to go on, switch states */
					}
					break;

				case GETBITL:
					/* Get the second of two bit transitions (a long) */
					/* This should be a zero bit */
					if(Signal == TAP_LONG) {
						/* This is a zero bit */
						++Bitnum;
						if (Bitnum == 9) {
							if (!(CountBits(Databyte) & 1)) {
								fprintf(stderr, "Error: bad parity\n");
								/* TODO: continue and hope the second header is uncorrupted */
								return 2;
							}
							if(HeadDataState == AwaitingHeader)
								/* Only save header data */
								Buffer[Bufidx] = (unsigned char) Databyte;
							else
								DEBUGLOG("%02x ", (int) Databyte);
							++Bufidx;
							State = BYTESEARCH;
						} else {
							Databyte >>= 1;
							State = GETBIT0;
						}
					} else {
						fprintf(stderr, "Error: data decoding error %d @%d\n", Signal, Bufidx);
						return 2;
						/* State = BYTESEARCH; */  /* To attempt to go on, switch states */
					}
					break;

				case GETBITS:
					/* Get the second of two bit transitions (a short) */
					/* This should be a one bit */
					if(Signal == TAP_SHORT) {
						/* This is a one bit */
						++Bitnum;
						if (Bitnum == 9) {
							if (CountBits(Databyte) & 1) {
								fprintf(stderr, "Error: bad parity\n");
								/* TODO: continue and hope the second header is uncorrupted */
								return 2;
							}
							if(HeadDataState == AwaitingHeader)
								/* Only save header data */
								Buffer[Bufidx] = (unsigned char) Databyte;
							else
								DEBUGLOG("%02x ", (int) Databyte);
							++Bufidx;
							State = BYTESEARCH;
						} else {
							Databyte >>= 1;
							Databyte |= 0x80;
							State = GETBIT0;
						}
					} else {
						fprintf(stderr, "Error: data decoding error %d @%d\n", Signal, Bufidx);
						return 2;
						/* State = BYTESEARCH; */  /* To attempt to go on, switch states */
					}
					break;
			}
		}

		/* We have read two copies of a header or data block. Now examine them.
		 * Skip checking if there is no data; probably EOF
		 */
		if(Bufidx) {
			if(HeadDataState == AwaitingHeader) {
				if(Bufidx < 2*MinHeaderSize) {
					/* Something went wrong */
					fprintf(stderr, "Error: corrupted data; minimum data underflow (%d < %d)\n", Bufidx, 2*MinHeaderSize);
					return 2;
				} else {
					/* Point to the first copy of the header block for now */
					struct TapeHeader *GoodHeader = (struct TapeHeader *) Buffer;

					DEBUGLOG("HeaderType %d %s\n", GoodHeader->HeaderType, TapeType((enum HeaderTypes) GoodHeader->HeaderType));

					/* Match the countdown bytes and validate the checksum. */
					if (CheckTapeHeader(GoodHeader, Bufidx/2, 0)) {
						/* Main header is bad; try the backup header instead */
						DEBUGLOG("First header bad; trying second\n");
						GoodHeader = (struct TapeHeader *) (Buffer + Bufidx/2);
						if (CheckTapeHeader(GoodHeader, Bufidx/2, 1)) {
							fprintf(stderr, "Error: Bad header\n");
							return 2;
						}
					}

					if(DelayedFile && GoodHeader->HeaderType != HeaderTypeSeqData) {
						/* A previous SEQ file is now finished & the size is known */
						++Totals->ArchiveEntries;
						Totals->TotalBlocks += (int) (DelayedFileLen / 254 + 1);
						Totals->TotalBlocksNow = Totals->TotalBlocks;
						Totals->TotalLength += DelayedFileLen;
						DisplayEntry(
							ConvertCBMName(FileName),
							TapeType(HeaderTypeSeqHead),
							(long) DelayedFileLen,
							(unsigned) (DelayedFileLen / 254 + 1),
							"Stored",
							0,
							(unsigned) (DelayedFileLen / 254 + 1),
							-1L
						);
						DelayedFile = 0;
					}

					if(GoodHeader->HeaderType == HeaderTypeEndOfTape)
						/* End of tape; stop looking */
						break;

					/* HeaderTypeSeqData contains only HeaderType and the rest is data */
					if(GoodHeader->HeaderType == HeaderTypeSeqData) {
						DEBUGLOG("Another %ld bytes of SEQ data\n", Bufidx/2 - sizeof(Countdown1) - 1);
						/* Don't count Counter, HeaderType or Checksum in the length */
						DelayedFileLen += Bufidx/2 - sizeof(Countdown1) - 2;

					} else {
						LONG Len;
						int i;
						if(Bufidx != 2*TAPE_HEADER_LEN) {
							fprintf(stderr, "Error: data underflow (%d < %u)\n", Bufidx, 2*TAPE_HEADER_LEN);
							return 2;
						}
						DEBUGLOG("StartAddr %d\n", (int) CF_LE_W(GoodHeader->StartAddr));
						DEBUGLOG("EndAddr %d\n", (int) CF_LE_W(GoodHeader->EndAddr));
						/* Programs in TAP appear 2 bytes smaller than the same program on disc
						 * because the two byte load address is not included in the byte count
						 * as it is on disc.
						 */
						Len = (LONG) ((CF_LE_W(GoodHeader->EndAddr) < CF_LE_W(GoodHeader->StartAddr) ?
							   (LONG)65536 : 0) + CF_LE_W(GoodHeader->EndAddr) - CF_LE_W(GoodHeader->StartAddr));
						DEBUGLOG("Len %d\n", Len);

						/* This header type is entirely data after the HeaderType byte */
						for (i=0; i < 16; ++i)
							/* Strip off control characters, which some tapes use (e.g. fast loaders) */
							FileName[i] = GoodHeader->FileName[i] < 0x80 && GoodHeader->FileName[i] >= 0x20 ? GoodHeader->FileName[i] : ' ';
						FileName[sizeof(FileName)-1] = 0;
						DEBUGLOG("Tape name: %s\n", FileName);

						/* To calculate the length for SEQ files we need to loop over all its data
						 * blocks and add up their lengths. The start/end addresses in the header
						 * don't hold the size since that is not known at the beginning.  An
						 * ambiguity can occur because ASCII data is NUL-terminated and a non-full
						 * block will be padded with space characters, but valid binary data could
						 * also have embedded NUL characters. Since we don't know if the block holds
						 * ASCII or binary data, and since these bytes are actually written to the
						 * tape and use actual space, we count the whole block as part of the file
						 * size.
						 * Delay generation of a SEQ file until the size can be calculated */
						if(GoodHeader->HeaderType == HeaderTypeSeqHead) {
							DelayedFile = 1;
							DelayedFileLen = 0;
						} else {
							++Totals->ArchiveEntries;
							Totals->TotalBlocks += (int) (Len / 254 + 1);
							Totals->TotalBlocksNow = Totals->TotalBlocks;
							Totals->TotalLength += Len;
							DisplayEntry(
								ConvertCBMName(FileName),
								TapeType((enum HeaderTypes) GoodHeader->HeaderType),
								(long) Len,
								(unsigned) (Len / 254 + 1),
								"Stored",
								0,
								(unsigned) (Len / 254 + 1),
								-1L
							);
						}
					}

					if(GoodHeader->HeaderType == HeaderTypeSeqHead || GoodHeader->HeaderType == HeaderTypeSeqData) {
						/* After a SEQ block always comes another header block */
						HeadDataState = AwaitingHeader;
					} else
						/* Next comes a data block */
						HeadDataState = AwaitingData;
				}

			} else /* AwaitingData */ {
				/* Awaiting a data block */
				/* The data wasn't saved in this state so there's nothing to do */
				DEBUGLOG("Skipping over %u bytes of data\n", Bufidx);

				/* Next comes another header block */
				HeadDataState = AwaitingHeader;
			}
		}
		DEBUGLOG("\n");
	}

	if(DelayedFile) {
		/* A previous SEQ file is now finished & the size is known */
		++Totals->ArchiveEntries;
		Totals->TotalBlocks += (int) (DelayedFileLen / 254 + 1);
		Totals->TotalBlocksNow = Totals->TotalBlocks;
		Totals->TotalLength += DelayedFileLen;
		DisplayEntry(
			ConvertCBMName(FileName),
			TapeType(HeaderTypeSeqHead),
			(long) DelayedFileLen,
			(unsigned) (DelayedFileLen / 254 + 1),
			"Stored",
			0,
			(unsigned) (DelayedFileLen / 254 + 1),
			-1L
		);
		DelayedFile = 0;
	}

	return 0;
}



/*---------------------------------------------------------------------------*/


/******************************************************************************
* Array of functions to determine archive types
******************************************************************************/
bool (* const TestFunctions[])(FILE *, const char *) = {
	IsC64_ARC,
	IsC64_10,
	IsC64_13,
	IsC64_15,
	IsC128_15,
	IsLHA_SFX,
	IsLHA,
	IsLynx,
	IsLynxNew,
	IsT64,
	IsD64,
	IsC1581,
	IsX64,
	IsP00,
	IsS00,
	IsU00,
	IsR00,
	IsD00,
	IsX00,		/* must come after the other _00 entries */
	IsN64,		/* must come after most other entries */
	IsLBR,
	IsTAP,

	NULL		/* last entry must be NULL--corresponds with UnknownArchive */
};

/******************************************************************************
* Read the archive and determine which type it is
* File is already open; name is used for P00 etc. type detection
******************************************************************************/
enum ArchiveTypes DetermineArchiveType(FILE *InFile, const char *FileName)
{
	enum ArchiveTypes ArchiveType;

	for (ArchiveType = 0; TestFunctions[ArchiveType] != NULL; ++ArchiveType)
		if ((*TestFunctions[ArchiveType])(InFile, FileName))
			break;

	return ArchiveType;
}

/******************************************************************************
* Array of functions to read archive directories
******************************************************************************/
int (* const DirFunctions[])(FILE *, enum ArchiveTypes, struct ArcTotals *,
	DisplayStartFunc, DisplayEntryFunc) = {
/* C64_ARC */	DirARC,
/* C64_10 */ 	DirARC,
/* C64_13 */ 	DirARC,
/* C64_15 */ 	DirARC,
/* C128_15 */	DirARC,
/* LHA_SFX */	DirLHA,
/* LHA */		DirLHA,
/* Lynx */		DirLynx,
/* LynxNew */	DirLynx,
/* T64 */		DirT64,
/* D64 */		DirD64,
/* C1581 */		DirD64,
/* X64 */		DirD64,
/* P00 */		DirP00,
/* S00 */		DirP00,
/* U00 */		DirP00,
/* R00 */		DirP00,
/* D00 */		DirP00,
/* X00 */		DirP00,
/* N64 */		DirN64,
/* LBR */		DirLBR,
/* TAP */		DirTAP
};

/******************************************************************************
* Read and display the archive directory
******************************************************************************/
int DirArchive(FILE *InFile, enum ArchiveTypes ArchiveType,
		struct ArcTotals *Totals,
		DisplayStartFunc DisplayStart, DisplayEntryFunc DisplayEntry)
{
	if (ArchiveType >= UnknownArchive)
		return 3;

	return DirFunctions[ArchiveType](InFile, ArchiveType, Totals,
									 DisplayStart, DisplayEntry);
}
