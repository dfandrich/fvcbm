/*
 * fvcbm.c
 *
 * File View for Commodore 64 and 128 self dissolving and other archives
 * Displays a list of the file names and file info contained within an archive
 * Supports ARC230 (C64 & C128 versions), ARC230 SDA, Lynx, LHA, LHA SFX
 *   T64 (tape image), TAP (raw tape image), D64/X64 (disk image),
 *   PC64 (headered R/S/U/P00 file), 64Net (.N64) archive formats
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
 * fvcbm is copyright (C) 1995-2023 by Daniel Fandrich
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with fvcbm; if not, see <https://www.gnu.org/licenses/>.
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
#define VERSION "3.2dev"
#define VERDATE "2023-09-15"

#if defined(__MSDOS__)
#define MAXPATH 80			/* length of longest permissible file path */
#else /* UNIX */
#define MAXPATH 1025			/* length of longest permissible file path */
#endif

#if defined(__TURBOC__)
unsigned _stklen = 8000;	/* printf() does strange things sometimes with the
							   default 4k stack */
#endif

const char * const ProgName = "fvcbm";	/* should rather use argv[0] on Unix */

/* Archive extensions to try, in this order */
/* List must end with NULL */
#define MAX_EXT_LEN	4		/* longest extension in this list, including dot */
const char * const Extensions[] = {
".sda", ".sfx", ".d64", ".x64", ".t64", ".lnx", ".lzh",
".n64", ".arc", ".lbr", ".p00", ".s00", ".u00", ".r00",
".tap",
NULL
};

/******************************************************************************
* Global Variables
******************************************************************************/
int WideFormat = 1;			/* zero when 1541-style listing is selected */

/******************************************************************************
* Functions
******************************************************************************/

/******************************************************************************
* Returns the length of an open file in bytes
******************************************************************************/
#ifdef UNIX
#include <sys/stat.h>

long filelength(int handle)
{
	struct stat statbuf;

	if (fstat(handle, &statbuf))
		return -1;
	return statbuf.st_size;
}

#elif defined(__Z88DK)
#include <unistd.h>

long filelength(int handle)
{
    unsigned long l, old;
    old = lseek(handle,0,SEEK_CUR);
    l = lseek(handle,0,SEEK_END);
    lseek(handle,old,SEEK_SET);
    return l;
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
	int FirstFileName = 1;
	char FileName[MAXPATH+1];
	FILE *InFile;
	enum ArchiveTypes ArchiveType;
	struct ArcTotals Totals;

#ifndef __Z88DK
	setvbuf(stdout, NULL, _IOLBF, 82);		/* speed up screen output */
#endif

	if ((argc < 2) ||
		(((argv[FirstFileName][0] == '-') || (argv[FirstFileName][0] == '/')) &&
		 ((argv[FirstFileName][1] == '?') || (argv[FirstFileName][1] == 'h')
#ifdef CPM
		  || (argv[FirstFileName][1] == 'H')
#endif
		 ) &&
		 (argv[FirstFileName][2] == '\x0'))) {
		printf("%s  ver. " VERSION "  " VERDATE "  by Daniel Fandrich\n", ProgName);
		printf("Usage:\n  %s [-d] filename1 [filenameN ...]\n"
			   "View directory of Commodore 64/128 archive and self-dissolving archive files.\n"
			   "Supports ARC230, Lynx, LZH (SFX), T64, TAP, D64, X64, N64, PC64 & LBR archive\n"
			   "types.\n"
			   "fvcbm is copyright (C) 1995-2023 by Daniel Fandrich, et. al.\n"
			   "This program comes with NO WARRANTY. See the file COPYING for details.\n",
			   ProgName);
		return 1;
	}

	if (((argv[FirstFileName][0] == '-') || (argv[FirstFileName][0] == '/')) &&
		((argv[FirstFileName][1] == 'd')
#ifdef CPM
		  || (argv[FirstFileName][1] == 'D')
#endif
		) && (argv[FirstFileName][2] == '\x0')) {
		WideFormat = 0;		/* 1541-style output */
		++FirstFileName;
	} else {
		WideFormat = 1;		/* wide FV-style output */
	}

	/* -- ends options */
	if ((argv[FirstFileName][0] == '-') && (argv[FirstFileName][1] == '-')) {
		++FirstFileName;
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
		} else if ((InFile = fopen(FileName, "rb")) == NULL) {

/******************************************************************************
* Given name wasn't found--add extensions and keep searching
******************************************************************************/
			const char * const *Ext;
			char TryFileName[MAXPATH+1];

			for (Ext = Extensions; *Ext != NULL; ++Ext) {
				strcat(strcpy(TryFileName, FileName), *Ext);
				if ((InFile = fopen(TryFileName, "rb")) != NULL) {
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
