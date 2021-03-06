#ifndef LVM_H_
#define LVM_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/* maximum jump depth */
#define MAX_JUMP_DEPTH	0xFF

/* maximum amount of bound c functions */
#define MAX_BIND_AMT	0xFFFF

/* maximum stack depth */
#define MAX_STACK_DEPTH	0xFFFF

/* maximum amount of variables */
#define MAX_VARIABLE_AMT 0xFFFF

/* masks used to extract instruction values */
#define INSTR_MASK	0xFF000000
#define REG1_MASK	0x00F00000
#define REG2_MASK	0x000F0000
#define REG3_MASK	0x0000F000
#define REG4_MASK	0x00000F00
#define IMMVL_MASK	0x000FFFFF
#define LIMMVL_MASK	0x00FFFFFF

/* special registers */
#define ZERO_REG	0x0
#define ESL_REG		0xA

/* instruction defines */
#define HALT		0x00 		// hlt %eax
#define MOV			0x01 		// mov %eax 10
#define ADD			0x02 		// add %eax %gr1 %gr2
#define SUB			0x03 		// sub ...
#define MUL			0x04 		// mul ...
#define DIV			0x05 		// div ...
#define NEG 		0x06 		// neg %eax
#define PRT			0x07 		// prt %eax
#define PRTC		0x08 		// ptc %eax
#define JMP			0x09 		// jmp @label_name_here
#define JNZ			0x0A 		// jnz %eax @label_name_here
#define JZ			0x0B 		// jz ...
#define JNE			0x0C 		// jne @label_name_here
#define JE 			0x0D 		// je ...
#define JGT			0x0E 		// jgt ...
#define JLT 		0x0F 		// jlt ...
#define JGE			0x10 		// jge ...
#define JLE 		0x11 		// jle ...
#define CMP 		0x12 		// cmp %eax %gr1
#define RET 		0x13 		// ret @label_to_return_from_here
#define MOVR		0x14 		// movr %eax %gr1 
#define CALL 		0x15		// call %eax %zero %zero %zero
#define PUSH		0x16		// push %eax
#define POP			0x17 		// pop %eax
#define SET 		0x18		// set %eax #x 
#define SETV 		0x19		// setv %eax %gr1
#define GET 		0x20 		// get %eax #x
#define GETA		0x21 		// geta %eax #x
#define DREF		0x22 		// dref %eax %gr1
#define ASL			0x23 		// asl %eax %gr1
#define ASR 		0x24		// asr %eax %gr2
#define MASK		0x25 		// mask %eax %gr1
#define PUSHI		0x26 		// pushi 'a'

/* instruction encoding macros */
#define ENCODE_IRVV(instr, reg, immv)					((instr) << 24 | (reg) << 20 | (immv))
#define ENCODE_IRR0(instr, reg1, reg2)					((instr) << 24 | (reg1) << 20 | (reg2) << 16)
#define ENCODE_IRRR0(instr, reg1, reg2, reg3)			((instr) << 24 | (reg1) << 20 | (reg2) << 16 | (reg3) << 12)
#define ENCODE_IRRRV(instr, reg1, reg2, reg3, immv)		((instr) << 24 | (reg1) << 20 | (reg2) << 16 | (reg3) << 12 | (immv))
#define ENCODE_IVVV(instr, immv)						((instr) << 24 | (immv))
#define ENCODE_IR00(instr, reg)							((instr) << 24 | (reg) << 20)

/* number of registers */
#define NUM_REGS 	0xF

/* size of each instruction in characters */
#define INSTR_CHAR_LENGTH	0x8

struct lvm;
struct lvm_jmp;
struct lvm_prg_ldr;
struct lvm_cint;
struct lvm_stack;

/* word typedef */
typedef unsigned int word_t;

/* c function type */
typedef void(*lvm_cint_fn)(struct lvm*);

// database for storing variables
typedef struct lvm_database
{
	intptr_t values[MAX_VARIABLE_AMT];		// variable values
} lvm_database_t;

