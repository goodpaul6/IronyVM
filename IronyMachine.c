/* machine header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "IronyMachine.h"

/* running? */
bool running = false;

/* global values */
int esp;	// instruction pointer
uintptr_t registers[NUM_REGS];
int inst;
int reg1;
int reg2;
int reg3;
int immd;
int limd;
uintptr_t memory[ALLOC_MEM];
external_function functions[MAX_EXT_FUNC];

/* prototypes */
void decode(int encoded);
void process(bool debug);
int* load_from_file(const char* filename);

void bind(int idx, external_function function)
{
	if(idx >= MAX_EXT_FUNC) return;
	functions[idx] = function;
}

void decode(int encoded)
{
	inst = (encoded & INSTR_MASK) >> 24;
	reg1 = (encoded & NREG1_MASK) >> 16;
	reg2 = (encoded & NREG2_MASK) >> 8;
	reg3 = (encoded & NREG3_MASK);
	immd = (encoded & IMMVL_MASK);
	limd = (encoded & LIMMV_MASK);
}

void process(bool debug)
{
	switch(inst)
	{
	case HALT:
		running = false;
		break;
	case LOAD:
		if(debug)
			printf("ldi r%d #%d\n", reg1, immd);
		registers[reg1] = immd;
		break;
	case ADD:
		if(debug)
			printf("add r%d r%d r%d\n", reg1, reg2, reg3);
		registers[reg1] = registers[reg2] + registers[reg3];
		break;
	case SUB:
		if(debug)
			printf("sub r%d r%d r%d\n", reg1, reg2, reg3);
		registers[reg1] = registers[reg2] - registers[reg3];
		break;
	case MUL:
		if(debug)
			printf("mul r%d r%d r%d\n", reg1, reg2, reg3);
		registers[reg1] = registers[reg2] * registers[reg3];
		break;
	case DIV:
		if(debug)
			printf("div r%d r%d r%d\n", reg1, reg2, reg3);
		registers[reg1] = registers[reg2] / registers[reg3];
		break;
	case STO:
		if(debug)
			printf("sto m [r%d] r%d #%d\n", reg1, reg2, registers[reg2]);
		memory[registers[reg1]] = registers[reg2];
		break;
	case CAL:
		if(debug)
			printf("cal #%d\n", registers[reg1]);
		functions[registers[reg1]]();
		break;
	case LOADR:
		if(debug)
			printf("ldr r%d r%d #%d\n", reg1, reg2, registers[reg2]);
		registers[reg1] = registers[reg2];
		break;
	case PRNT:
		if(debug)
			printf("prnt r%d #%d\n\n", reg1, registers[reg1]);
		printf("%i", registers[reg1]);
		break;
	case GET:
		if(debug)
			printf("get r%d m [#%d] #%d\n", reg1, registers[reg2], memory[registers[reg2]]);
		registers[reg1] = memory[registers[reg2]];
		break;
	case ASL:
		if(debug)
			printf("asl r%d r%d #%d\n", reg1, reg2, registers[reg2]);
		registers[reg1] <<= registers[reg2];
		break;
	case ASR:
		if(debug)
			printf("asr r%d r%d #%d\n", reg1, reg2, registers[reg2]);
		registers[reg1] >>= registers[reg2];
		break;
	case JMP:
		if(debug)
			printf("jmp #%d\n", limd);
		esp = limd;
		break;
	case JMPF:
		if(debug)
			printf("jmpf r%d #%d #%d\n", reg1, registers[reg1], immd);
		if(registers[reg1] > 0)
			esp = immd;
		break;
	case ALOC:
		if(debug)
			printf("aloc r%d r%d m [#%d]\n", reg1, reg2, registers[reg2]);
		registers[reg2] = (uintptr_t)malloc(registers[reg1]);
		break;
	case FREE:
		if(debug)
			printf("free r%d #%d\n", reg1, registers[reg1]);
		free((void*)registers[reg1]);
		break;
	case MSET:
		if(debug)
			printf("mset r%d r%d r%d\n", reg1, reg2, reg3);
		uintptr_t i;
		for(i = registers[reg1]; i < registers[reg1] + registers[reg3]; i++)
			memory[i] = registers[reg2];
		break;
	};
}

int* load_from_file(const char* filename)
{
	int* program = NULL;
	char hexbuf[9];
	FILE* fp = fopen(filename, "r");
	if(!fp) return NULL;
	int size = 0;
	
	while(true)
	{
		size++;
		fread(hexbuf, 1, 8, fp);
		hexbuf[8] = 0;
		program = realloc(program, sizeof(int) * size);
		program[size - 1] = strtol(hexbuf, NULL, 16);
		fgetc(fp);
		if(feof(fp))
			break;
	}
	return program;
}

// prints out the character denoted by the value in register (denoted by reg2 value)
void m_printc()
{
	printf("%c", registers[reg2]);
}

// prints out a string at the memory location denoted by register (denoted by reg2 value) 
void m_prints()
{
	printf("%s", &memory[registers[reg2]]);
}

// prints out a string at the memory location denoted by register (denoted by reg2 value) of length register (denoted by reg3 value)
void m_printsl()
{
	printf("%.*s", registers[reg3], &memory[registers[reg2]]); 
}

// reads (from stdin) register (denoted by reg3 value) value bytes into memory location denoted by register (denoted by reg2 value)
void m_gets()
{
	int loc = registers[reg2];	// get the location where we must store the string
	fread(&memory[loc], 1, registers[reg3], stdin);	// read the string
	memory[loc + registers[reg3]] = 0;	// null terminate the string
}

void run_program(const int* program)
{
	if(running) return;
	running = true;
	esp = 0;
	while(running)
	{
		decode(program[esp]);
		process(true);
		esp += 1;
	}
}

int main(int argc, char* argv[])
{
	if(argc == 2)
	{
		bind(0, &m_printc);
		bind(1, &m_prints);
		bind(2, &m_printsl);
		bind(3, &m_gets);
		char* fname = argv[1];
		int* prog = load_from_file(argv[1]);
		run_program(prog);
		free(prog);
	}
	else
		printf("ERROR: Invalid arguments. Expected bytecode filename after program name.");
	return 0;
}