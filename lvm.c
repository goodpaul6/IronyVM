#include "lvm.h"

#include <stdlib.h>
#include <string.h>

// push a value onto the virtual machine's stack
void lvm_stack_push(lvm_stack_t* stack, intptr_t value)
{
	stack->values[stack->position++] = value;
}

// pop a value from the virtual machine's stack
intptr_t lvm_stack_pop(lvm_stack_t* stack)
{
	if(stack->position > 0)
		return stack->values[--stack->position];
	else
		return stack->values[stack->position];
}

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
	vm->stack.position = 0;
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
	vm->reg1 = (vm->current & REG1_MASK) >> 20;
	vm->reg2 = (vm->current & REG2_MASK) >> 16;
	vm->reg3 = (vm->current & REG3_MASK) >> 12;
	vm->reg4 = (vm->current & REG4_MASK) >> 8;
	vm->immd = (vm->current & IMMVL_MASK);
	vm->limd = (vm->current & LIMMVL_MASK);
}

// evaluate the currently decoded instruction within the vm
void lvm_eval(lvm_t* vm)
{
	vm->regs[ZERO_REG] = 0;
	vm->regs[ESL_REG] = vm->stack.position;
	switch(vm->instr_num)
	{
	case HALT:
		vm->running = 0;
		vm->result = vm->regs[vm->reg1];
		break;
	case MOV:
		if(vm->debug)
			printf("mov\n");
		vm->regs[vm->reg1] = vm->immd;
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
	case JNZ:
		if(vm->debug)
			printf("jnz\n");
		if(vm->regs[vm->reg1] != 0)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->immd);
			vm->pc = vm->immd;
		}
		break;
	case JZ:
		if(vm->debug)
			printf("jz\n");
		if(vm->regs[vm->reg1] == 0)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->immd);
			vm->pc = vm->immd;
		}
		break;
	case JNE:
		if(vm->debug)
			printf("jne\n");
		if(vm->cmp1 != vm->cmp2)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->limd;
		}
		break;
	case JE:
		if(vm->debug)
			printf("je\n");
		if(vm->cmp1 == vm->cmp2)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->limd;
		}
		break;
	case JGT:
		if(vm->debug)
			printf("jgt\n");
		if(vm->cmp1 > vm->cmp2)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->limd;
		}
		break;
	case JLT:
		if(vm->debug)
			printf("jlt\n");
		if(vm->cmp1 < vm->cmp2)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->limd;
		}
		break;
	case JGE:
		if(vm->debug)
			printf("jge\n");
		if(vm->cmp1 >= vm->cmp2)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->limd;
		}
		break;
	case JLE:
		if(vm->debug)
			printf("jle\n");
		if(vm->cmp1 <= vm->cmp2)
		{
			lvm_jmp_jump(&vm->jmp_table, vm->pc, vm->limd);
			vm->pc = vm->limd;
		}
		break;
	case CMP:
		if(vm->debug)
			printf("cmp\n");
		vm->cmp1 = vm->regs[vm->reg1];
		vm->cmp2 = vm->regs[vm->reg2];
		break;
	case RET:
		if(vm->debug)
			printf("jbo\n");
		vm->pc = lvm_jmp_back(&vm->jmp_table, vm->limd);
		break;
	case MOVR:
		if(vm->debug)
			printf("movr\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg2];
		break;
	case CALL:
		if(vm->debug)
			printf("call\n");
		lvm_cint_call(&vm->cint, vm, vm->regs[vm->reg1]);
		break;
	case PUSH:
		if(vm->debug)
			printf("push\n");
		lvm_push(vm, vm->regs[vm->reg1]);
		break;
	case POP:
		if(vm->debug)
			printf("pop\n");
		vm->regs[vm->reg1] = lvm_pop(vm);
		break;
	case SET:
		if(vm->debug)
			printf("set\n");
		vm->db.values[vm->immd] = vm->regs[vm->reg1];
		break;
	case SETV:
		if(vm->debug)
			printf("setv\n");
		*(intptr_t*)(vm->regs[vm->reg1]) = vm->regs[vm->reg2]; 
		break;
	case GET:
		if(vm->debug)
			printf("get\n");
		vm->regs[vm->reg1] = vm->db.values[vm->immd];
		break;
	case GETA:
		if(vm->debug)
			printf("geta\n");
		vm->regs[vm->reg1] = (intptr_t)(&vm->db.values[vm->immd]);
		break;
	case DREF:
		if(vm->debug)
			printf("dref\n");
		vm->regs[vm->reg1] = *(intptr_t*)(vm->regs[vm->reg2]);
		break;
	case ASL:
		if(vm->debug)
			printf("asl\n");
		vm->regs[vm->reg1] <<= vm->regs[vm->reg2];
		break;
	case ASR:
		if(vm->debug)
			printf("asr\n");
		vm->regs[vm->reg1] >>= vm->regs[vm->reg2];
		break;
	case MASK:
		if(vm->debug)
			printf("mask\n");
		vm->regs[vm->reg1] = vm->regs[vm->reg1] & vm->regs[vm->reg2];
		break;
	case PUSHI:
		if(vm->debug)
			printf("pushi\n");
		lvm_push(vm, vm->immd);
		break;	
	}

	if(vm->debug)
		printf("instr performed at pc %u\n", vm->pc);
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