// virtual stack
typedef struct lvm_stack
{
	intptr_t values[MAX_STACK_DEPTH];		// stack values
	size_t position;						// position in the stack (0 indexed)
} lvm_stack_t;

// c interface system
typedef struct lvm_cint
{
	int occupied[MAX_BIND_AMT];						// used to prevent 2 functions from being bound to the same id
	lvm_cint_fn bound_functions[MAX_BIND_AMT];		// stores the bound functions in an array
} lvm_cint_t;

// program loader
typedef struct lvm_prg_ldr
{
	FILE* input_file;	// input file pointer
	char read_buf[9];	// buffer in which the instruction will be stored
	int last_char;		// last character read
} lvm_prg_ldr_t;

// jump table
typedef struct lvm_jmp
{
	size_t jmpfrom_locations[MAX_JUMP_DEPTH];	// locations jumped from
	size_t jmpto_locations[MAX_JUMP_DEPTH];		// locations jumped to
	size_t current_jmp_lvl;						// current jump lvl
} lvm_jmp_t;

// machine struct
typedef struct lvm
{
	size_t pc;				// program counter
	intptr_t cmp1;			// comparison value 1
	intptr_t cmp2;			// comparison value 2
	intptr_t regs[NUM_REGS];		// registers for storing values
	int instr_num;			// instruction argument
	int reg1;				// register argument 1
	int reg2;				// register argument 2
	int reg3;				// register argument 3
	int reg4;				// register argument 4
	int immd;				// immediate value
	int limd;				// long immediate value
	word_t* program;		// halt-terminated program array
	int current;			// current instruction
	int running;			// is the vm running
	int result;				// resulting value (i.e main return value)
	int should_free;		// whether the program should be freed from memory upon completion
	int debug;				// whether to debug the instructions
	lvm_prg_ldr_t loader;	// program loader (from file)
	lvm_jmp_t jmp_table;	// jump/branch table
	lvm_cint_t cint;		// c interface module
	lvm_stack_t stack;		// virtual stack instance
	lvm_database_t db;		// database
} lvm_t;

void lvm_close(lvm_t *vm);
void lvm_push(lvm_t *vm, intptr_t value);
intptr_t lvm_pop(lvm_t *vm);
int lvm_run(lvm_t *vm);
int lvm_read(lvm_t *vm,const char *filename);
void lvm_load(lvm_t *vm,word_t *program,int should_free);
void lvm_setdbg(lvm_t *vm,int value);
void lvm_reset(lvm_t *vm);
void lvm_overbind(lvm_t *vm,lvm_cint_fn fn,size_t id);
void lvm_bind(lvm_t *vm,lvm_cint_fn fn,size_t id);
void lvm_eval(lvm_t *vm);
void lvm_decode(lvm_t *vm);
void lvm_fetch(lvm_t *vm);
void lvm_init(lvm_t *vm);

void lvm_stack_push(lvm_stack_t *stack,intptr_t value);
intptr_t lvm_stack_pop(lvm_stack_t *stack);

size_t lvm_jmp_back(lvm_jmp_t *jmp,size_t pc);
void lvm_jmp_jump(lvm_jmp_t *jmp,size_t current_pc,size_t new_pc);
void lvm_jmp_init(lvm_jmp_t *jmp);

word_t *lvm_prg_ldr_read(lvm_prg_ldr_t *ldr);
int lvm_prg_ldr_loadf(lvm_prg_ldr_t *ldr,const char *filename);
void lvm_prg_ldr_init(lvm_prg_ldr_t *ldr);

void lvm_cint_call(lvm_cint_t *interface,lvm_t *vm,size_t id);
int lvm_cint_overbind(lvm_cint_t *interface,lvm_cint_fn fn,size_t id);
int lvm_cint_bind(lvm_cint_t *interface,lvm_cint_fn fn,size_t id);
void lvm_cint_init(lvm_cint_t *interface);

#endif