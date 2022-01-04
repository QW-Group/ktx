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

#ifdef _WIN32
#include <stdio.h>
#include <windows.h>
#endif

#include "qvm.h" // <- that one should be synced with engine!
#include "cmdlib.h"

#define MYSORT
#define EMITJTS

// for symbols
#define SYM_HASHTABLE_SIZE 2048

// for opcodes/directives - minimize collisions
#define OP_HASHTABLE_SIZE 512

char	outputFilename[MAX_OS_PATH];

typedef enum {
	OP_UNDEF, 

	OP_IGNORE, 

	OP_BREAK, 

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,				// *(stack[top-1]) = stack[top]
	OP_ARG,
	OP_BLOCK_COPY,

	// -------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI,
	
	// additional directives

	DIR_PROC = 128,
	DIR_ENDPROC,
	DIR_ADDRESS,
	DIR_EXPORT,
	DIR_IMPORT,
	DIR_CODE,
	DIR_DATA,
	DIR_LIT,
	DIR_BSS,
	DIR_LINE,
	DIR_FILE,
	DIR_EQU,
	DIR_ALIGN,
	DIR_BYTE,
	DIR_SKIP

} opcode_t;

typedef enum { 
	PASS_DEFINE,
	PASS_COMPILE,
	PASS_OPTIMIZE,
	PASS_COUNT
} pass_t;


typedef enum { 
	ST_RESERVED,
	ST_FUNCTION,
	ST_LABEL
} symtype_t;


#define	MAX_IMAGE	0x400000

typedef struct segment_s {
	byte	image[MAX_IMAGE];
	int		imageUsed;
	int		segmentBase;		// only valid after PASS_DEFINE
} segment_t;


typedef struct symbol_s {
	struct	symbol_s	*next;
	struct  segment_s	*segment;
	char	*name;
	int		value;

	symtype_t type;
	int		refcnt;
	int		ignore;

	struct {
		char *name;
		int  line;
	} file;
} symbol_t;


typedef struct hashchain_s {
	void   *data;
	struct hashchain_s	*next;
} hashchain_t;


typedef struct hashtable_s {
	int buckets;
	int	chains;
	int	mask;
	hashchain_t	**table;
} hashtable_t;

int symtablelen = SYM_HASHTABLE_SIZE;

hashtable_t symtable;
hashtable_t optable;

byte		jbitmap[MAX_IMAGE];
segment_t	segment[NUM_SEGMENTS];
segment_t	*currentSegment;

pass_t		passNumber;
int			passCount = 0;
int			ignoreFunc = 0;
int			ignoreLabel = 0;

int			errorCount;

const char *passName[ PASS_COUNT ] = { "define",  "compile", "optimize" };

typedef struct options_s {
	qboolean verbose;
	qboolean writeMapFile;
	qboolean vanillaQ3Compatibility;
	qboolean optimize;
} options_t;

options_t options = { 0 };

symbol_t *entry      = NULL;  // QVM entry point
symbol_t *symbols    = NULL;
symbol_t *lastSymbol = NULL;  // Most recent symbol defined

#define	MAX_ASM_FILES	256

int		numAsmFiles;
char	*asmFiles[MAX_ASM_FILES];
char	*asmFileNames[MAX_ASM_FILES];

int		currentFileIndex;
char	*currentFileName;
int		currentFileLine;

int		stackSize = PROGRAM_STACK_SIZE; // 65536

// we need to convert arg and ret instructions to
// stores to the local stack frame, so we need to track the
// characteristics of the current functions stack frame

int		currentLocals;			// bytes of locals needed by this function
int		currentArgs;			// bytes of largest argument list called from this function
int		currentArgOffset;		// byte offset in currentArgs to store next arg, reset each call

#define	MAX_LINE_LENGTH	1024

char	lineBuffer[MAX_LINE_LENGTH];
int		lineParseOffset;
char	token[MAX_LINE_LENGTH];

int		instructionCount;

char	symExport[MAX_LINE_LENGTH];

#define ASMP(O) TryAssemble_##O
#define ASMF(O) int TryAssemble_##O( void )

typedef struct {
	char	*name;
	int		opcode;
	int	(*func)(void);
} sourceOps_t;

// declare prototypes

	ASMF(PROC);
	ASMF(ENDPROC);
	ASMF(ADDRESS);
	ASMF(EXPORT);
	ASMF(IMPORT);
	ASMF(CODE);
	ASMF(DATA);
	ASMF(LIT);
	ASMF(BSS);
	ASMF(LINE);
	ASMF(FILE);
	ASMF(EQU);
	ASMF(ALIGN);
	ASMF(BYTE);
	ASMF(SKIP);

	ASMF(ARG);
	ASMF(POP);
	ASMF(RET);
	ASMF(CALL);
	ASMF(ADDRL);
	ASMF(ADDRF);
	ASMF(LABEL);

sourceOps_t	sourceOps[] = 
{
	#include "opstrings.h"
};

#define	NUM_SOURCE_OPS ( sizeof( sourceOps ) / sizeof( sourceOps[0] ) )

#define JUSED(v) jbitmap[(v)/8]|=1<<((v)&7)

int vreport( const char* fmt, va_list vp )
{
	if ( options.verbose != qtrue )
		return 0;
	return vprintf(fmt, vp);
}

int report( const char *fmt, ... )
{
	va_list va;
	int retval;

	va_start( va, fmt );
	retval = vreport( fmt, va );
	va_end( va );
	return retval;
}

#ifdef EMITJTS

unsigned int crc_table[256];

void _crc32_init( unsigned int *crc )
{
	unsigned int c;
    int i, j;
    for (i = 0; i < 256; i++)
    {
        c = i;
        for (j = 0; j < 8; j++)
            c = c & 1 ? (c >> 1) ^ 0xEDB88320UL : c >> 1;
        crc_table[i] = c;
    };
    *crc = 0xFFFFFFFFUL;
}

void _crc32_update( unsigned int *crc, unsigned char *buf, unsigned int len )
{
    while (len--) 
        *crc = crc_table[(*crc ^ *buf++) & 0xFF] ^ (*crc >> 8);
}

void _crc32_final( unsigned int *crc )
{
	*crc = *crc ^ 0xFFFFFFFFUL;
}

