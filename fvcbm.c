/*
 * fvcbm.c
 *
 * File View for Commodore 64 and 128 self dissolving and other archives
 * Displays a list of the file names and file info contained within an archive
 * Supports ARC230 (C64 & C128 versions), ARC230 SDA, Lynx, LHA, LHA SFX
 *   T64 (tape image), D64/X64 (disk image) archive formats
 * Inspired by Vernon D. Buerg's FV program for MS-DOS archives
 *
 * Written under MS-DOS; compiled under Turbo C ver. 2.0; use -a- option to
 *   align structures on bytes; link with WILDARGS.OBJ for wildcard
 *   pattern matching
 * Microsoft C support; compile with -DMSC and -Zp1 option to align structures
 *   on bytes  (note: this support has not been properly tested)
 * Support for 386 SCO UNIX System V/386 Release 3.2; compile cbmarcs.c with
 *   -Zp1 option to align structures on bytes; compile fvcbm.c without
 *   -Zp1
 * There are many dependencies on little-endian architecture -- porting to
 *   big-endian machines will take some work
 *
 * Source file tab size is 4
 *
 * Things to do:
 *  - somehow find a way to determine a raw D64 file without the CBM header
 *    (are they required to have the header?)
 *  - file paths with '/' instead of '\' aren't handled right by WILDARGS.OBJ
 *    (Turbo C only)
 *	- search for all supported file extensions before giving up
 *  - perhaps output consistent case or do PETSCII to ASCII conversion
 *	- fix display of LHA archives with long path names
 *  - add display of Lynx oc'ult and REL record lengths
 *
 * Version:
 *	93-01-05  ver. 1.0  by Daniel Fandrich
 *		Domain email: <dan@fch.wimsey.bc.ca>; CompuServe: 72365,306
 *		Initial release
 *	93-04-14  ver. 1.1 (unreleased)
 *		Moved code around to make adding new archive types easier
 *		Added Lynx & LHA archive support
 *	93-07-24  ver. 1.2 (unreleased)
 *		Added support for 386 SCO UNIX
 *		Added support for Microsoft C (untested)
 *		Added support for multiple files on command line
 *		Fixed Lynx IX file lengths
 *		Added Lynx XVII support, including more reliable last file lengths
 *		Fixed LHA 0 length files
 *  95-01-04  ver. 1.3
 *      Reorganized code
 *      Fixed spacing of error lines
 *		Added T64 archive support
 *		Added D64/X64 archive support
 *      Added 1541-style directory listing (-d command-line option)
 *
 * This program is in the public domain.  Use it as you see fit, but please
 * give credit where credit is due.
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
#include <fcntl.h>

#elif defined(MSC)
#include <io.h>
#include <fcntl.h>

#else /* UNIX */
/* #include <endian.h> */
#endif

#include "cbmarcs.h"

#if defined(BIG_ENDIAN) || (WORDS_BIG_ENDIAN==1)
#error fvcbm requires a little-endian CPU
#endif

/******************************************************************************
* Constants
******************************************************************************/
#define VERSION "1.3"
#define VERDATE "95-01-04"

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

char *ProgName = "fvcbm";	/* this should be changed to argv[0] for Unix */
#define DefaultExt ".sda"

/******************************************************************************
* Global Variables
******************************************************************************/
int WideFormat = 1;

/******************************************************************************
* Functions
******************************************************************************/

#ifdef UNIX
#include <sys/stat.h>

/******************************************************************************
* Returns the length of an open file in bytes
******************************************************************************/
long filelength(int handle)
{
	struct stat statbuf;

	if (fstat(handle, &statbuf))
		return -1;
	return statbuf.st_size;
}
#endif


/******************************************************************************
* Display header information about an archive
******************************************************************************/
void DisplayHeader(enum ArchiveTypes ArchiveType, const char *Name)
{
	if (WideFormat) {
		printf("\nName              Type  Length  Blks  Method     SF   Now   Check\n");
		printf(  "================  ====  ======  ====  ========  ====  ====  =====\n");
	} else
		printf("\n");
}

/******************************************************************************
* Display a file's name and info
******************************************************************************/
int DisplayFile(const char *Name, const char *Type, unsigned long Length,
			unsigned Blocks, const char *Storage, int Compression,
			unsigned BlocksNow, long Checksum)
{
	char QuoteName[19];

	if (WideFormat) {
		printf("%-16s  %s  %7lu  %4u  %-8s %4d%%  %4u",
				Name, Type, Length, Blocks, Storage, Compression, BlocksNow);
		if (Checksum >= 0)
			printf("   %04X", (int) Checksum);
		printf("\n");
	} else {
		QuoteName[0] = '"';
		strcpy(QuoteName+1,Name);
		strcat(QuoteName,"\"");
		printf("%-5u%-18s %s\n",
			Blocks, QuoteName, Type);
	}
	return 0;
}

