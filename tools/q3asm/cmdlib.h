/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#ifdef _MSC_VER
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma check_stack(off)

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#ifdef _MSC_VER

#pragma intrinsic( memset, memcpy )

#endif

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum { qfalse, qtrue } qboolean;
typedef unsigned char byte;
#endif

#ifdef _WIN32
#define	MAX_OS_PATH   MAX_PATH
#ifndef PATH_SEP
#define PATH_SEP '\\'
#endif
#else
#define	MAX_OS_PATH   1024
#ifndef PATH_SEP
#define PATH_SEP '/'
#endif
#endif

int Q_filelength( FILE *f );

void	Q_mkdir( const char *path );

double I_FloatTime( void );

void	Error( const char *error, ... );

FILE	*SafeOpenWrite( const char *filename );
FILE	*SafeOpenRead( const char *filename );
void	SafeRead( FILE *f, void *buffer, int count );
void	SafeWrite( FILE *f, const void *buffer, int count );

int		LoadFile( const char *filename, void **bufferptr );
void	SaveFile( const char *filename, const void *buffer, int count );

void 	DefaultExtension( char *path, const char *extension );
void 	StripExtension( char *path );

void	CreatePath( const char *path );

void	ExtractFileExtension( const char *path, char *dest );

char    *COM_Parse ( char *data );

extern char com_token[1024];

char    *copystring( const char *s );

int     BigEndian( void );
int     LongSwap( int l );

#endif