#endif

int log2floor( int x ) 
{
	int r = 1;
	int v = x;
	while( v >>= 1 )
		r <<= 1;
	if ( r < x ) 
		r <<= 1;
	return r;
}

#define hashtable_get(H,hash) (H).table[(hash)&(H).mask]

/* The chain-and-bucket hash table.  -PH */

void hashtable_alloc( hashtable_t *H, int buckets )
{
	// round-up buckets so we can use mask correctly
	buckets = log2floor( buckets );
	H->buckets = buckets;
	H->mask = buckets - 1;
	H->chains = 0;
	H->table = malloc( buckets * sizeof( *(H->table) ) );
	memset( H->table, 0, buckets * sizeof( *(H->table) ) );
}


// By Paul Larson
unsigned int HashSymbol( const char *sym ) {
	unsigned int hash = 0;
	while( *sym ) 
		hash = 101 * hash + *sym++;
	return hash ^ (hash >> 16);
}


// Modified version, perfect hash for our limited directive set
unsigned int HashOpcode( const char *op ) {
	unsigned int hash = 0;
	while( *op ) 
		hash = 2039545 * hash + *op++;
	return hash ^ (hash >> 14);
//		hash = 6720353 * hash + *op++;
//	return hash ^ (hash >> 18);
}


void hashtable_add( hashtable_t *H, int hashvalue, void *data )
{
	hashchain_t *hc, **hb;

	hb = &H->table[ hashvalue & H->mask ];
	hc = malloc( sizeof( *hc ) );
	hc->data = data;
	hc->next = *hb;
	*hb = hc;
	H->chains++;
}

/* 
	There is no need in hashtable_remove routines or so since
	symbol/opcode tables is allocated once and used until program end
*/


/*
==============
InitTables
==============
*/
void InitTables( void ) {
	int i, hash;
	sourceOps_t *op;

	hashtable_alloc( &symtable, symtablelen );
	hashtable_alloc( &optable, OP_HASHTABLE_SIZE );
	op = sourceOps;
	for ( i = 0 ; i < NUM_SOURCE_OPS ; i++, op++ ) {
		hash = HashOpcode( op->name );
		hashtable_add( &optable, hash, op );
	}
}


void hashtable_stats( hashtable_t *H )
{
	int len, empties, longest, nodes;
	int i;
	float meanlen;
	hashchain_t *hc;

	empties = 0;
	longest = 0;
	nodes = 0;
	for ( i = 0; i < H->buckets; i++ ) 
	{
		if ( H->table[i] == NULL )
			{ empties++; continue; }
		for (hc = H->table[i], len = 0; hc; hc = hc->next, len++);
		if (len > longest) { longest = len; }
		nodes += len;
    }
	meanlen = (float)(nodes) / (H->buckets - empties);

	report( "Stats for %s hashtable:", (H==&symtable) ? "symbol" : "opcode" );
	report( " %d buckets (%i empty), %d nodes\n", H->buckets, empties, nodes );
	report( " longest chain: %d, mean non-empty: %f\n", longest, meanlen );
}


/* 
	Search for a symbol in corresponding hash table
	return NULL if not found 
*/
symbol_t *hashtable_symbol_exists( int hash, const char *sym )
{
	hashchain_t *hc;
	symbol_t *s;
	hc = hashtable_get( symtable, hash );
	while( hc != NULL ) {
		s = (symbol_t*)hc->data;
		if ( !strcmp( sym, s->name ) )
			return s;
		hc = hc->next;
	}
	return NULL; /* no matches */
}


#ifdef MYSORT
/*
    Quick symbols by its values
*/
static void sym_sort_v( symbol_t **a, int n ) 
{
	symbol_t *temp;
	symbol_t *m;
	int	i, j; 

	i = 0;
	j = n;
	m = a[ n>>1 ];

	do {
		// sort by values
		while ( a[i]->value < m->value ) i++;
		while ( a[j]->value > m->value ) j--;

		if ( i <= j ) {
			temp = a[i]; 
			a[i] = a[j]; 
			a[j] = temp;
			i++; 
			j--;
		}
  } while ( i <= j );

  if ( j > 0 ) sym_sort_v( a, j );
  if ( n > i ) sym_sort_v( a+i, n-i );
}
#else
/* Comparator function for quicksorting. */
static int symlist_cmp( const void *e1, const void *e2 )
{
	const symbol_t *a, *b;

	a = *(const symbol_t **)e1;
	b = *(const symbol_t **)e2;

	return ( a->value - b->value );
}
#endif


void sort_symbols( void )
{
	int      i, n;
	symbol_t *s;
	symbol_t **symlist;

	if ( symtable.chains <= 1 ) 
	{
		return; // nothing to sort actually
	}

	// alloc max possible buffer for storing all symbol pointers
	symlist = malloc( symtable.chains * sizeof( symbol_t* ) );

	// now count only used symbols
	n = 0;
	s = symbols;
	while ( s != NULL ) 
	{
		if ( !s->ignore ) 
		{
			symlist[n] = s;
			n++;
		}
		s = s->next;
	}

	// nothing to sort?
	if ( n <= 1 ) 
	{
		free( symlist );
		return;
	}

#ifdef _DEBUG	
	report( "Quick-sorting %d symbols\n", n );
#endif

#ifdef MYSORT
	sym_sort_v( symlist, n-1 );
#else
	qsort( symlist, n, sizeof(symbol_t*), symlist_cmp );
#endif

	// re-link chains
	s = symbols = symlist[0];
	for ( i = 1; i < n; i++ ) 
	{
      s->next = symlist[i];
      s = s->next;
    }

	lastSymbol = s;
	s->next = NULL;
	free( symlist );
}


/*
 Problem:
	BYTE values are specified as signed decimal string.  A properly functional
	atoip() will cap large signed values at 0x7FFFFFFF.  Negative word values are
	often specified as very large decimal values by lcc.  Therefore, values that
	should be between 0x7FFFFFFF and 0xFFFFFFFF come out as 0x7FFFFFFF when using
	atoi().  Bad.
*/
int atoiNoCap( const char *s ) 
{
	int		sign;
	int		c, v;

	if ( !s )
		return 0;

	// skip spaces
	while ( ( c = (*s & 255) ) <= ' ' ) 
	{
		if ( !c )
			return 0;
		s++;
	}

	// check sign
	switch ( c ) 
	{
		case '-':
			s++;
			sign = 1;
			break;
		case '+':
			s++;
		default:
			sign = 0;
			break;
	}

	v = 0; // resulting value

	for ( ;; ) 
	{
		c = *s++ & 255;
		if ( c < '0' || c > '9' )
			break;
		v = v * 10 + (c - '0');
	}

	if ( sign )
		v = -v;
	return v;
}


