#include <stdio.h>
#include <stdlib.h>

/* maximum jump depth */
#define MAX_JUMP_DEPTH	0xFF

/* masks used to extract instruction values */
#define INSTR_MASK	0xFF000000
#define REG1_MASK	0x00FF0000
#define REG2_MASK	0x0000FF00
#define REG3_MASK	0x000000FF
#define IMMVL_MASK	0x0000FFFF
#define LIMMVL_MASK	0x00FFFFFF

/* instruction defines */
#define HALT		0x0
#define LOADI		0x1
#define LOADR		0x2
#define ADD			0x3
#define SUB			0x4
#define MUL			0x5
#define DIV			0x6
#define NEG 		0x7
#define PRT			0x8
#define PRTC		0x9
#define JMP			0xA
#define JMF			0xB
#define JBO			0xC

/* instruction encoding macros */
#define ENCODE_IRVV(instr, reg, immv)			((instr) << 24 | (reg) << 20 | (immv))
#define ENCODE_IRRR(instr, reg1, reg2, reg3)	((instr) << 24 | (reg1) << 20 | (reg2) << 8 | (reg3))
#define ENCODE_IVVV(instr, immv)				((instr) << 24 | (immv))
#define ENCODE_IR00(instr, reg)					((instr) << 24 | (reg) << 20)

/* number of registers */
#define NUM_REGS 	0xF

/* size of each instruction in characters */
#define INSTR_CHAR_LENGTH	0x8

/* word typedef */
typedef int word_t;

// program loader
typedef struct
{
	FILE* input_file;	// input file pointer
	char read_buf[9];	// buffer in which the instruction will be stored
	int last_char;		// last character read
} lvm_prg_ldr_t;

// initialize the vm program loader
void lvm_prg_ldr_init(lvm_prg_ldr_t* ldr)
{
	ldr->input_file = NULL;
	ldr->last_char = ' ';
}

// load a file into the vm program loader (returns true if successful)
int lvm_prg_ldr_loadf(lvm_prg_ldr_t* ldr, const char* filename)
{
	ldr->input_file = fopen(filename, "rb");

	if(!ldr->input_file)
		return 0;
	return 1;
}	

// read a program from the loaded file (returns NULL if unsuccessful)
word_t* lvm_prg_ldr_read(lvm_prg_ldr_t* ldr)
{
	if(!ldr->input_file)
		return NULL;

	word_t* program = NULL;

	ldr->last_char = ' ';

	size_t program_capacity = 2;
	size_t program_length = 0;

	while(ldr->last_char != EOF)
	{
		program = realloc(program, sizeof(word_t) * program_capacity);
		while(isspace(ldr->last_char))
			ldr->last_char = fgetc(ldr->input_file);

		int pos = 0;
		while(isxdigit(ldr->last_char))
		{
			ldr->read_buf[pos] = ldr->last_char;
			++pos;
			ldr->read_buf[pos] = '\0';	
			ldr->last_char = fgetc(ldr->input_file);
		}

		if(pos == INSTR_CHAR_LENGTH)
		{
			program[program_length] = (word_t)strtol(ldr->read_buf, NULL, 16);
			++program_length;
			if(program_length >= program_capacity)
				program_capacity *= 2;
		}
	}

	return program;
}

// jump table
typedef struct
{
	size_t jmpfrom_locations[MAX_JUMP_DEPTH];	// locations jumped from
	size_t jmpto_locations[MAX_JUMP_DEPTH];		// locations jumped to
	size_t current_jmp_lvl;						// current jump lvl
} lvm_jmp_t;

// initialize a jump table
void lvm_jmp_init(lvm_jmp_t* jmp)
{
	jmp->current_jmp_lvl = 0;
}

// jump to an instruction and store the return location on the jump table
void lvm_jmp_jump(lvm_jmp_t* jmp, size_t current_pc, size_t new_pc)
{
	if(jmp->current_jmp_lvl > 0)
	{
		if(jmp->jmpfrom_locations[jmp->current_jmp_lvl - 1] == current_pc)	// if the location from which we are jumping is the location
			return;															// which was jumped from previously we do not need to add to the table
	}

	jmp->jmpfrom_locations[jmp->current_jmp_lvl] = current_pc;
	jmp->jmpto_locations[jmp->current_jmp_lvl] = new_pc;
	++jmp->current_jmp_lvl;
}

// get the pc from which we jumped to the location passed in as the pc
size_t lvm_jmp_back(lvm_jmp_t* jmp, size_t pc)
{
	size_t i; for(i = 0; i <= jmp->current_jmp_lvl; i++)
	{
		if(jmp->jmpto_locations[i] == pc)
		{
			jmp->current_jmp_lvl = i;
			return jmp->jmpfrom_locations[i];
		}
	}

	return -1;	// the pc passed in was never jumped to
}

// machine struct
typedef struct
{
	size_t pc;				// program counter
	int regs[NUM_REGS];		// registers for storing values
	int instr_num;			// instruction argument
	int reg1;				// register argument 1
	int reg2;				// register argument 2
	int reg3;				// register argument 3
	int immd;				// immediate value
	int limd;				// long immediate value
	word_t* program;		// 0-terminated program array
	int current;			// current instruction
	int running;			// is the vm running
	int result;				// resulting value (i.e main return value)
	int should_free;		// whether the program should be freed from memory upon completion
	int debug;				// whether to debug the instructions
	lvm_prg_ldr_t loader;	// program loader (from file)
	lvm_jmp_t jmp_table;	// jump/branch table
} lvm_t;

