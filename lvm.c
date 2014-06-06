#include "lvm.h"

#include <stdlib.h>

// initialize the vm's c interface module
void lvm_cint_init(lvm_cint_t* interface)
{
	unsigned int i;
	for(i = 0; i < MAX_BIND_AMT; i++)
	{
		interface->occupied[i] = 0;
		interface->bound_functions[i] = NULL;
	}
}

// bind a function to the vm (returns true if successful)
int lvm_cint_bind(lvm_cint_t* interface, lvm_cint_fn fn, size_t id)
{
	if(id >= MAX_BIND_AMT) return 0;
	if(interface->occupied[id]) return 0;

	interface->bound_functions[id] = fn;
	interface->occupied[id] = 1;

	return 1;
}

// overwrite a previously bound function, if no previous function was bound, no overwrite occurs (returns true if successful)
int lvm_cint_overbind(lvm_cint_t* interface, lvm_cint_fn fn, size_t id)
{
	if(id >= MAX_BIND_AMT) return 0;
	if(!interface->occupied[id]) return 0;

	interface->bound_functions[id] = fn;

	return 1;
}

// call a bound function (does nothing if function doesn't exist)
void lvm_cint_call(lvm_cint_t* interface, lvm_t* vm, size_t id)
{
	if(id >= MAX_BIND_AMT) return;
	if(!interface->occupied[id]) return;

	if(!interface->bound_functions[id]) return;

	interface->bound_functions[id](vm);
}

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
	lvm_cint_init(&vm->cint);
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

// binds a c function to the lvm (warning supplied if unsuccessful)
void lvm_bind(lvm_t* vm, lvm_cint_fn fn, size_t id)
{
	if(!lvm_cint_bind(&vm->cint, fn, id))
		fprintf(stderr, "WARNING: Attempt to bind function to id [%u] failed!\n", id);
}

// overwrites a previously bound function in the lvm (warning supplied if unsuccessful)
void lvm_overbind(lvm_t* vm, lvm_cint_fn fn, size_t id)
{
	if(!lvm_cint_overbind(&vm->cint, fn, id))
		fprintf(stderr, "WARNING: Attempt to bind over a function bound at id [%u] failed!\n", id);
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
int lvm_read(lvm_t* vm, const char* filename)
{
	if(vm->running) return 0;
	if(!lvm_prg_ldr_loadf(&vm->loader, filename)) return 0;
	word_t* program = lvm_prg_ldr_read(&vm->loader);
	if(!program) return 0;
	lvm_reset(vm);
	vm->program = program;
	vm->should_free = 1;
	return 1;
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

// close the vm
void lvm_close(lvm_t* vm)
{
	lvm_reset(vm);
}

int main(int argc, char* argv[])
{
	if(argc == 2)
	{
		lvm_t vm;
		lvm_init(&vm);
		if(argv[1][0] == '-')
			lvm_setdbg(&vm, 1);
		if(!lvm_read(&vm, (argv[1][0] == '-') ? (++argv[1]) : argv[1]))
		{
			fprintf(stderr, "ERROR: Could not read file\n");
			return 1;
		}
		int res = lvm_run(&vm);
		lvm_close(&vm);
		return res;
	}

	fprintf(stderr, "ERROR: Invalid command line arguments (lvm program.path.here)\n");
	return 1;
}