/*
============
CodeError
============
*/
void CodeError( char *fmt, ... ) {
	va_list		argptr;

	errorCount++;

	fprintf( stderr, "%s:%i ", currentFileName, currentFileLine );

	va_start( argptr,fmt );
	vfprintf( stderr, fmt, argptr );
	va_end( argptr );

}


/*
============
EmitByte
============
*/
void EmitByte( segment_t *seg, int v ) {
	if ( seg->imageUsed >= MAX_IMAGE ) {
		Error( "MAX_IMAGE" );
	}
	seg->image[ seg->imageUsed ] = v;
	seg->imageUsed++;
}


/*
============
EmitInt

Emit integer to corresponding segment in little-endian representation
============
*/
void EmitInt( segment_t *seg, int v ) {
	if ( seg->imageUsed >= MAX_IMAGE - 4) {
		Error( "MAX_IMAGE" );
	}
	seg->image[ seg->imageUsed ] = v & 255;
	seg->image[ seg->imageUsed + 1 ] = ( v >> 8 ) & 255;
	seg->image[ seg->imageUsed + 2 ] = ( v >> 16 ) & 255;
	seg->image[ seg->imageUsed + 3 ] = ( v >> 24 ) & 255;
	seg->imageUsed += 4;
}


symbol_t* FindSymbol( const char *sym ) {
	symbol_t	*s;
	hashchain_t *hc;
	hc = hashtable_get( symtable, HashSymbol( sym ) );
	while ( hc ) {
		s = (symbol_t*)hc->data;
		if ( !strcmp( sym, s->name ) )
			return s;
		hc = hc->next;
	}
	return NULL;
}


sourceOps_t *FindOpcode( const char *opcode ) {
	sourceOps_t *op;
	hashchain_t *hc;
	hc = hashtable_get( optable, HashOpcode( opcode ) );
	while ( hc ) {
		op = (sourceOps_t*)hc->data;
		if ( !strcmp( opcode, op->name ) )
			return op;
		hc = hc->next;
	}
	return NULL;
}


symbol_t* FindIgnoreSymbol( const char *sym ) {
	symbol_t	*s;
	hashchain_t *hc;
	hc = hashtable_get( symtable, HashSymbol( sym ) );
	while ( hc ) {
		s = (symbol_t*)hc->data;
		if ( s->ignore && !strcmp( sym, s->name ) 
			// strict checks
			&& s->file.name == currentFileName 
			&& s->file.line == currentFileLine )
			return s;
		hc = hc->next;
	}
	return NULL;
}


/*
============
IsIgnoredSymbol

returns non-NULL in case of ignored symbol
============
*/
symbol_t *IsIgnoredSymbol( const char *sym ) {
	char		buf[MAX_LINE_LENGTH]; 
	symbol_t	*s;
	
	if ( sym[0] == '$' ) {
		sprintf( buf, "%s_%i", sym, currentFileIndex );
		s = FindIgnoreSymbol( buf ); // local
		if ( s )
			return s;
	} else {
		sprintf( buf, "%s^%i", sym, currentFileIndex );
		s = FindIgnoreSymbol( buf ); // static
		if ( s )
			return s;
		s = FindIgnoreSymbol( sym ); // global
		if ( s )
			return s;
	}
	return NULL;
}


/* 
============
CheckIgnoredSymbols

check and set ignore flags for non-referenced symbols  
============
*/
int CheckIgnoredSymbols( void ) {
	symbol_t *s;
	int count = 0;
	for ( s = symbols ; s ; s = s->next ) {
		if ( s->ignore || s->type == ST_RESERVED )
			continue;
		if ( s->refcnt > 0 ) {
			s->refcnt = 0;  // reset for further optimization passes
			continue;
		}
		s->ignore = 1;
		count++;
	}
	return count;
}


/*
============
DefineSymbol

Symbols can only be defined on PASS_DEFINE
============
*/
void DefineSymbol( const char *sym, int value, symtype_t type, qboolean allowStatic ) {
	char		buf[MAX_LINE_LENGTH];
	symbol_t	*s;
	int			hash;

	if ( passNumber != PASS_DEFINE || ignoreFunc )
		return; // NULL

	// add the file suffix to local symbols to guarantee unique
	if ( sym[0] == '$' ) {
		sprintf( buf, "%s_%i", sym, currentFileIndex );
		sym = buf;
	} else 
	// special suffix for static symbols
	if ( allowStatic && ( *symExport == 0 || strcmp( sym, symExport ) != 0 ) ) {
		sprintf( buf, "%s^%i", sym, currentFileIndex );
		sym = buf;
	}

	*symExport = '\0'; // reset export symbol

	hash = HashSymbol( sym ); // save as hash value can be used later

	s = hashtable_symbol_exists( hash, sym );
	if ( s ) {
		// can happen on second PASS_DEFINE or PASS_OPTIMIZE
		if ( s->file.name != currentFileName || s->file.line != currentFileLine ) {
			// symbol exists but was defined elsewhere
			CodeError( "%s already defined (%s:%i)\n", sym, s->file.name, s->file.line );
		} else {
			s->value = value;
			s->segment = currentSegment;
			lastSymbol = s;
		}
		return;
	}

	s = malloc( sizeof( *s ) );
	s->next = NULL;
	s->name = copystring( sym );
	s->value = value;
	s->segment = currentSegment;

	// extra information
	s->type = type;
	s->refcnt = 0;
	s->ignore = 0;
	s->file.name = currentFileName;
	s->file.line = currentFileLine;

	hashtable_add( &symtable, hash, s );

/*
  Hash table lookup already speeds up symbol lookup enormously.
  We postpone sorting until end of pass 0.
  Since we're not doing the insertion sort, lastSymbol should always
   wind up pointing to the end of list.
  This allows constant time for adding to the list.
 -PH
*/
	if ( symbols == NULL ) {
		lastSymbol = symbols = s;
	} else {
		lastSymbol->next = s;
		lastSymbol = s;
	}

	return; //s
}


