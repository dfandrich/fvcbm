/*
 * cbmarcs.c
 *
 * Commodore archive formats directory display routines
 *
 * Compile this file with "pack structures" compiler flag if not GNU C
 */

#ifdef __MSDOS__
#include <io.h>
#else
extern long filelength(int);
#endif
#include <string.h>
#include <ctype.h>
#include "cbmarcs.h"


typedef unsigned char BYTE;		/* 8 bits */
typedef unsigned short WORD;	/* 16 bits */
typedef unsigned long LONG;		/* 32 bits */

#ifdef __GNUC__
#define PACK __attribute__ ((packed))	/* pack structures on byte boundaries */
#else
#define PACK
#endif

extern char *ProgName;

/******************************************************************************
* Constants
******************************************************************************/

/* These archive format signatures are somewhat of a kludge */

static BYTE MagicHeaderC64[10] = {0x9e,'(','2','0','6','3',')',0x00,0x00,0x00};
static BYTE MagicHeaderC128[10] = {0x9e,'(','7','1','8','3',')',0x00,0x00,0x00};
static BYTE MagicHeaderARC = 2;
static BYTE MagicHeaderLHASFX[10] = {0x97,0x32,0x30,0x2C,0x30,0x3A,0x8B,0xC2,0x28,0x32};
static BYTE MagicHeaderLHA[3] = {'-','l','h'};
static BYTE MagicHeaderLynx[10] = {' ','1',' ',' ',' ','L','Y','N','X',' '};
static BYTE MagicHeaderLynxNew[27] = {0x0A,0x00,0x97,'5','3','2','8','0',',','0',0x3A,
							   0x97,'5','3','2','8','1',',','0',0x3A,
							   0x97,'6','4','6',',',0xC2,0x28};
static BYTE MagicHeaderT64[19] = {'C','6','4',' ','t','a','p','e',' ',
						   'i','m','a','g','e',' ','f','i','l','e'};
static BYTE MagicHeaderD64[3] = {'C','B','M'};
static BYTE MagicHeaderX64[6] = {0x43,0x15,0x41,0x64,0x01,0x01};

static BYTE MagicC64_10[3] = {0x85,0xfd,0xa9};
static BYTE MagicC64_13[3] = {0x85,0x2f,0xa9};
static BYTE MagicC64_15[4] = {0x8d,0x21,0xd0,0x4c};
static BYTE MagicC128_15 = 0x4c;

enum {MagicARCEntry = 2};

static BYTE MagicLHAEntry[3] = {'-','l','h'};

/* These descriptions must be in the order encountered in ArchiveTypes */
char *ArchiveFormats[] = {
/* C64__0 */	" ARC",
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
/* Disk image */" X64"
};

/* CBM ARC compression types */
static char *ARCEntryTypes[] = {
/* 0 */	"Stored",
/* 1 */ "Packed",
/* 2 */ "Squeezed",
/* 3 */ "Crunched",
/* 4 */ "Squashed",
/* 5 */ "?5",			/* future use */
/* 6 */ "?6",
/* 7 */ "?7",
/* 8 */ "?8"
};

