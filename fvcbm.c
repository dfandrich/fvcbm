/*
 * fvcbm.c
 *
 * File View for Commodore 64 and 128 self dissolving and other archives
 * Displays a list of the file names and file info contained within an archive
 * Supports ARC230 (C64 & C128 versions), ARC230 SDA, Lynx, LHA, LHA SFX
 *   T64 (tape image), D64/X64 (disk image), PC64 (headered R/S/U/P00 file)
 *   archive formats
 * Inspired by Vernon D. Buerg's FV program for MS-DOS archives
 *
 * Written under MS-DOS; compiled under Turbo C ver. 2.0; use -a- option to
 *   align structures on bytes in cbmarcs.c; link with WILDARGS.OBJ for
 *	 wildcard pattern matching
 * Microsoft C support; compile with -DMSC and -Zp1 option to align structures
 *   on bytes in cbmarcs.c (note: this support has not been properly tested)
 * Support for 386 SCO UNIX System V/386 Release 3.2; compile cbmarcs.c with
 *   -Zp1 option to align structures on bytes; compile fvcbm.c without -Zp1
 * Big-endian architecture support has been added but is untested
 *
 * Source file tab size is 4
 *
 * Things to do:
 *  - use a better way to determine a raw D64 file without the `CBM' header
 *  - add option to force fvcbm to assume a file is a certain archive format
 *  - file paths with '/' instead of '\' aren't handled right by WILDARGS.OBJ
 *    (Turbo C only)
 *	- search for lots of known file extensions before giving up
 *  - perhaps output consistent file name case or do PETSCII to ASCII
 *    conversion
 *	- fix display of LHA archives with long path names
 *  - add display of Lynx oc'ult mode
 *  - add REL record length display
 *  - look for info on ARK, LIB and Zipcode archives & possibly add them
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
 *	95-01-04  ver. 1.3 (unreleased)
 *		Reorganized code
 *		Added support for GNU C & Linux
 *		Fixed spacing of error lines
 *		Added T64 archive support
 *		Added D64/X64 archive support
 *		Added 1541-style directory listing (-d command-line option)
 *	95-01-20  ver. 2.0
 *		Added X64 version number
 *		Fixed X64 archive determination
 *		Added PC64 (P00) archive types
 *	95-03-11  ver. 2.1 (CURRENTLY UNRELEASED)
 *		Added 1581 disk image to D64/X64
 *		Fixed LHA 0-length files
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
/**/
#endif

#include "cbmarcs.h"

/******************************************************************************
* Constants
******************************************************************************/
#define VERSION "2.1à"
#define VERDATE "95-04-22"

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
#define DefaultExt ".sda"	/* the only file extension automatically tried */

/******************************************************************************
* Global Variables
******************************************************************************/
int WideFormat = 1;			/* zero when 1541-style listing is selected */

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
		if (Name)
			printf("Title:   %s\n", Name);
		printf("\nName              Type  Length  Blks  Method     SF   Now   Check\n");
		printf(  "================  ====  ======  ====  ========  ====  ====  =====\n");

	} else {
		if (Name)
			printf("\n     \"%s\"", Name);
		printf("\n");
	}
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
			   "Supports ARC230, Lynx, LHA (SFX), T64, D64, X64 and PC64 archive types.\n"
			   "Placed into the public domain by Daniel Fandrich.\n", ProgName);
		return 1;
	}

	if (((argv[1][0] == '-') || (argv[1][0] == '/')) &&
		 (argv[1][1] == 'd') && (argv[1][2] == '\x0')) {
		WideFormat = 0;		/* 1541-style output */
		FirstFileName = 2;
	} else {
		WideFormat = 1;		/* wide FV-style output */
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

		if ((ArchiveType = DetermineArchiveType(InFile,FileName)) == UnknownArchive)
			Error = 3;
		else {

/******************************************************************************
* Display the archive contents
******************************************************************************/
			DisplayHeader(ArchiveType, NULL);		/* show output header */

			if ((DispError = DirArchive(InFile, ArchiveType, &Totals, DisplayFile)) != 0)
				Error = DispError;

			DisplayTrailer(ArchiveType, &Totals);	/* show output trailer */
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
