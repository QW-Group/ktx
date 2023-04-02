/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on QuakeWorld DM mod code by Id Software, Inc.
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
 *  $Id$
 */

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)		// truncation from const double to float
#pragma warning(disable : 4996)		// MSVS8 warnings about POSIX functions
#pragma warning(disable : 4311)		// pointer truncation
#pragma warning(disable : 4267)		// conversion from 'size_t' to 'int', possible loss of data
#endif

#if defined(__GNUC__)
#define PRINTF_FUNC( fmtargnumber ) __attribute__ (( format( __printf__, fmtargnumber, fmtargnumber+1 )))
#define SCANF_FUNC( fmtargnumber ) __attribute__ (( format( __scanf__, fmtargnumber, fmtargnumber+1 )))
#else
#define PRINTF_FUNC( fmtargnumber )
#define SCANF_FUNC( fmtargnumber )
#endif

/**********************************************************************
 VM Considerations

 The VM can not use the standard system headers because we aren't really
 using the compiler they were meant for.  We use bg_lib.h which contains
 prototypes for the functions we define for our own use in bg_lib.c.

 When writing mods, please add needed headers HERE, do not start including
 stuff like <stdio.h> in the various .c files that make up each of the VMs
 since you will be including system headers files can will have issues.

 Remember, if you use a C library function that is not defined in bg_lib.c,
 you will have to add your own version for support in the VM.

 **********************************************************************/

#ifdef Q3_VM

// QVM does not have such thing as visibility, using empty value to make compiler happy.
#define VISIBILITY_VISIBLE

#include "bg_lib.h"

#else

// Visibility for native library.
#ifdef _WIN32
	#define VISIBILITY_VISIBLE __declspec(dllexport)
#else
	#define VISIBILITY_VISIBLE __attribute__((visibility("default")))
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#if !defined( _WIN32 ) || !defined(_MSC_VER)
// so intptr_t is defined for all non MS compilers
#include <stdint.h>
#endif

#if defined( __linux__ ) || defined( _WIN32 ) /* || defined( __APPLE__ ) require?*/

// this is trap/syscalls, for popular functions like cos/sin/tan prototypes
// most likely alredy declared in above included headers, but not for below functions,
// because they are BSD originated, so we need declare it.

size_t strlcpy(char *dst, char *src, size_t siz);
size_t strlcat(char *dst, char *src, size_t siz);

#endif

// native_lib.c

#if defined( _WIN32 )

	int Q_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
	int snprintf(char *buffer, size_t count, char const *format, ...);

	#else

#define Q_vsnprintf vsnprintf

#endif // defined( _WIN32 )

#endif

#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		128			// max length of a filesystem pathname

#define	MAX_EDICTS		768			// FIXME: ouch! ouch! ouch!
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS		256			// these are sent over the net as bytes
#define	MAX_SOUNDS		256			// so they cannot be blindly increased
#define	MAX_STYLESTRING	64

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#define	QDECL

typedef unsigned char byte;
typedef enum
{
	false, true
} qbool;

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef char *string_t;
typedef intptr_t func_t;

typedef int stringref_t;
typedef int funcref_t;

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

//=============================================
short ShortSwap(short l);
int LongSwap(int l);
float FloatSwap(const float *f);

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct
{
	byte b0;
	byte b1;
	byte b2;
	byte b3;
	byte b4;
	byte b5;
	byte b6;
	byte b7;
} qint64;

//=============================================
/*
 short	BigShort(short l);
 short	LittleShort(short l);
 int		BigLong (int l);
 int		LittleLong (int l);
 qint64  BigLong64 (qint64 l);
 qint64  LittleLong64 (qint64 l);
 float	BigFloat (const float *l);
 float	LittleFloat (const float *l);

 void	Swap_Init (void);
 */

//=============================================
int Q_isprint(int c);
int Q_islower(int c);
int Q_isupper(int c);
int Q_isalpha(int c);

// portable case insensitive compare
int Q_stricmp(const char *s1, const char *s2);
int Q_strncmp(const char *s1, const char *s2, int n);
int Q_stricmpn(const char *s1, const char *s2, int n);
char* Q_strlwr(char *s1);
char* Q_strupr(char *s1);
char* Q_strrchr(const char *string, int c);

// buffer size safe library replacements
void Q_strncpyz(char *dest, const char *src, int destsize);
void Q_strcat(char *dest, int size, const char *src);

// strlen that discounts Quake color sequences
int Q_PrintStrlen(const char *string);
// removes color sequences from string
char* Q_CleanStr(char *string);

#endif	// __Q_SHARED_H