/******************************************************************************
* Display summary information about an archive
******************************************************************************/
void DisplayTrailer(enum ArchiveTypes ArchiveType, const struct ArcTotals *Totals)
{
	if (WideFormat) {
		printf("================  ====  ======  ====  ========  ====  ====  =====\n");
		printf("*total %5u           %7lu  %4d  %s",
			Totals->ArchiveEntries,
			Totals->TotalLength,
			Totals->TotalBlocks,
			ArchiveFormats[ArchiveType]);
		if (Totals->Version > 0)
			printf("%3u ", Totals->Version);
		else if (Totals->Version < 0)
			printf("%2u.%u",
				-Totals->Version / 10,
				-Totals->Version - 10 * (-Totals->Version / 10)
			);
		else
			printf("    ");
		printf(" %4d%%  %4d",
			Totals->TotalBlocks == 0 ?
				0 :
				(unsigned) (100 - (Totals->TotalBlocksNow * 100L / (Totals->TotalBlocks))),
			Totals->TotalBlocksNow
		);
		if (Totals->DearcerBlocks > 0)
			printf("+%d\n", Totals->DearcerBlocks);
		else
			printf("\n");

	} else
		printf("%u BLOCKS USED.\n", Totals->TotalBlocks);
}


/******************************************************************************
* Main loop
******************************************************************************/
int main(int argc, char *argv[])
{
	int ArgNum;
	int Error = 0;
	int DispError;
	int FirstFileName;
	char FileName[MAXPATH+1];
	FILE *InFile;
	enum ArchiveTypes ArchiveType;
	struct ArcTotals Totals;

	setvbuf(stdout, NULL, _IOLBF, 82);		/* speed up screen output */
	if ((argc < 2) ||
		(((argv[1][0] == '-') || (argv[1][0] == '/')) &&
		 ((argv[1][1] == '?') || (argv[1][1] == 'h')) &&
		 (argv[1][2] == '\x0'))) {
		printf("%s  ver. " VERSION "  " VERDATE "  by Daniel Fandrich\n\n", ProgName);
		printf("Usage:\n   %s [-d] filename1[" DefaultExt "] "
					"[filename2[" DefaultExt "] "
					"[... filenameN[" DefaultExt "]]]\n"
			   "View directory of Commodore 64/128 archive and self-dissolving archive files.\n"
			   "Supports ARC230, Lynx, LHA (SFX), T64, D64 and X64 archive types.\n"
			   "Placed into the public domain by Daniel Fandrich.\n", ProgName);
		return 1;
	}

	if (((argv[1][0] == '-') || (argv[1][0] == '/')) &&
		 (argv[1][1] == 'd') && (argv[1][2] == '\x0')) {
		WideFormat = 0;
		FirstFileName = 2;
	} else {
		WideFormat = 1;
		FirstFileName = 1;
	}

/******************************************************************************
* Loop through archive display for each file name
******************************************************************************/
	for (ArgNum=FirstFileName; ArgNum<argc; ++ArgNum) {

/******************************************************************************
* Open the archive file
******************************************************************************/
		strncpy(FileName, argv[ArgNum], sizeof(FileName) - sizeof(DefaultExt) - 1);
		FileName[sizeof(FileName)-1] = 0;
		if (strcmp(FileName,"-") == 0) {		/* "-" means read from stdin */
			InFile = stdin;
#ifdef __MSDOS__
			setmode(fileno(stdin), O_BINARY);	/* put standard input into binary mode */
#endif
		} else if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {
			strcat(FileName, DefaultExt);
			if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {
				perror(FileName);
				printf("\n");
				fclose(InFile);
				Error = 2;
				continue;		/* go do next file */
			}
		}
		printf("Archive: %s\n", FileName);

		if ((ArchiveType = DetermineArchiveType(InFile)) == UnknownArchive)
			Error = 3;
		else {

/******************************************************************************
* Display the archive contents
******************************************************************************/
			DisplayHeader(ArchiveType, "");

			if ((DispError = DirArchive(InFile, ArchiveType, &Totals, DisplayFile)) != 0)
				Error = DispError;

			DisplayTrailer(ArchiveType, &Totals);
		}

/******************************************************************************
* Go do next file name on command line
******************************************************************************/
		fclose(InFile);
		if (ArgNum<argc-1)
			printf("\n");
	}

	return Error;
}
