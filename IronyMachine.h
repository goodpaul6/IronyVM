#ifndef IRONY_MACHINE_H_
#define IRONY_MACHINE_H_

#include <stdint.h>

/* the number of registers available */
#define NUM_REGS 256

/* instruction codes */
#define HALT	0x00
#define LOAD	0x01
#define ADD		0x02
#define SUB		0x03
#define MUL		0x04
#define DIV		0x05
#define STO		0x06
#define CAL		0x07
#define LOADR	0x08
#define PRNT	0x09
#define GET		0x0A
#define ASL		0x0B
#define ASR		0x0C
#define JMP		0x0D
#define JMPF	0x0E
#define ALOC	0x0F
#define FREE	0x10
#define MSET	0x11

/* masks for instruction data */
#define INSTR_MASK	0xFF000000	// instruction value
#define NREG1_MASK	0xFF0000	// register number 1
#define NREG2_MASK	0xFF00		// register number 2
#define NREG3_MASK	0xFF		// register number 3
#define IMMVL_MASK	0xFFFF		// immediate value
#define LIMMV_MASK	0xFFFFFF	// long immediate value

/* allocated space */
#define ALLOC_MEM	0xFFFF	// allocate storage memory

/* max bound functions */
#define MAX_EXT_FUNC	0xFFFF

/* word */
#define WORD uint16_t		// machine independent word

/* encoding macro */
#define ENCODE(i, r1, r2, r3, v)	(i << 24 | r1 << 16 | r2 << 8 | r3 | v)

/* registers */
extern uintptr_t registers[NUM_REGS];

/* instruction and operands */
extern int inst;
extern int reg1;
extern int reg2;
extern int reg3;
extern int immd; // immediate value
extern int limd; // long immediate value (0xFFFFFF). This allows much larger values to be represented where required (at the tradeoff of register arguments)

/* memory (the user can store values here) */
extern uintptr_t memory[ALLOC_MEM];

/* the bound functions use the operand values to supplement their function (thus, they take no arguments) */
typedef void(*external_function)();

/* there is a limit of 0xFFFF external functions, as the maximum immediate value is 65535 */
extern external_function functions[MAX_EXT_FUNC];

/* a helper function for binding a function to an index */
void bind(const int idx, external_function function);

/* a helper function which runs a program given as an array of integers */
void run_program(const int* program);

/* a helper function which loads the program from a file */
int* load_from_file(const char* fileName);

#endif