/* LHA compression types */
static char *LHAEntryTypes[] = {
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

/* File types in T64 tape archives */
static char *T64FileTypes[] = {
/* 0 */	"SEQ",
/* 1 */ "PRG",
/* 2 */ "?2?",
/* 3 */ "?3?",
/* 4 */ "?4?",
/* 5 */ "?5?",
/* 6 */ "?6?",
/* 7 */ "?7?"
};

/* File types as found on disk (bitwise AND code with with CBM_TYPE) */
static char *CBMFileTypes[] = {
/* 0 */	"DEL",
/* 1 */	"SEQ",
/* 2 */ "PRG",
/* 3 */ "USR",
/* 4 */ "REL",

/* 5 */ "?5?",	/* should never see the rest */
/* 6 */ "?6?",
/* 7 */ "?7?"
};

/* File type mask bits */
enum {
	CBM_TYPE = 0x07,		/* Mask to get file type */
	CBM_CLOSED = 0x80,		/* Mask to get closed bit */
	CBM_LOCKED = 0x40		/* Mask to get locked bit */
};

/* End of 1541 filename character */
enum { CBM_END_NAME = '\xA0' };

/******************************************************************************
* Structures
******************************************************************************/
struct ArchiveHeader {
	union {
		struct {
			WORD StartAddress PACK;
			BYTE Filler1[2] PACK;
			WORD Version PACK;
			BYTE Magic1[10] PACK;

			BYTE Filler2 PACK;
			BYTE FirstOffL PACK;
			BYTE Magic2[3] PACK;
			BYTE FirstOffH PACK;
		} C64_10;

		struct {
			WORD StartAddress PACK;
			BYTE Filler1[2] PACK;
			WORD Version PACK;
			BYTE Magic1[10] PACK;

			BYTE Filler2[11] PACK;
			BYTE FirstOffL PACK;
			BYTE Magic2[3] PACK;
			BYTE FirstOffH PACK;
		} C64_13;

		struct {
			WORD StartAddress PACK;
			BYTE Filler1[2] PACK;
			WORD Version PACK;
			BYTE Magic1[10] PACK;

			BYTE Filler2[7] PACK;
			BYTE Magic2[4] PACK;
			WORD StartPointer PACK;
		} C64_15;

		struct {
			WORD StartAddress PACK;
			BYTE Filler1[2] PACK;
			WORD Version PACK;
			BYTE Magic1[10] PACK;

			BYTE Magic2 PACK;
			WORD StartPointer PACK;
		} C128_15;

		struct {
			BYTE Magic PACK;
			BYTE EntryType PACK;
		} C64_ARC;

		struct {
			WORD StartAddress PACK;
			BYTE Filler[4] PACK;
			BYTE Magic[sizeof(MagicHeaderLHASFX)] PACK;
		} LHA_SFX;

		struct {
			BYTE Filler[2] PACK;
			BYTE Magic[sizeof(MagicHeaderLHA)] PACK;
		} LHA;

		struct {
			BYTE Magic[sizeof(MagicHeaderLynx)] PACK;
		} Lynx;

		struct {
			WORD StartAddress PACK;
			WORD EndHeaderAddr PACK;
			BYTE Magic[sizeof(MagicHeaderLynxNew)] PACK;
		} LynxNew;

		struct {
			BYTE Magic[sizeof(MagicHeaderT64)] PACK;
		} T64;

		struct {
			BYTE Magic[sizeof(MagicHeaderD64)] PACK;
		} D64;

		struct {
			BYTE Magic[sizeof(MagicHeaderX64)] PACK;
		} X64;
	} Type;
};

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

struct LHAEntryHeader {
	BYTE HeadSize PACK;
	BYTE HeadChk PACK;
	BYTE HeadID[3] PACK;
	BYTE EntryType PACK;
	BYTE Magic PACK;
	LONG PackSize PACK;
	LONG OrigSize PACK;
	struct	{			/* DOS format time */
		WORD ft_tsec : 5;	/* Two second interval */
		WORD ft_min	 : 6;	/* Minutes */
		WORD ft_hour : 5;	/* Hours */
		WORD ft_day	 : 5;	/* Days */
		WORD ft_month: 4;	/* Months */
		WORD ft_year : 7;	/* Year */
	} FileTime PACK;
	WORD Attr PACK;
	BYTE FileNameLen PACK;
	BYTE FileName[64] PACK;
};

struct T64Header {
	BYTE Magic[32] PACK;
	BYTE MinorVersion PACK;
	BYTE MajorVersion PACK;
	WORD Entries PACK;
	WORD Used PACK;
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

struct D64DirHeader {
	BYTE FirstTrack PACK;
	BYTE FirstSector PACK;
	BYTE Format PACK;
	BYTE Reserved PACK;
	BYTE BAM[140] PACK;
	BYTE DiskName[18] PACK;
	BYTE Filler1 PACK;
	BYTE DOSVersion PACK;
	BYTE DOSFormat PACK;
	BYTE Filler2[4] PACK;
  /*BYTE Filler3[85] PACK;*/
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
/*	BYTE Data[]; */
};

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
static void StripBit7(char *InString)
{
	while (*InString)
		*InString++ = *InString & 0x7F;
}

/******************************************************************************
* Convert Roman numeral to decimal
* Note: input string is cleared
******************************************************************************/
static int RomanToDec(char *Roman)
{
	int Digit;
	char Last = 0;
	int Value = 0;

	while (*Roman) {
		switch (toupper(*Roman)) {
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
* Return disk image offset for 1541 disk
******************************************************************************/
static unsigned long Location1541TS(unsigned char Track, unsigned char Sector)
{
	static const unsigned Sectors[40] = {
	/* Tracks number	Offset in sectors of start of track */
	/* tracks 1-18 */	0,21,42,63,84,105,126,147,168,189,
						210,231,252,273,294,315,336,357,
	/* tracks 19-25 */	376,395,414,433,452,471,490,
	/* tracks 26-31 */	508,526,544,562,580,598,
	/* tracks 32-35 */	615,632,649,666,
	/* The rest of the tracks are nonstandard */
	/* tracks 36-40 */	683,700,717,734,751
	};
	enum {BYTES_PER_SECTOR=256};	/* bytes per sector in 1541 disk image */

	return (Sectors[Track-1] + Sector) * (long) BYTES_PER_SECTOR;
}

/******************************************************************************
* Follow chain of file sectors in disk image, counting total bytes in the file
******************************************************************************/
static unsigned long CountCBMBytes(FILE *DiskImage, unsigned long Offset,
						unsigned char FirstTrack, unsigned char FirstSector)
{
	struct D64DataBlock DataBlock;
	unsigned int BlockCount = 0;

	DataBlock.NextTrack = FirstTrack;		/* prime the track & sector */
	DataBlock.NextSector = FirstSector;
	do {
		if (fseek(DiskImage, Location1541TS(
								DataBlock.NextTrack,
								DataBlock.NextSector
							 ) + Offset, SEEK_SET) != 0) {
			perror(ProgName);
			return 2;
		}
		if (fread(&DataBlock, sizeof(DataBlock), 1, DiskImage) != 1) {
			perror(ProgName);
			return 2;
		}
		++BlockCount;
	} while (DataBlock.NextTrack > 0);

	return (BlockCount - 1) * 254 + DataBlock.NextSector - 1;
}


/******************************************************************************
* Return file type string given letter code
******************************************************************************/
static char *FileTypes(char TypeCode)
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
int DirARC(FILE *InFile, enum ArchiveTypes ArcType,	struct ArcTotals *Totals,
		int (DisplayFunction)()) {
	long CurrentPos;
	char EntryName[17];
	long FileLen;
	struct ArchiveEntryHeader FileHeader;
	struct ArchiveHeaderNew FileHeaderNew;
	struct ArchiveHeader Header;

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
* Reread header in order to get location of start of archives
* This might not be necessary, in that the version number may uniquely
*  determine the location of the start of data
******************************************************************************/

	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		perror(ProgName);
		return 2;
	}

/******************************************************************************
* Find the version number and first archive entry offset for each format
******************************************************************************/
	switch (ArcType) {
		case C64__0:	/* Not a self dearcer -- just the arc data */
			CurrentPos = 0L;
			break;

		case C64_10:
			CurrentPos = ((Header.Type.C64_10.FirstOffH << 8) | Header.Type.C64_10.FirstOffL) - Header.Type.C64_10.StartAddress + 2;
			Totals->Version = -Header.Type.C64_10.Version;
			Totals->DearcerBlocks = (CurrentPos-1) / 254 + 1;
			break;

		case C64_13:
			CurrentPos = ((Header.Type.C64_13.FirstOffH << 8) | Header.Type.C64_13.FirstOffL) - Header.Type.C64_13.StartAddress + 2;
			Totals->Version = -Header.Type.C64_13.Version;
			Totals->DearcerBlocks = (CurrentPos-1) / 254 + 1;
			break;

		case C64_15:
			fseek(InFile, Header.Type.C64_15.StartPointer - Header.Type.C64_15.StartAddress + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			CurrentPos = ((FileHeaderNew.FirstOffH << 8) | FileHeaderNew.FirstOffL) - Header.Type.C64_15.StartAddress + 2;
			Totals->Version = -Header.Type.C64_15.Version;
			Totals->DearcerBlocks = (CurrentPos-1) / 254 + 1;
			break;

		case C128_15:
			fseek(InFile, Header.Type.C128_15.StartPointer - Header.Type.C128_15.StartAddress + 2, SEEK_SET);
			fread(&FileHeaderNew, sizeof(FileHeaderNew), 1, InFile);
			CurrentPos = ((FileHeaderNew.FirstOffH << 8) | FileHeaderNew.FirstOffL) - Header.Type.C128_15.StartAddress + 2;
			Totals->Version = -Header.Type.C128_15.Version;
			Totals->DearcerBlocks = (CurrentPos-1) / 254 + 1;
			break;
	}

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		perror(ProgName);
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
		DisplayFunction(
			EntryName,
			FileTypes(FileHeader.FileType),
			(long) FileLen,
			(unsigned) (FileLen-1) / 254 + 1,
			ARCEntryTypes[FileHeader.EntryType],
			(int) (100 - (FileHeader.BlockLength * 100L / (FileLen / 254 + 1))),
			(unsigned) FileHeader.BlockLength,
			(long) FileHeader.Checksum
		);

		CurrentPos += FileHeader.BlockLength * 254;
		fseek(InFile, CurrentPos, SEEK_SET);
		++Totals->ArchiveEntries;
		Totals->TotalLength += FileLen;
		Totals->TotalBlocks += (FileLen-1) / 254 + 1;
		Totals->TotalBlocksNow += FileHeader.BlockLength;
	};
	return 0;
}

/******************************************************************************
* Lynx reading routines
******************************************************************************/
int DirLynx(FILE *InFile, enum ArchiveTypes LynxType, struct ArcTotals *Totals,
		int (DisplayFunction)()) {
	char EntryName[17];
	char FileType[2];
	int NumFiles;
	int FileBlocks;
	int LastBlockSize;
	long FileLen;
	char LynxVer[10];
	int ExpectLastLength;
	int ReadCount;

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
			if (fscanf(InFile, " %*s LYNX %s %*[^\015]", LynxVer) != 1) {
				perror(ProgName);
				return 2;
			}
			getc(InFile);				/* Get CR without killing whitespace */
			Totals->Version = RomanToDec(LynxVer);
			Totals->DearcerBlocks = 0;
			ExpectLastLength = Totals->Version >= 10;
			break;

		case LynxNew:
/*			fseek(InFile, Header.Type.LynxNew.EndHeaderAddr -
						Header.Type.LynxNew.StartAddress + 5, SEEK_SET); */
			if (fseek(InFile, 0x5F, SEEK_SET) != 0) {
				perror(ProgName);
				return 2;
			}
			if (fscanf(InFile, " %*s *LYNX %s %*[^\015]", LynxVer) != 1) {
				perror(ProgName);
				return 2;
			}
			getc(InFile);				/* Get CR without killing whitespace */
			Totals->Version = RomanToDec(LynxVer);
			Totals->DearcerBlocks = 0;
			ExpectLastLength = Totals->Version >= 10;
			break;
	}

	if (fscanf(InFile, "%d%*[^\015]\015", &NumFiles) != 1) {
		perror(ProgName);
		return 2;
	}

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	for (; NumFiles--;) {
		ReadCount = fscanf(InFile, "%16[^\015]%*[^\015]\015", EntryName);
		ReadCount += fscanf(InFile, "%d%*[^\015]\015", &FileBlocks);
		ReadCount += fscanf(InFile, "%1s%*[^\015]", FileType);
		if (ReadCount != 3) {
			perror(ProgName);
			return 2;
		}
		getc(InFile);	/* eat the CR (\015) without killing whitespace so
						   ftell() will be correct, below */
		StripBit7(EntryName);

/******************************************************************************
* Find the exact length of the file.
* For the first n-1 entries (for all n entries in newer Lynx versions, like
*  XVII), the length in bytes of the last block of the file is specified.
* For the last entry in older Lynx versions, like IX, we must find out how many
*  bytes in the file and subtract everything not belonging to the last file.
*  This can give an incorrect result if the file has padding after the last
*  file (which would happen if the file was transferred using XMODEM), but
*  Lynx thinks the padding is part of the file, too.
******************************************************************************/
		if (NumFiles || ExpectLastLength) {
			fscanf(InFile, "%d%*[^\015]\015", &LastBlockSize);
			FileLen = (long) ((FileBlocks-1) * 254L + LastBlockSize - 1);
		} else				/* last entry -- calculate based on file size */
			FileLen = filelength(fileno(InFile)) - Totals->TotalBlocksNow * 254L -
							(((ftell(InFile) - 1) / 254) + 1) * 254L;

		DisplayFunction(
			EntryName,
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
		Totals->TotalBlocks += (FileLen-1) / 254 + 1;	/*  \  These two values		*/
		Totals->TotalBlocksNow += FileBlocks;			/*  /  should be equal		*/
	};
	return 0;
}

/******************************************************************************
* LHA (SFX) reading routines
******************************************************************************/
int DirLHA(FILE *InFile, enum ArchiveTypes LHAType, struct ArcTotals *Totals,
		int (DisplayFunction)()) {
	struct LHAEntryHeader FileHeader;
	char FileName[80];
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
			Totals->DearcerBlocks = (CurrentPos-1) / 254 + 1;
			break;

		case LHA:
			CurrentPos = 0;
			Totals->Version = 0;
			Totals->DearcerBlocks = 0;
			break;
	}

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}

	while (1) {
		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;
		if (memcmp(FileHeader.HeadID, MagicLHAEntry, sizeof(MagicLHAEntry)) != 0)
			break;

		memcpy(FileName, FileHeader.FileName, min(sizeof(FileName)-1, FileHeader.FileNameLen));
		FileName[min(sizeof(FileName)-1, FileHeader.FileNameLen)] = 0;
		DisplayFunction(
			FileName,
			FileTypes(FileHeader.FileName[FileHeader.FileNameLen-2] ? ' ' : FileHeader.FileName[FileHeader.FileNameLen-1]),
			(long) FileHeader.OrigSize,
			FileHeader.OrigSize ? (unsigned) (FileHeader.OrigSize-1) / 254 + 1 : 0,
			LHAEntryTypes[FileHeader.EntryType - '0'],
			FileHeader.OrigSize ? (int) (100 - (FileHeader.PackSize * 100L / FileHeader.OrigSize)) : 100,
			FileHeader.PackSize ? (unsigned) (FileHeader.PackSize-1) / 254 + 1 : 0,
			(long) (unsigned) (FileHeader.FileName[FileHeader.FileNameLen+1] << 8) | FileHeader.FileName[FileHeader.FileNameLen]
		);

		CurrentPos += FileHeader.HeadSize + FileHeader.PackSize + 2;
		fseek(InFile, CurrentPos, SEEK_SET);
		++Totals->ArchiveEntries;
		Totals->TotalLength += FileHeader.OrigSize;
		Totals->TotalBlocks += (FileHeader.OrigSize-1) / 254 + 1;
		Totals->TotalBlocksNow += (FileHeader.PackSize-1) / 254 + 1;
	};
	return 0;
}


/******************************************************************************
* T64 reading routines
******************************************************************************/
int DirT64(FILE *InFile, enum ArchiveTypes ArchiveType, struct ArcTotals *Totals,
		int (DisplayFunction)()) {
	char FileName[17];
	int NumFiles;
	unsigned FileLength;
	struct T64Header Header;
	struct T64EntryHeader FileHeader;

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
		perror(ProgName);
		return 2;
	}
	Totals->Version = -(Header.MajorVersion * 10 + Header.MinorVersion);
	Totals->ArchiveEntries = Header.Entries;

/******************************************************************************
* Read the archive directory contents
******************************************************************************/
	for (NumFiles = Header.Entries; NumFiles; --NumFiles) {
		if (fread(&FileHeader, sizeof(FileHeader), 1, InFile) != 1)
			break;

		memcpy(FileName, FileHeader.FileName, 16);
		FileName[16] = 0;
		FileLength = FileHeader.EndAddr - FileHeader.StartAddr;
		DisplayFunction(
			FileName,
			T64FileTypes[FileHeader.FileType],
			(long) FileLength,
			FileLength / 254 + 1,
			"Stored",
			0,
			FileLength / 254 + 1,
			(long) -1L
		);

		Totals->TotalLength += FileLength;
		Totals->TotalBlocks += FileLength / 254 + 1;
	}

	Totals->TotalBlocksNow = Totals->TotalBlocks;
	return 0;
}


/******************************************************************************
* D64/X64 reading routines
******************************************************************************/
int DirD64(FILE *InFile, enum ArchiveTypes D64Type, struct ArcTotals *Totals,
		int (DisplayFunction)()) {
	char FileName[17];
	char *EndName;
	long CurrentPos;
	long HeaderOffset;
	long FileLength;
	int EntryCount;
	struct D64DirHeader DirHeader;
	struct D64DirBlock DirBlock;

	Totals->ArchiveEntries = 0;
	Totals->TotalBlocks = 0;
	Totals->TotalBlocksNow = 0;
	Totals->TotalLength = 0;
	Totals->DearcerBlocks = 0;
	Totals->Version = 0;

/******************************************************************************
* Find the version number and first archive entry offset for each format
******************************************************************************/
	switch (D64Type) {
		case D64:
			HeaderOffset = 0;			/* No header on D64 images */
			CurrentPos = Location1541TS(18,0);
			break;

		case X64:
			HeaderOffset = 0x40;		/* X64 header takes 64 bytes */
			CurrentPos = Location1541TS(18,0) + HeaderOffset;
			break;
	}

/******************************************************************************
* Read the disk directory header block
******************************************************************************/
	if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
		perror(ProgName);
		return 2;
	}
	if (fread(&DirHeader, sizeof(DirHeader), 1, InFile) != 1) {
		perror(ProgName);
		return 2;
	}
	if (DirHeader.Format != 'A') {		/* Disk format code */
		fprintf(stderr,"%s: Unsupported disk image format (%c)\n", ProgName, DirHeader.Format);
		return 3;
	}

/******************************************************************************
* Go through the entire directory
******************************************************************************/
	/* Simulate having read a directory sector already */
	DirBlock.NextTrack = DirHeader.FirstTrack;
	DirBlock.NextSector = DirHeader.FirstSector;

	while (DirBlock.NextTrack > 0) {
		CurrentPos = Location1541TS(DirBlock.NextTrack, DirBlock.NextSector) + HeaderOffset;

		if (fseek(InFile, CurrentPos, SEEK_SET) != 0) {
			perror(ProgName);
			return 2;
		}
		if (fread(&DirBlock, sizeof(DirBlock), 1, InFile) != 1) {
			perror(ProgName);
			return 2;
		}

		for (EntryCount=0; EntryCount < D64_ENTRIES_PER_BLOCK; ++EntryCount) {
			if ((DirBlock.Entry[EntryCount].FileType & CBM_CLOSED) != 0) {

				FileLength = CountCBMBytes(
								InFile,
								HeaderOffset,
								DirBlock.Entry[EntryCount].FirstTrack,
								DirBlock.Entry[EntryCount].FirstSector
							 );
				strncpy(FileName,DirBlock.Entry[EntryCount].FileName,sizeof(FileName)-1);
				FileName[sizeof(FileName)-1] = 0;
				if ((EndName = strchr(FileName,CBM_END_NAME)) != NULL)
					*EndName = 0;
				StripBit7(FileName);

				DisplayFunction(
					FileName,
					CBMFileTypes[DirBlock.Entry[EntryCount].FileType & CBM_TYPE],
					FileLength,
					DirBlock.Entry[EntryCount].FileBlocks,
					"Stored",
					0,
					DirBlock.Entry[EntryCount].FileBlocks,
					(long) -1L
				);
				Totals->TotalLength += FileLength;
				Totals->TotalBlocks += DirBlock.Entry[EntryCount].FileBlocks;
				++Totals->ArchiveEntries;
			}

		}
	}

	Totals->TotalBlocksNow = Totals->TotalBlocks;
	return 0;
}