// initialize a vm 
void lvm_init(lvm_t* vm)
{
	vm->pc = 0;
	vm->instr_num = 0;
	vm->reg1 = 0;
	vm->reg2 = 0;
	vm->reg3 = 0;
	vm->immd = 0;
	vm->limd = 0;
	vm->program = NULL;
	vm->current = 0;
	vm->running = 0;
	vm->should_free = 0;
	vm->debug = 0;
	lvm_prg_ldr_init(&vm->loader);
	lvm_jmp_init(&vm->jmp_table);
}

// fetch and store the current instruction within the vm's loaded program
void lvm_fetch(lvm_t* vm)
{
	if(vm->program != NULL)
		vm->current = vm->program[vm->pc++];
}

// decode the currently loaded instruction within the vm
void lvm_decode(lvm_t* vm)
{
	vm->instr_num = (vm->current & INSTR_MASK) >> 24;
	vm->reg1 = (vm->current & REG1_MASK) >> 16;
	vm->reg2 = (vm->current & REG2_MASK) >> 8;
	vm->reg3 = (vm->current & REG3_MASK);
	vm->immd = (vm->current & IMMVL_MASK);
	vm->limd = (vm->current & LIMMVL_MASK);
}

// evaluate the currently decoded instruction within the vm
void lvm_eval(lvm_t* vm)
{
	switch(vm->instr_num)
	{
	case HALT:
		vm->running = 0;
		vm->result = vm->regs[vm->reg1];
		break;
	case LOADI:
		if(vm->debug)
			printf("loadi\n");
		vm->regs[vm->reg1] = vm->immd;
		break;
	case LOADR:
		if(vm->debug)
			printf("loadr\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg2];
		break;
	case ADD:
		if(vm->debug)
			printf("add\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg2] + vm->regs[vm->reg3];
		break;
	case SUB:
		if(vm->debug)
			printf("sub\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg2] - vm->regs[vm->reg3];
		break;
	case MUL:
		if(vm->debug)
			printf("mul\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg2] * vm->regs[vm->reg3];
		break;
	case DIV:
		if(vm->debug)
			printf("div\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg2] / vm->regs[vm->reg3];
		break;
	case NEG:
		if(vm->debug)
			printf("neg\n");
		vm->regs[vm->reg1] = -vm->regs[vm->reg1];
		break;
	case PRT:
		if(vm->debug)
			printf("prt\n");
		printf("%d", vm->regs[vm->reg1]);
		break;
	case PRTC:
		if(vm->debug)
			printf("prtc\n");
		printf("%c", ((char)vm->regs[vm->reg1]));
		break;
	case JMP:
		if(vm->debug)
			printf("jmp\n");
		lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
		vm->pc = vm->limd;
		break;
	case JMF:
		if(vm->debug)
			printf("jmf\n");
		if(vm->regs[vm->reg1] == 0)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->immd;
		}
		break;
	case JBO:
		if(vm->debug)
			printf("jbo\n");
		vm->pc = lvm_jmp_back(&vm->jmp_table, vm->limd);
		break;
	}
}

// resets a vm so that no program is loaded (if a program is loaded)
void lvm_reset(lvm_t* vm)
{
	if(vm->program)
	{
		if(vm->should_free)
			free(vm->program);
		lvm_init(vm);
	}
}

// set the debug flag in the vm
void lvm_setdbg(lvm_t* vm, int value)
{
	vm->debug = value;
}

// load a program into the vm
void lvm_load(lvm_t* vm, word_t* program, int should_free)
{
	if(vm->running) return;
	lvm_reset(vm);
	vm->program = program;
	vm->should_free = should_free;
}

// read a program from a file into the vm
void lvm_read(lvm_t* vm, const char* filename)
{
	if(vm->running) return;
	if(!lvm_prg_ldr_loadf(&vm->loader, filename)) return;
	word_t* program = lvm_prg_ldr_read(&vm->loader);
	if(!program) return;
	lvm_reset(vm);
	vm->program = program;
	vm->should_free = 1;
}

// run the currently loaded program on the vm
int lvm_run(lvm_t* vm)
{
	if(vm->running) return;
	vm->running = 1;

	while(vm->running)
	{
		lvm_fetch(vm);
		lvm_decode(vm);
		lvm_eval(vm);
	}

	vm->pc = 0;

	return vm->result;
}

int main(int argc, char* argv[])
{
	word_t prog[] = 
	{
		ENCODE_IRVV(LOADI, 0, 10),
		ENCODE_IVVV(JMP, 3),
		ENCODE_IR00(HALT, 0),
		ENCODE_IR00(PRT, 0),
		ENCODE_IRVV(LOADI, 0, 0),
		ENCODE_IVVV(JBO, 3)
	};

	lvm_t vm;
	lvm_init(&vm);
	lvm_load(&vm, prog, 0);
	return lvm_run(&vm);
}