/*
============
LookupSymbol

Perform lookup and return symbol value
============
*/
int LookupSymbol( const char *sym ) {
	char		buf[MAX_LINE_LENGTH]; 
	symbol_t	*s;

	// can't operate at this stage
	if ( passNumber == PASS_DEFINE )
		return 0;

	 // ignore all symbol lookups inside ignore scope
	if ( ignoreFunc )
		return 0;
	
	// now perform lookup and set reference counters
	if ( sym[0] == '$' ) {
		sprintf( buf, "%s_%i", sym, currentFileIndex );
		s = FindSymbol( buf ); // local
		if ( s ) {
			s->refcnt++;
			return (s->segment->segmentBase + s->value);
		}
	} else {
		// look for static first
		sprintf( buf, "%s^%i", sym, currentFileIndex );
		s = FindSymbol( buf );
		if ( s ) {
			s->refcnt++;
			return (s->segment->segmentBase + s->value);
		}
		// now search as global
		s = FindSymbol( sym );
		if ( s ) {
			s->refcnt++;
			return (s->segment->segmentBase + s->value);
		}
	}

	CodeError( "error: symbol %s undefined\n", sym );
	return 0;
}



/*
==============
ExtractLine

Extracts the next line from the given text block.
If a full line isn't parsed, returns NULL
Otherwise returns the updated parse pointer
===============
*/
char *ExtractLine( char *data ) {
/* Goal:
	 Given a string `data', extract one text line into buffer `lineBuffer' that
	 is no longer than MAX_LINE_LENGTH characters long.  Return value is
	 remainder of `data' that isn't part of `lineBuffer'.
 -PH
*/
	/* Hand-optimized by PhaethonH */
	char 	*p, *q;

	currentFileLine++;

	lineParseOffset = 0;
	token[0] = 0;
	*lineBuffer = 0;

	p = q = data;
	if (!*q) {
		return NULL;
	}

	for ( ; !((*p == 0) || (*p == '\n')); p++)  /* nop */ ;

	if ((p - q) >= MAX_LINE_LENGTH) {
		CodeError( "MAX_LINE_LENGTH" );
		return data;
	}

	memcpy( lineBuffer, data, (p - data) );
	lineBuffer[(p - data)] = 0;
	p += (*p == '\n') ? 1 : 0;  /* Skip over final newline. */
	return p;
}
                                                        
/*
==============
Parse

Parse a token out of linebuffer
==============
*/
qboolean Parse( void ) {
	/* Hand-optimized by PhaethonH */
	const char 	*p, *q;

	/* Because lineParseOffset is only updated just before exit, this makes this code version somewhat harder to debug under a symbolic debugger. */

	*token = 0;  /* Clear token. */

	// skip whitespace
	for (p = lineBuffer + lineParseOffset; *p && (*p <= ' '); p++) /* nop */ ;

	// skip ; comments
	// die on end-of-string
	if ((*p == ';') || (*p == 0)) {
		lineParseOffset = p - lineBuffer;
		return qfalse;
	}

	q = p;  /* Mark the start of token. */
	/* Find separator first. */
	for ( ; *p > 32; p++) /* nop */ ;  /* XXX: unsafe assumptions. */
	/* *p now sits on separator.  Mangle other values accordingly. */
	strncpy(token, q, p - q);
	token[p - q] = 0;

	lineParseOffset = p - lineBuffer;

	return qtrue;
}


/*
==============
ParseValue
==============
*/
int	ParseValue( void ) {
	Parse();
	return atoiNoCap( token );
}


/*
==============
ParseExpression
==============
*/
int	ParseExpression( void ) {
	/* Hand optimization, PhaethonH */
	int		i, j;
	char	sym[MAX_LINE_LENGTH];
	int		v;

	/* Skip over a leading minus. */
	for ( i = ((token[0] == '-') ? 1 : 0) ; i < MAX_LINE_LENGTH ; i++ ) {
		if ( token[i] == '+' || token[i] == '-' || token[i] == 0 ) {
			break;
		}
	}

	memcpy( sym, token, i );
	sym[i] = 0;

	// resolve depending on first character
	switch (*sym) {  
		/* Optimizing compilers can convert cases into "calculated jumps".  I think these are faster.  -PH */
		case '-':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			v = atoiNoCap( sym );
			break;
		default:
			v = LookupSymbol( sym );
			break;
	}

	// parse add / subtract offsets
	while ( token[i] != 0 ) {
		for ( j = i + 1 ; j < MAX_LINE_LENGTH ; j++ ) {
			if ( token[j] == '+' || token[j] == '-' || token[j] == 0 ) {
				break;
			}
		}

		memcpy( sym, token+i+1, j-i-1 );
		sym[j-i-1] = 0;

		switch ( token[i] ) {
			case '+':
				v += atoiNoCap( sym );
				break;
			case '-':
				v -= atoiNoCap( sym );
				break;
		}

		i = j;
	}

	return v;
}


/*
==============
SwitchToSegment

BIG HACK: I want to put all 32 bit values in the data
segment so they can be byte swapped, and all char data in the lit
segment, but switch jump tables are emited in the lit segment and
initialized strng variables are put in the data segment.

I can change segments here, but I also need to fixup the
label that was just defined

Note that the lit segment is read-write in the VM, so strings
aren't read only as in some architectures.
==============
*/
void SwitchToSegment( segmentName_t seg ) {
	if ( currentSegment == &segment[seg] ) {
		return;
	}
	report( "%s %i: SwitchToSerment %i\n", currentFileName, currentFileLine, seg );
	currentSegment = &segment[seg];
	if ( passNumber == PASS_DEFINE ) {
		lastSymbol->segment = currentSegment;
		lastSymbol->value = currentSegment->imageUsed;
	}
}


// call instructions reset currentArgOffset
ASMF(CALL)
{
	EmitByte( &segment[CODESEG], OP_CALL );
	instructionCount++;
	currentArgOffset = 0;
	return 1;
}


