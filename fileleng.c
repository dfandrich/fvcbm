/*
 * filelength.c
 *
 * Contains filelength() which returns the length of a file given a handle.
 * Because it depends on struct stat, it cannot be compiled with the
 *   compressed structures flag (-Zp1 in SCO Unix).
 * Compile normally and link to fvcbm.c
 *
 * Version:
 *	93-07-24  ver. 1.2  by Daniel Fandrich
 *		Domain: <dan@fch.wimsey.bc.ca>; CompuServe: 72365,306
 *		Initial release
 *
 * This routine is in the public domain.  Use it as you see fit.
 */

#include <sys/stat.h>
#include "filelength.h"

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
