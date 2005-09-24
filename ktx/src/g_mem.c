/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on Q3 VM code by Id Software, Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: g_mem.c,v 1.1.1.1 2005/09/24 12:45:02 disconn3ct Exp $
 */

// g_mem.c
//
#include "g_local.h"


#define POOLSIZE	(256 * 1024)

static char     memoryPool[POOLSIZE];
static int      allocPoint;

void           *G_Alloc( int size )
{
	char           *p;

/*	if ( g_debugAlloc.integer ) {
		G_Printf( "G_Alloc of %i bytes (%i left)\n", size, POOLSIZE - allocPoint - ( ( size + 31 ) & ~31 ) );
	}*/

	if ( allocPoint + size > POOLSIZE )
	{
		G_Error( "G_Alloc: failed on allocation of %i bytes\n", size );	// bk010103 - was %u, but is signed
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

void G_InitMemory( void )
{
	allocPoint = 0;
}