// arg is converted to a reversed store
ASMF(ARG)
{
	EmitByte( &segment[CODESEG], OP_ARG );
	instructionCount++;
	if ( 8 + currentArgOffset >= 256 ) {
		CodeError( "currentArgOffset >= 256" );
		return 0;
	}
	EmitByte( &segment[CODESEG], 8 + currentArgOffset );
	currentArgOffset += 4;
	return 1;
}


// ret just leaves something on the op stack
ASMF(RET)
{
	EmitByte( &segment[CODESEG], OP_LEAVE );
	instructionCount++;
	EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );
	return 1;
}


// pop is needed to discard the return value of a function
ASMF(POP)
{
	EmitByte( &segment[CODESEG], OP_POP );
	instructionCount++;
	return 1;
}


// address of a parameter is converted to OP_LOCAL
ASMF(ADDRF)
{
	int		v;
	Parse();
	v = ParseExpression();
	instructionCount++;
	v = 16 + currentArgs + currentLocals + v;
	EmitByte( &segment[CODESEG], OP_LOCAL );
	EmitInt( &segment[CODESEG], v );
	return 1;
}


// address of a local is converted to OP_LOCAL
ASMF(ADDRL)
{
	int v;
	Parse();
	v = ParseExpression();
	instructionCount++;
	v = 8 + currentArgs + v;
	EmitByte( &segment[CODESEG], OP_LOCAL );
	EmitInt( &segment[CODESEG], v );
	return 1;
}


ASMF(PROC)
{
	char	name[1024];

	Parse();					// function name
	strcpy( name, token );

	// we are in optimizing stage(s)
	if ( ignoreFunc == 0 ) {
		if ( IsIgnoredSymbol( name ) ) {
			ignoreFunc++;
		}
	} else {
		// should never happen but anyway
		ignoreFunc++;
	}

	if ( ignoreFunc > 0 )
		return 1;

	if ( !entry ) {
		if ( strcmp( token, "vmMain" ) )
			printf( "Warning: entry point should be 'vmMain' instead of '%s'\n", token );
		DefineSymbol( token, instructionCount, ST_RESERVED, qtrue );
		entry = symbols;
	} else {
		DefineSymbol( token, instructionCount, ST_FUNCTION, qtrue );
	}

	currentLocals = ParseValue();	// locals
	currentLocals = ( currentLocals + 3 ) & ~3;
	currentArgs = ParseValue();		// arg marshalling
	currentArgs = ( currentArgs + 3 ) & ~3;

	if ( 8 + currentLocals + currentArgs >= 32767 ) {
		CodeError( "Locals > 32k in %s\n", name );
	}
	instructionCount++;
	EmitByte( &segment[CODESEG], OP_ENTER );
	EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );
	return 1;
}


ASMF(ENDPROC)
{
	//int		v, v2;
	
	if ( ignoreFunc > 0 ) {
		ignoreFunc--;
		return 1;
	}

	//Parse();				// skip the function name
	//v = ParseValue();		// locals
	//v2 = ParseValue();	// arg marshalling
	
	// all functions must leave something on the opstack
	instructionCount++;
	EmitByte( &segment[CODESEG], OP_PUSH );

	instructionCount++;
	EmitByte( &segment[CODESEG], OP_LEAVE );
	EmitInt( &segment[CODESEG], 8 + currentLocals + currentArgs );

	return 1;
}


ASMF(ADDRESS)
{
	int		v;

	Parse();
	v = ParseExpression();

	/* Addresses are 32 bits wide, and therefore go into data segment. */
	SwitchToSegment( DATASEG );

	EmitInt( currentSegment, v );
#if 0
	if ( passNumber == PASS_COMPILE && token[ 0 ] == '$' ) // crude test for labels 
	{
		EmitInt( &segment[ JTRGSEG ], v );
	}
#endif
	return 1;
}


ASMF(EXPORT)
{
	Parse(); // symbol name
	strcpy( symExport, token );
	return 1;
}


ASMF(IMPORT)
{
	return 1;
}


ASMF(CODE)
{
	currentSegment = &segment[CODESEG];
	return 1;
}


ASMF(BSS)
{
	currentSegment = &segment[BSSSEG];
	return 1;
}


ASMF(DATA)
{
	currentSegment = &segment[DATASEG];
	return 1;
}


ASMF(LIT)
{
	currentSegment = &segment[LITSEG];
	return 1;
}


ASMF(LINE)
{
	return 1;
}


ASMF(FILE)
{
	return 1;
}


ASMF(EQU)
{
	char	name[1024];

	Parse();
	strcpy( name, token );
	Parse();
	DefineSymbol( name, atoiNoCap( token ), ST_LABEL, qfalse );
	return 1;
}


ASMF(ALIGN)
{
	int		v;
	v = ParseValue();
	currentSegment->imageUsed = (currentSegment->imageUsed + v - 1 ) & ~( v - 1 );
	return 1;
}


ASMF(SKIP)
{
	int		v;
	v = ParseValue();

	currentSegment->imageUsed += v;
	return 1;
}


ASMF(BYTE)
{
	int	i, v, v2;

	v = ParseValue();	// size
	v2 = ParseValue();	// value

	if ( v == 1 ) {
		// Character (1-byte) values go into lit(eral) segment
		SwitchToSegment( LITSEG );
	} else if ( v == 4 ) {
		// 32-bit (4-byte) values go into data segment
		SwitchToSegment( DATASEG );
	} else {
		// and 16-bit (2-byte) values will cause q3asm to barf
		CodeError( "%i bit initialized data not supported", v*8 );
		return 0;
	}

	// emit little endian
	for ( i = 0 ; i < v ; i++ ) {
		EmitByte( currentSegment, (v2 & 255) ); /* paranoid ANDing  -PH */
		v2 >>= 8;
	}
	return 1;
}


// code labels are emited as instruction counts, not byte offsets,
// because the physical size of the code will change with
// different run time compilers and we want to minimize the
// size of the required translation table
ASMF(LABEL)
{
	Parse();

	// never ignore labels inside active code blocks
	if ( currentSegment != &segment[CODESEG] && IsIgnoredSymbol( token ) ) {
		ignoreLabel = 1;
		return 1;
	} else {
		ignoreLabel = 0;
	}

	if ( currentSegment == &segment[CODESEG] ) {
		if ( passNumber == PASS_COMPILE )
			JUSED( instructionCount );
		DefineSymbol( token, instructionCount, ST_LABEL, qtrue );
	} else {
		DefineSymbol( token, currentSegment->imageUsed, ST_LABEL, qtrue );
	}

	return 1;
}


