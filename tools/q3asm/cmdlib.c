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
// cmdlib.c

#include "cmdlib.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#elif defined(NeXT)
#include <libc.h>
#else
#include <unistd.h>
#endif

char		com_token[1024];
qboolean	com_eof;

/*
=================
Error

For abnormal program terminations in console apps
=================
*/
void Error( const char *error, ...)
{
	va_list argptr;

	printf( "\n************ ERROR ************\n" );
	va_start( argptr, error );
	vprintf( error, argptr );
	va_end( argptr );
	printf( "\n" );
	exit( 1 );
}


char *copystring( const char *s )
{
	int		len;
	char	*b;
	len = strlen( s ) + 1;
	b = malloc( len );
	memcpy( b, s, len );
	return b;
}


/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
#ifdef _WIN32
	DWORD count;
	count = GetTickCount();
	return (double)count*0.001;
#else
	time_t	t;
	time (&t);
	return t;
#endif

#if 0
// more precise, less portable
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
#endif
}


void Q_mkdir (const char *path)
{
#ifdef _WIN32
	if (_mkdir (path) != -1)
		return;
#else
	if (mkdir (path, 0777) != -1)
		return;
#endif
	if (errno != EEXIST)
		Error ("mkdir %s: %s",path, strerror(errno));
}

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int		c;
	int		len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			com_eof = qtrue;
			return NULL;			// end of file;
		}
		data++;
	}
	
	// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		for ( ;; ) {
		//do
		//{
			c = *data++;
			if (c=='\"')
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		//} while (1);
		}
	}

	// parse single characters
	switch ( c ) {
		case '{': case '}':
		case '(': case ')':
		case '\'': case ':': 
		{
			com_token[len++] = c;
			com_token[len] = '\0';
			return data+1;
		}
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		switch ( c ) {
			case '{': case '}':
			case '(': case ')':
			case '\'': case ':': 
			{
				com_token[len] = '\0';
				return data;
			}
		}
	} while ( c > ' ' );

	com_token[len] = '\0';

	return data;
}


/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

#ifdef MAX_PATH
#undef MAX_PATH
#endif
#define MAX_PATH 4096

static FILE* myfopen( const char* filename, const char* mode )
{
	char* p;
	char fn[MAX_PATH];

	fn[0] = '\0';
	strncat(fn, filename, sizeof(fn)-1);

	for(p=fn;*p;++p) if(*p == '\\') *p = '/';

	return fopen(fn, mode);
}


FILE *SafeOpenWrite (const char *filename)
{
	FILE	*f;

	f = myfopen(filename, "wb");

	if (!f)
		Error ("Error opening %s: %s",filename,strerror(errno));

	return f;
}

FILE *SafeOpenRead (const char *filename)
{
	FILE	*f;

	f = myfopen(filename, "rb");

	if (!f)
		Error ("Error opening %s: %s",filename,strerror(errno));

	return f;
}


void SafeRead (FILE *f, void *buffer, int count)
{
	if ( fread (buffer, 1, count, f) != (size_t)count)
		Error ("File read failure");
}


void SafeWrite (FILE *f, const void *buffer, int count)
{
	if (fwrite (buffer, 1, count, f) != (size_t)count)
		Error ("File write failure");
}


/*
==============
LoadFile
==============
*/
int    LoadFile( const char *filename, void **bufferptr )
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = SafeOpenRead (filename);
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
SaveFile
==============
*/
void    SaveFile ( const char *filename, const void *buffer, int count )
{
	FILE	*f;

	f = SafeOpenWrite( filename );
	SafeWrite( f, buffer, count );
	fclose( f );
}



void DefaultExtension ( char *path, const char *extension )
{
	char    *src;
//
// if path doesnt have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while ( *src != '/' && *src != '\\' && src != path )
	{
		if ( *src == '.' )
			return;                 // it has an extension
		src--;
	}

	strcat( path, extension );
}


void    StripExtension ( char *path )
{
	int             length;

	length = strlen(path)-1;
	while( length > 0 && path[length] != '.' )
	{
		length--;
		if ( path[length] == '/' )
			return;		// no extension
	}
	if ( length )
		path[length] = '\0';
}


/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

// detect endianess
int BigEndian( void ) {
	union {
		int i;
		char s[4];
	} u;
	strcpy( u.s, "123" );
	if ( u.i == 0x00333231 )
		return 0;
	else
		return 1;
}

int LongSwap( int l )
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

//=======================================================

/*
============
CreatePath
============
*/
void	CreatePath (const char *path)
{
	const char	*ofs;
	char		c;
	char		dir[1024];

#ifdef _WIN32
	int		olddrive = -1;

	if ( path[1] == ':' )
	{
		olddrive = _getdrive();
		_chdrive( toupper( path[0] ) - 'A' + 1 );
	}
#endif

	if (path[1] == ':')
		path += 2;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		c = *ofs;
		if (c == '/' || c == '\\')
		{	// create the directory
			memcpy( dir, path, ofs - path );
			dir[ ofs - path ] = 0;
			Q_mkdir( dir );
		}
	}

#ifdef _WIN32
	if ( olddrive != -1 )
	{
		_chdrive( olddrive );
	}
#endif
}