// push a value onto the stack of the vm
void lvm_push(lvm_t* vm, intptr_t value)
{
	lvm_stack_push(&vm->stack, value);
}

// pop a value from the stack of the vm
intptr_t lvm_pop(lvm_t* vm)
{
	return lvm_stack_pop(&vm->stack);
}

// close the vm
void lvm_close(lvm_t* vm)
{
	lvm_reset(vm);
}

// bound functions 

void lvm_fnmalloc(lvm_t* vm)
{
	vm->regs[vm->reg2] = (intptr_t)malloc((size_t)vm->reg3);
}

void lvm_fnfree(lvm_t* vm)
{
	free((void*)vm->regs[vm->reg2]);
}

void lvm_fnset(lvm_t* vm)
{
	memset((void*)vm->regs[vm->reg2], (int)vm->regs[vm->reg3], (size_t)vm->regs[vm->reg4]);
}

void lvm_fncpy(lvm_t* vm)
{
	memcpy((void*)vm->regs[vm->reg2], (void*)vm->regs[vm->reg3], (size_t)vm->regs[vm->reg4]);
}

void lvm_fntobyte(lvm_t* vm)
{
	vm->regs[vm->reg2] = *(uint8_t*)vm->regs[vm->reg2];
}

void lvm_fntodbyte(lvm_t* vm)
{
	vm->regs[vm->reg2] = *(uint16_t*)vm->regs[vm->reg2];
}

void lvm_fntoword(lvm_t* vm)
{
	vm->regs[vm->reg2] = *(uint32_t*)vm->regs[vm->reg2];
}

void lvm_fntodword(lvm_t* vm)
{
	vm->regs[vm->reg2] = *(uint64_t*)vm->regs[vm->reg2]; 
}

// end of bound functions

int main(int argc, char* argv[])
{
	if(argc == 2)
	{
		lvm_t vm;
		lvm_init(&vm);
		
		lvm_bind(&vm, &lvm_fnmalloc, 0);
		lvm_bind(&vm, &lvm_fnfree, 1);
		lvm_bind(&vm, &lvm_fnset, 2);
		lvm_bind(&vm, &lvm_fncpy, 3);
		lvm_bind(&vm, &lvm_fntobyte, 4);
		lvm_bind(&vm, &lvm_fntodbyte, 5);
		lvm_bind(&vm, &lvm_fntoword, 6);
		lvm_bind(&vm, &lvm_fntodword, 7);

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