/*
==============
AssembleLine
==============
*/
void AssembleLine( void ) {
	int		opcode;
	int		expression;
	sourceOps_t *op;

	Parse();
	if ( !token[0] ) {
		return;
	}

	op = FindOpcode( token );

#if 0
	hc = hashtable_get( optable, HashOpcode( token ) );
	while ( hc ) {
		op = (sourceOps_t*)(hc->data);
		if ( !strcmp( token, op->name ) )
			break;
		hc = hc->next;
	}
#endif

	if ( !op ) {
		CodeError( "Unknown token: %s\n", token );
		return;
	}

	opcode = op->opcode;

	if ( ignoreFunc ) {
		if ( ignoreLabel ) {
			CodeError( "ignore label in ignoreFunc scope" );
			ignoreLabel = 0;
		}
		switch( opcode ) {
			case DIR_PROC:
			case DIR_ENDPROC:
			//case DIR_ALIGN: 
			case DIR_CODE: // NEVER ignore segment directives!
			case DIR_LIT:
			case DIR_BSS:
			case DIR_DATA: 
				if ( op->func ) // execute only dedicated directives
					op->func();
			default:
				break;
		}
		return; // unconditionally return from this scope
	}

	if ( ignoreLabel ) {
		switch( opcode ) {
			case DIR_ADDRESS:
			case DIR_SKIP:
			case DIR_BYTE:
				return;
		}
		ignoreLabel = 0;
	}

	// execute opcode(directive) function and exit
	if ( op->func ) {
		op->func();
		return;
	}

	// we ignore most conversions
	if ( op->opcode == OP_IGNORE ) {
		return;
	}

	// sign extensions need to check next parm
	if ( opcode == OP_SEX8 ) {
		Parse();
		if ( token[0] == '1' ) {
			opcode = OP_SEX8;
		} else if ( token[0] == '2' ) {
			opcode = OP_SEX16;
		} else {
			CodeError( "Bad sign extension: %s\n", token );
			return;
		}
	}

	// check for expression
	Parse();
	if ( token[0] && opcode != OP_CVIF && opcode != OP_CVFI ) {
		expression = ParseExpression();
		// code like this can generate non-dword block copies:
		// auto char buf[2] = " ";
		// we are just going to round up.  This might conceivably
		// be incorrect if other initialized chars follow.
		if ( opcode == OP_BLOCK_COPY ) {
			expression = ( expression + 3 ) & ~3;
		}
		EmitByte( &segment[CODESEG], opcode );
		EmitInt( &segment[CODESEG], expression );
	} else {
		EmitByte( &segment[CODESEG], opcode );
	}

	instructionCount++;
}


/*
==============
WriteMapFile
==============
*/
void WriteMapFile( void ) {
	FILE		*f;
	symbol_t	*s;
	char		imageName[MAX_OS_PATH];
	int			seg;

	strcpy( imageName, outputFilename );
	StripExtension( imageName );
	strcat( imageName, ".map" );

	// Symbols list may be shrinked after sort because of ignored symbols
	// so we probably can't perform normal define/compile passes after that.
	// However, as we already created qvm file before - we can do anything now

	sort_symbols(); 

	report( "Writing %s...\n", imageName );

	f = SafeOpenWrite( imageName );
	for ( seg = CODESEG ; seg <= BSSSEG ; seg++ ) {
		for ( s = symbols ; s ; s = s->next ) {
			if ( s->name[0] == '$' ) {
				continue;	// skip locals
			}
			if ( &segment[seg] != s->segment ) {
				continue;
			}
			fprintf( f, "%i %8x %s\n", seg, s->value, s->name );
		}
	}
	fclose( f );
}


