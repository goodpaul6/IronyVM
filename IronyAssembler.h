#ifndef IRONY_ASSEMBLER_H_
#define IRONY_ASSEMBLER_H_

#include <stdio.h>

/* EXAMPLE PROGRAM: */
/* @start:
 * ldi 0 100
 * ldi 1 200
 * add 0 0 1
 * add 0 0 1
 * add 0 0 1
 * prt 0
 * jmp @start
*/

/* Maximum string length */
#define MAX_STR		512

/* Number of bytes read for instruction */
#define MNEM_BYTES	3

/* Number of bytes prior to instruction */
#define LABEL_BYTES 1

/* Modifiers */
#define HEX_MOD	"#"

/* Maximum bytes read for a number */
#define MAX_NUM_BYTES	8

/* Maximum value for a number */
#define MAX_NUM_VALUE	0xFFFFFF

 /* Maximum amount of labels */
 #define MAX_LABEL 0xFFFFFF

/* Instruction strings */
#define LOAD_INSTR	"ldi"
#define HALT_INSTR	"hlt"
#define ADD_INSTR	"add"
#define SUB_INSTR	"sub"
#define MUL_INSTR	"mul"
#define DIV_INSTR	"div"
#define STO_INSTR	"sto"
#define CALL_INSTR	"cal"
#define LODR_INSTR	"ldr"
#define PRNT_INSTR	"prt"
#define GET_INSTR	"get"
#define ASL_INSTR	"asl"
#define ASR_INSTR	"asr"
#define JMP_INSTR	"jmp"
#define JMPF_INSTR	"jmf"
#define ALOC_INSTR	"alc"
#define FREE_INSTR	"fre"
#define MSET_INSTR	"mst"

/* Possible token types */
typedef enum
{
	TOK_INSTR_OR_LABEL,
	TOK_LABEL,
	TOK_LDNUM,
	TOK_HEX
} TOKEN;

/* struct which holds the value of a token */
typedef struct
{
	TOKEN tok;
	char* str;
	int instr_num;
	double val;
} TOKEN_VALUE;

/* instruction number (for labeling) */
extern int instr_number;
/* global token vlaue for lexer */
extern TOKEN_VALUE token_value;
extern FILE* input_file;
extern FILE* output_file;
/* index = instruction #, char* = label name */
extern char* labels[MAX_LABEL];

/* opens files and initializes program */
int open_files(const char* inp, const char* out);
/* lexes a the token from input file (returns 0 on eof/done) */
int read_token();
/* parses and simultaneously converts lexed data to assembled bytecodes */
void parse_and_convert();

#endif