/******************************************************************************
* Read the archive and determine which type it is
******************************************************************************/
enum ArchiveTypes DetermineArchiveType(FILE *InFile)
{
	struct ArchiveHeader Header;
	enum ArchiveTypes ArchiveType;

	if (fread(&Header, sizeof(Header), 1, InFile) != 1) {
		fprintf(stderr,"%s: Not a known Commodore archive\n", ProgName);
		return UnknownArchive;		/* Disk read error, probably file too short */
	}

/******************************************************************************
* Is it a C64 format?
******************************************************************************/
	if (memcmp(Header.Type.C64_10.Magic1, MagicHeaderC64, sizeof(MagicHeaderC64)) == 0) {
		if (memcmp(Header.Type.C64_10.Magic2, MagicC64_10, sizeof(MagicC64_10)) == 0)
			ArchiveType = C64_10;
		if (memcmp(Header.Type.C64_13.Magic2, MagicC64_13, sizeof(MagicC64_13)) == 0)
			ArchiveType = C64_13;
		if (memcmp(Header.Type.C64_15.Magic2, MagicC64_15, sizeof(MagicC64_15)) == 0)
			ArchiveType = C64_15;

/******************************************************************************
* Is it a C128 format?
******************************************************************************/
	} else if (memcmp(Header.Type.C128_15.Magic1, MagicHeaderC128, sizeof(MagicHeaderC128)) == 0) {
		if (Header.Type.C128_15.Magic2 == MagicC128_15)
			ArchiveType = C128_15;

/******************************************************************************
* Is it headerless ARC format?
* (This type does not have a dearcer built in, just the data)
******************************************************************************/
	} else if ((BYTE) Header.Type.C64_ARC.Magic == MagicHeaderARC) {
		ArchiveType = C64__0;

/******************************************************************************
* Is it LHA SFX format?
******************************************************************************/
	} else if (memcmp(Header.Type.LHA_SFX.Magic, MagicHeaderLHASFX, sizeof(MagicHeaderLHASFX)) == 0) {
		ArchiveType = LHA_SFX;

/******************************************************************************
* Is it headerless LHA format?
******************************************************************************/
	} else if (memcmp(Header.Type.LHA.Magic, MagicHeaderLHA, sizeof(MagicHeaderLHA)) == 0) {
		ArchiveType = LHA;

/******************************************************************************
* Is it Lynx format?
******************************************************************************/
	} else if (memcmp(Header.Type.Lynx.Magic, MagicHeaderLynx, sizeof(MagicHeaderLynx)) == 0) {
		ArchiveType = Lynx;

	} else if (memcmp(Header.Type.LynxNew.Magic, MagicHeaderLynxNew, sizeof(MagicHeaderLynxNew)) == 0) {
		ArchiveType = LynxNew;

/******************************************************************************
* Is it T64 format?
******************************************************************************/
	} else if (memcmp(Header.Type.T64.Magic, MagicHeaderT64, sizeof(MagicHeaderT64)) == 0) {
		ArchiveType = T64;

/******************************************************************************
* Is it D64 format?
* It appears the only good way to detect a D64 archive is to go look at
*  "track 18, sector 0" -- this way works on 1571-made autoboot disks (right?)
******************************************************************************/
	} else if (memcmp(Header.Type.D64.Magic, MagicHeaderD64, sizeof(MagicHeaderD64)) == 0) {
		ArchiveType = D64;

/******************************************************************************
* Is it X64 format?
******************************************************************************/
	} else if (memcmp(Header.Type.X64.Magic, MagicHeaderX64, sizeof(MagicHeaderX64)) == 0) {
		ArchiveType = X64;

/******************************************************************************
* Unrecognized format
******************************************************************************/
	} else {
		ArchiveType = UnknownArchive;
		fprintf(stderr,"%s: Not a known Commodore archive\n", ProgName);
	}

	return ArchiveType;
}

/******************************************************************************
* Read and display the archive directory
******************************************************************************/
int DirArchive(FILE *InFile, enum ArchiveTypes ArchiveType,
		struct ArcTotals *Totals,
			int (DisplayFunction)())
{
/* Could put these functions into an array and call them using ArchiveType as index */
		switch (ArchiveType) {
			case C64__0:
			case C64_10:
			case C64_13:
			case C64_15:
			case C128_15:
				return DirARC(InFile, ArchiveType, Totals, DisplayFunction);

			case LHA_SFX:
			case LHA:
				return DirLHA(InFile, ArchiveType, Totals, DisplayFunction);

			case Lynx:
			case LynxNew:
				return DirLynx(InFile, ArchiveType, Totals, DisplayFunction);

			case T64:
				return DirT64(InFile, ArchiveType, Totals, DisplayFunction);

			case D64:
			case X64:
				return DirD64(InFile, ArchiveType, Totals, DisplayFunction);

			default:		/* this should never happen */
				return 3;
		}
}