/*
===============
WriteVmFile
===============
*/
void WriteVmFile( void ) {
	char	imageName[MAX_OS_PATH];
	char	jtsName[MAX_OS_PATH];
	const char *errMsg;
	vmHeader_t	header;
	FILE	*f;
	unsigned int crc;
	int		i, headerSize;
	instruction_t *inst;

	strcpy( imageName, outputFilename );
	StripExtension( imageName );
	strcpy( jtsName, imageName );
	strcat( imageName, ".qvm" );
	strcat( jtsName, ".jts" );

	remove( imageName );
	remove( jtsName );

	if ( errorCount != 0 ) {
		report( "Not writing a file due to errors\n" );
		return;
	}

	report( "code segment: %7i\n", segment[CODESEG].imageUsed );
	report( "data segment: %7i\n", segment[DATASEG].imageUsed );
	report( "lit  segment: %7i\n", segment[LITSEG].imageUsed );
	report( "bss  segment: %7i\n", segment[BSSSEG].imageUsed );
	report( "instruction count: %i\n", instructionCount );

	if( !options.vanillaQ3Compatibility ) {
		header.vmMagic = VM_MAGIC_VER2;
		headerSize = sizeof( header );
	} else {
		header.vmMagic = VM_MAGIC;

		// Don't write the VM_MAGIC_VER2 bits when maintaining 1.32b compatibility.
		// (I know this isn't strictly correct due to padding, but then platforms
		// that pad wouldn't be able to write a correct header anyway).  Note: if
		// vmHeader_t changes, this needs to be adjusted too.
		headerSize = sizeof( header ) - sizeof( header.jtrgLength );
	}

	// Build jump table targets segment
	segment[JTRGSEG].imageUsed = 0;

	inst = ( instruction_t* ) malloc ( (instructionCount + 8) * sizeof( inst[0] ) );
	memset( inst, 0, (instructionCount + 8) * sizeof( inst[0] ) );
	
	errMsg = VM_LoadInstructions( segment[CODESEG].image, segment[CODESEG].imageUsed, instructionCount, inst );
	if ( errMsg ) {
		CodeError( "VM_LoadInstructions: %s\n", errMsg );
		exit(1);
	}

	errMsg = VM_CheckInstructions( inst, instructionCount, 0x7FFFFFFF );
	if ( errMsg ) {
		CodeError( "VM_CheckInstructions: %s\n", errMsg );
		exit(1);
	}

	//segment[JTRGSEG].segmentBase = segment[BSSSEG].segmentBase + segment[BSSSEG].imageUsed;
	for ( i = 0; i < instructionCount; i++ ) {
		if ( inst[i].jused ) {
			continue;
		}
		if ( jbitmap[i/8]&(1<<(i&7)) ) {
			EmitInt( &segment[JTRGSEG], i );
		}
	}
	segment[JTRGSEG].imageUsed = (segment[JTRGSEG].imageUsed + 3) & ~3;

	header.instructionCount = instructionCount;
	header.codeOffset = headerSize;
	header.codeLength = segment[CODESEG].imageUsed;
	header.dataOffset = header.codeOffset + segment[CODESEG].imageUsed;
	header.dataLength = segment[DATASEG].imageUsed;
	header.litLength = segment[LITSEG].imageUsed;
	header.bssLength = segment[BSSSEG].imageUsed;
	header.jtrgLength = segment[JTRGSEG].imageUsed;

	if ( BigEndian() ) {
		// byte swap the header
		for ( i = 0 ; i < sizeof( vmHeader_t ) / 4 ; i++ ) {
			((int *)&header)[i] = LongSwap( ((int *)&header)[i] );
		}
	}   

#ifdef EMITJTS

	// Prepare data for storing jump targets segment in external jts file.
	// With VM_MAGIC_VER2 we don't need that but at the same time
	// VM_MAGIC_VER2 terminates Q3 SDK EULA which may be not acceptable.
	// So we export jump targets into separate file while keeping main qvm
	// vq3-compatible. Host engine should be modified to accept jts files.
	// -----------------------------------------------------------------------
	// Jump targets is required by recent 1.32e engine builds to safely turn on 
	// bytecode optimizations which may provide significant performance boost 

	_crc32_init( &crc );
	_crc32_update( &crc, (void*)&header, sizeof( header ) - sizeof( header.jtrgLength ) );
	_crc32_update( &crc, (void*)&segment[CODESEG].image, segment[CODESEG].imageUsed );
	_crc32_update( &crc, (void*)&segment[DATASEG].image, segment[DATASEG].imageUsed );
	_crc32_update( &crc, (void*)&segment[LITSEG].image, segment[LITSEG].imageUsed );
	_crc32_final( &crc );

	// byte swap the sum
	if ( BigEndian() ) {
		for ( i = 0 ; i < sizeof( crc ) / 4 ; i++ ) {
			((int *)&crc)[i] = LongSwap( ((int *)&crc)[i] );
		}
	}   	
#endif

	report( "Writing to %s\n", imageName );

	CreatePath( imageName );
	f = SafeOpenWrite( imageName );
	SafeWrite( f, &header, headerSize );
	SafeWrite( f, &segment[CODESEG].image, segment[CODESEG].imageUsed );
	SafeWrite( f, &segment[DATASEG].image, segment[DATASEG].imageUsed );
	SafeWrite( f, &segment[LITSEG].image, segment[LITSEG].imageUsed );

	if ( !options.vanillaQ3Compatibility ) {
		SafeWrite( f, &segment[JTRGSEG].image, segment[JTRGSEG].imageUsed );
	}

	fclose( f );

#ifdef EMITJTS
	// write jump targets to separate file
	if ( options.vanillaQ3Compatibility ) 
	{
		CreatePath( jtsName );
		f = SafeOpenWrite( jtsName );
		SafeWrite( f, &crc, sizeof( crc ) );
		SafeWrite( f, &header.jtrgLength, sizeof( header.jtrgLength ) );
		SafeWrite( f, &segment[JTRGSEG].image, segment[JTRGSEG].imageUsed );
		fclose( f );
	}
#endif

}


void PassDefineCompile( void ) {
	char	*ptr;
	int		i;

	// clear potential garbage
	for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
		memset( segment[i].image, 0, MAX_IMAGE );
	}
	memset( jbitmap, 0, sizeof( jbitmap ) );

	for ( passNumber = PASS_DEFINE ; passNumber <= PASS_COMPILE ; passNumber++ ) {
		segment[LITSEG].segmentBase = segment[DATASEG].imageUsed;
		segment[BSSSEG].segmentBase = segment[LITSEG].segmentBase + segment[LITSEG].imageUsed;
		segment[JTRGSEG].segmentBase = segment[BSSSEG].segmentBase + segment[BSSSEG].imageUsed;

		for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
			segment[i].imageUsed = 0;
		}

		segment[DATASEG].imageUsed = 4;		// skip the 0 byte, so NULL pointers are fixed up properly
		instructionCount = 0;

		report( "pass #%i: %s\n", ++passCount, passName[ passNumber ] );

		for ( i = 0 ; i < numAsmFiles ; i++ ) {
			currentFileIndex = i;
			currentFileName = asmFileNames[ i ];
			currentFileLine = 0;

			fflush( NULL );
			ptr = asmFiles[i];
			ignoreFunc = 0;
			ignoreLabel = 0;
			while ( ptr ) {
				ptr = ExtractLine( ptr );
				AssembleLine();
			}
			if ( ignoreFunc ) {
				CodeError( "ignore level %i\n", ignoreFunc );
				return;
			}
			if ( errorCount ) {
				return;
			}
		}
		// align all segments
		for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
			segment[i].imageUsed = (segment[i].imageUsed + 3) & ~3;
		}
	}
}


void PassOptimize( void ) {
	char		*ptr;
	int			i;
	
	passNumber = PASS_OPTIMIZE;

	if ( !CheckIgnoredSymbols() )
		return;

	do {
		for ( i = 0 ; i < NUM_SEGMENTS ; i++ ) {
			segment[i].imageUsed = 0;
			segment[i].segmentBase = 0;
		}

		report( "pass #%i: %s\n", ++passCount, passName[ passNumber ] );

		for ( i = 0 ; i < numAsmFiles ; i++ ) {
			currentFileIndex = i;
			currentFileName = asmFileNames[ i ];
			currentFileLine = 0;
			fflush( NULL );
			ptr = asmFiles[i];
			ignoreFunc = 0;
			ignoreLabel = 0;
			while ( ptr ) {
				ptr = ExtractLine( ptr );
				AssembleLine();
			}
			if ( ignoreFunc ) {
				CodeError( "ignore level is not zero: %i\n", ignoreFunc );
				return;
			}
		}
	} while ( CheckIgnoredSymbols() > 0 );
}


