/*
 * fvcbm.c
 *
 * File View for Commodore 64 and 128 self dissolving and other archives
 * Displays a list of the file names and file info contained within an archive
 * Supports ARC230 (C64 & C128 versions), ARC230 SDA, Lynx, LHA, LHA SFX
 *   T64 (tape image), D64/X64 (disk image), PC64 (headered R/S/U/P00 file),
 *   64Net (.N64) archive formats
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
 *  - add option to force fvcbm to assume a file is a certain archive format
 *  - file paths with '/' instead of '\' aren't handled right by WILDARGS.OBJ
 *    (Turbo C only)
 *  - perhaps output consistent file name case or do PETSCII to ASCII
 *    conversion
 *	- fix display of LHA archives with long path names
 *  - add display of Lynx oc'ult mode
 *  - add REL record length display (though I've never seen REL in an archive)
 *  - add Wraptor archive format (.wra)
 *  - look for info on SDL archive format (Self Dissolving Lynx)
 *  - look for info on WAD archive format (similar to ZipCode)
 *  - look for info on MAD archive format (self-extracting)
 *  - look for info on ARK and LIB archives
 *  - add Zipcode archive format (1!*)
 *  - think about adding PKZIP archive support since it's popular among
 *    emulator users for compressing disk images
 *
 * Version:
 *	93-01-05  ver. 1.0  by Daniel Fandrich
 *		Domain email: <dan@fch.wimsey.bc.ca>; CompuServe: 72365,306
 *		Initial release with ARC230 support only
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
 *		Many changes to cbmarcs.c
 *	95-10-12  ver. 3.0
 *		Added searching through a list of file extensions to find a file
 *		Many changes to cbmarcs.c
 *
 * fvcbm is copyright (C) 1995-1996 by Daniel Fandrich
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * See the file COPYING, which contains a copy of the GNU General
 * Public License.
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

#elif defined(MSC) || defined(__ZTC__) || defined(__SC__) || defined(__WATCOMC__)
#include <io.h>
#include <fcntl.h>

#else /* UNIX */
/* */
#endif

/* Get some automatic filename globbing */
#ifdef __ZTC__
#undef MSDOS
#define MSDOS
#include <dos.h>
WILDCARDS
#endif

#include "cbmarcs.h"

/******************************************************************************
* Constants
******************************************************************************/
#define VERSION "3.0"
#define VERDATE "96-09-14"

#if defined(__MSDOS__)
#define READ_BINARY "rb"
#define MAXPATH 80			/* length of longest permissible file path */
#else /* UNIX */
#define MAXPATH 1025			/* length of longest permissible file path */
#define READ_BINARY "r"
#endif

#if defined(__TURBOC__)
unsigned _stklen = 8000;	/* printf() does strange things sometimes with the
							   default 4k stack */
#endif

const char *ProgName = "fvcbm";	/* this should be changed to argv[0] for Unix */

/* Archive extensions to try, in this order */
/* List must end with NULL */
#define MAX_EXT_LEN	4		/* longest extension in this list, including dot */
const char *Extensions[] = {
".sda", ".sfx", ".d64", ".x64", ".t64", ".lnx", ".lzh",
".n64", ".arc", ".lbr", ".p00", ".s00", ".u00", ".r00",
NULL
};

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
			printf("%4u", Totals->Version);
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
		printf("%s  ver. " VERSION "  " VERDATE "  by Daniel Fandrich\n", ProgName);
		printf("Usage:\n  %s [-d] filename1 [filenameN ...]\n"
			   "View directory of Commodore 64/128 archive and self-dissolving archive files.\n"
			   "Supports ARC230, Lynx, LZH (SFX), T64, D64, X64, N64, PC64 & LBR archive types.\n"
			   "fvcbm is copyright (C) 1995-1996 by Daniel Fandrich.\n"
			   "This program comes with NO WARRANTY. See the file COPYING for details.\n",
			   ProgName);
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
		strncpy(FileName, argv[ArgNum], sizeof(FileName) - MAX_EXT_LEN);
		FileName[sizeof(FileName)-1] = 0;

		if (strcmp(FileName,"-") == 0) {		/* "-" means read from stdin */
			InFile = stdin;
#ifdef __MSDOS__
			setmode(fileno(stdin), O_BINARY);	/* put standard input into binary mode */
#endif
		} else if ((InFile = fopen(FileName, READ_BINARY)) == NULL) {

/******************************************************************************
* Given name wasn't found--add extensions and keep searching
******************************************************************************/
			const char **Ext;
			char TryFileName[MAXPATH+1];

			for (Ext = Extensions; *Ext != NULL; ++Ext) {
				strcat(strcpy(TryFileName, FileName), *Ext);
				if ((InFile = fopen(TryFileName, READ_BINARY)) != NULL) {
					strcpy(FileName, TryFileName);
					break;
				}
			}

/******************************************************************************
* Couldn't find any variation of the file name
******************************************************************************/
			if (InFile == NULL) {
				perror(FileName);
				printf("\n");
				Error = 2;
				continue;		/* go do next file */
			}
		}

/******************************************************************************
* Display header
* To do: Add display of archive comment
******************************************************************************/
		printf("Archive: %s\n", FileName);

		if ((ArchiveType = DetermineArchiveType(InFile,FileName)) == UnknownArchive) {
			fprintf(stderr,"%s: Not a known Commodore archive\n", ProgName);
			Error = 3;
		} else {

/******************************************************************************
* Display the archive contents
******************************************************************************/
			if ((DispError = DirArchive(InFile, ArchiveType, &Totals,
										DisplayHeader, DisplayFile)) != 0)
				Error = DispError;
			else
				DisplayTrailer(ArchiveType, &Totals);	/* show output trailer */
		}

/******************************************************************************
* Go do next file name on command line
******************************************************************************/
		fclose(InFile);
		if (ArgNum<argc-1)
			printf("\n");
	}

	fflush(stdout);		/* Make sure the buffered output is displayed */

	return Error;
}
