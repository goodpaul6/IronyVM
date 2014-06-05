#include <stdio.h>

/* maximum jump depth */
#define MAX_JUMP_DEPTH	0xFFFF

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
	int* program;			// 0-terminated program array
	int current;			// current instruction
	int running;			// is the vm running
	int result;				// resulting value (i.e main return value)
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
	vm->instr_num = (vm->current & INSTR_MASK) >> 12;
	vm->reg1 = (vm->current & REG1_MASK) >> 8;
	vm->reg2 = (vm->current & REG2_MASK) >> 4;
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
		vm->regs[vm->reg1] = vm->immd;
		break;
	case LOADR:
		vm->regs[vm->reg1] = vm->regs[vm->reg2];
		break;
	case ADD:
		vm->regs[vm->reg1] = vm->regs[vm->reg2] + vm->regs[vm->reg3];
		break;
	case SUB:
		vm->regs[vm->reg1] = vm->regs[vm->reg2] - vm->regs[vm->reg3];
		break;
	case MUL:
		vm->regs[vm->reg1] = vm->regs[vm->reg2] * vm->regs[vm->reg3];
		break;
	case DIV:
		vm->regs[vm->reg1] = vm->regs[vm->reg2] / vm->regs[vm->reg3];
		break;
	case NEG:
		vm->regs[vm->reg1] = -vm->regs[vm->reg1];
		break;
	case PRT:
		printf("%d", vm->regs[vm->reg1]);
		break;
	case PRTC:
		printf("%c", ((char)vm->regs[vm->reg1]));
		break;
	case JMP:
		lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
		vm->pc = vm->limd;
		break;
	case JMF:
		if(vm->regs[vm->reg1] == 0)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->immd;
		}
		break;
	case JBO:
		vm->pc = lvm_jmp_back(&vm->jmp_table, vm->limd);
		break;
	}
}

// load a program into the vm
void lvm_load(lvm_t* vm, int* program)
{
	if(vm->running) return;
	vm->program = program;
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
	int prog[] = 
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
	lvm_load(&vm, prog);
	return lvm_run(&vm);
}