/*
===============
Assemble
===============
*/
void Assemble( void ) {
	int			i;
	char		filename[MAX_OS_PATH];

	report( "Output filename: %s\n", outputFilename );

	for ( i = 0 ; i < numAsmFiles ; i++ ) {
		strcpy( filename, asmFileNames[ i ] );
		DefaultExtension( filename, ".asm" );
		LoadFile( filename, (void **)&asmFiles[i] );
	}

	PassDefineCompile();

	if ( options.optimize ) {
		// errors is unacceptable
		if ( errorCount )
			return;
		PassOptimize();
		if ( errorCount )
			return;
		lastSymbol = NULL;
		instructionCount = 0;
		currentSegment = NULL;
		for ( i = 0; i < NUM_SEGMENTS; i++ ) {
			segment[i].imageUsed = 0;
			segment[i].segmentBase = 0;
		}
		// perform final fixup
		PassDefineCompile();
	}

	passNumber = PASS_DEFINE;

	// reserve the stack in bss
	DefineSymbol( "_stackStart", segment[BSSSEG].imageUsed, ST_RESERVED, qfalse );
	segment[BSSSEG].imageUsed += stackSize;
	DefineSymbol( "_stackEnd", segment[BSSSEG].imageUsed, ST_RESERVED, qfalse );

	// write the image
	WriteVmFile();

	// write the map file even if there were errors
	if( options.writeMapFile ) {
		WriteMapFile();
	}
}


/*
=============
ParseOptionFile
=============
*/
void ParseOptionFile( const char *filename ) {
	char		expanded[MAX_OS_PATH];
	char		*text, *text_p;

	strcpy( expanded, filename );
	DefaultExtension( expanded, ".q3asm" );
	LoadFile( expanded, (void **)&text );
	if ( !text ) {
		return;
	}

	text_p = text;

	while( ( text_p = COM_Parse( text_p ) ) != 0 ) {
		if ( !strcmp( com_token, "-o" ) ) {
			// allow output override in option file
			text_p = COM_Parse( text_p );
			if ( text_p ) {
				strcpy( outputFilename, com_token );
			}
			continue;
		}

		asmFileNames[ numAsmFiles ] = copystring( com_token );
		numAsmFiles++;
	}
}


static const char *banner = {
	"Usage: %s [OPTION]... [FILES]...\n"
	"Assemble LCC bytecode assembly to Q3VM bytecode.\n"
	"\n"
	"    -o OUTPUT      Write assembled output to file OUTPUT.qvm\n"
	"    -f LISTFILE    Read options and list of files to assemble from LISTFILE\n"
	"    -b BUCKETS     Set symbol hash table to BUCKETS buckets\n"
	"    -r             Remove unreferenced symbols/functions from image\n"
	"    -m             Write map file\n"
	"    -v             Verbose compilation report\n"
	"    -vq3           Produce a qvm file compatible with Q3 1.32b\n"
	"    --             Stop switches parsing\n"
};


/*
==============
main
==============
*/
int main( int argc, const char *argv[] ) {
	int			i;
	double		start, end;
	const char	*bin;

	if ( argc < 2 ) {
		// strip binary name
		bin = strrchr( argv[0], PATH_SEP );
		if ( !bin || !bin[1] )
			bin = argv[0];
		else
			bin++;
		printf( banner, bin );
		return 0;
	}

	start = I_FloatTime ();

	// default filename is "q3asm"
	strcpy( outputFilename, "q3asm" );
	numAsmFiles = 0;	

	for ( i = 1 ; i < argc ; i++ ) {
		if ( argv[i][0] != '-' ) {
			break;
		}
		if ( !strcmp( argv[i], "-o" ) ) {
			if ( i == argc - 1 ) {
				Error( "-o must preceed a filename" );
			}
			/* Timbo of Tremulous pointed out -o not working; stock ID q3asm folded in the change. Yay. */
			strcpy( outputFilename, argv[ i+1 ] );
			i++;
			continue;
		}

		if ( !strcmp( argv[i], "-f" ) ) {
			if ( i == argc - 1 ) {
				Error( "-f must preceed a filename" );
			}
			ParseOptionFile( argv[ i+1 ] );
			i++;
			continue;
		}

		if ( !strcmp( argv[i], "-b" ) ) {
			if ( i == argc - 1 ) {
				Error( "-b requires an argument" );
			}
			i++;
			symtablelen = atoiNoCap( argv[ i ] );
			continue;
		}

		if ( !strcmp( argv[ i ], "-r" ) ) {
			options.optimize = qtrue;
			continue;
		}

/* 
		Verbosity option added by Timbo, 2002.09.14.
        By default (no -v option), q3asm remains silent except for critical errors.
        Verbosity turns on all messages, error or not.
        Motivation: not wanting to scrollback for pages to find asm error.
*/
		if( !strcmp( argv[ i ], "-v" ) ) {
			options.verbose = qtrue;
			continue;
		}

		if( !strcmp( argv[ i ], "-m" ) ) {
			options.writeMapFile = qtrue;
			continue;
		}

		if( !strcmp( argv[ i ], "-vq3" ) ) {
			options.vanillaQ3Compatibility = qtrue;
			continue;
		}

		if( !strcmp( argv[ i ], "--" ) ) {
			break;
		}

		Error( "Unknown option: %s", argv[ i ] );
	}

	// the rest of the command line args are asm files
	for ( ; i < argc ; i++ ) {
		asmFileNames[ numAsmFiles ] = copystring( argv[ i ] );
		numAsmFiles++;
	}

	InitTables();
	Assemble();

	// do not print stats/timings on error
	if ( errorCount ) {
		return errorCount;
	}

	if ( options.verbose ) {
		hashtable_stats( &symtable );
#ifdef _DEBUG
		hashtable_stats( &optable );
#endif
	}

	end = I_FloatTime();
	report( "%5.3f seconds elapsed\n---------------------\n", end-start );

	return 0;
}

