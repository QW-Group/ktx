/* 
	copied from qfiles.h, should be synced with host qvm format
	- made for standalone compilation
*/

#include "cmdlib.h"

#define	VM_MAGIC        0x12721444
#define	VM_MAGIC_VER2   0x12721445

#define PROGRAM_STACK_SIZE 0x10000

typedef struct {
	int		vmMagic;

	int		instructionCount;

	int		codeOffset;
	int		codeLength;

	int		dataOffset;
	int		dataLength;
	int		litLength;			// ( dataLength - litLength ) should be byteswapped on load
	int		bssLength;			// zero filled memory appended to datalength

	//!!! below here is VM_MAGIC_VER2 !!!
	int		jtrgLength;			// number of jump table targets
} vmHeader_t;


typedef enum {
	CODESEG,
	DATASEG,	// initialized 32 bit data, will be byte swapped
	LITSEG,		// strings
	BSSSEG,		// 0 filled
	JTRGSEG,	// psuedo-segment that contains only jump table targets
	NUM_SEGMENTS
} segmentName_t;

typedef struct {
	int			value;
	byte		op;
	byte		opStack;
	unsigned	jused:1;
} instruction_t;

const char *VM_LoadInstructions( const byte *code_pos, int codeLength, int instructionCount, instruction_t *buf );
const char *VM_CheckInstructions( instruction_t *buf, int instructionCount, int dataLength );
