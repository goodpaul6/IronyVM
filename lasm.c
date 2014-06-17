#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* the maximum length of a register name */
#define MAX_REGCHARS	4

/* the maximum length of a mnemomic name */
#define MAX_MNEMCHARS	4

/* maximum token length */
#define MAX_TOKLEN		256

/* register amount */
#define NUM_REGS		16

/* mnemonic amount */
#define NUM_MNEM		0x3

/* place this character before register references */
#define REGISTER_MOD	'%'

/* the operand types */
typedef enum 
{
	OPTYPE_IRVVV,
	OPTYPE_IRR00,
	OPTYPE_IRRR0,
	OPTYPE_IVVVV,
	OPTYPE_IR000,
} lasm_operand_type;

// mnemonic struct
struct lasm_mnem_s
{
	char name[MAX_MNEMCHARS];
	uint8_t opcode;
	lasm_operand_type optype;
} lasm_mnemdefs[NUM_MNEM] = 
{
	{"hlt", 0x00, OPTYPE_IR000},
	{"mov", 0x01, OPTYPE_IRVVV},
	{"add", 0x02, OPTYPE_IRRR0},
	{"sub", 0x03, OPTYPE_IRRR0},
	{"mul", 0x04, OPTYPE_IRRR0},
	{"div", 0x05, OPTYPE_IRRR0},
	{"neg", 0x06, OPTYPE_IR000},
	{"prt", 0x07, OPTYPE_IR000},
	{"ptc", 0x08, OPTYPE_IR000},
	{"jmp", 0x09, OPTYPE_IVVVV},
	{"jnz", 0x0A, OPTYPE_IRVVV},
	{"jz", 0x0B, OPTYPE_IRVVV},
	{"jne", 0x0C, OPTYPE_IVVVV},
	{"je", 0x0D, OPTYPE_IVVVV},
	{"jgt", 0x0E, OPTYPE_IVVVV},
};

// register struct
struct lasm_regs_s
{
	char name[MAX_REGCHARS];
	uint8_t value;
} lasm_regdefs[NUM_REGS] = 
{
	{"zero", 0},		// always has the value 0
	{"eax", 1},			// global accumulator
	{"erx", 2},			// result value of the program
	{"eci", 3},			// call index when calling functions
	{"er1", 4},			// result 1
	{"er2", 5},			// result 2
	{"ea1", 6},			// argument 1
	{"ea2", 7},			// argument 2
	{"ea3", 8},			// argument 3
	{"epv", 9},			// for holding pointer values
	{"esp", 10},		// stack pointer
	{"ecx", 11},		// loop counter
	{"gr1", 12},		
	{"gr2", 13},		
	{"gr3", 14}, 		
	{"gr4", 15},		
};

// token type enum
typedef enum
{
	TOKEN_LABEL,
	TOKEN_INTEGER,
	TOKEN_REGISTER,
	TOKEN_INSTR
} lasm_tokentype;

// the token value struct
struct lasm_token_s 
{
	lasm_tokentype type;		// the type of the token
	char buffer[MAX_TOKLEN];	// the buffer which has a string representation of the token
	size_t integer;		// if the token was an integer, this holds the integer value of the token
	size_t pc;					// location in program
} lasm_tokenval;

// the symbol table struct
typedef struct
{
	size_t capacity;			// the current capacity of the labels array and the pcs array
	size_t length;				// the current length of the labels array and the pcs array
	char** labels;				// the labels array
	size_t* pcs;				// the program counter array
} lasm_symtable_t;

// the assembler struct
struct
{
	lasm_symtable_t symbols;	// used for storing and querying labels
	FILE* input_file;			// input file
	FILE* output_file;			// output file
	int last;					// last character read
} lasm;


// prototypes
void lasm_symtable_extend();

// initialize the assembler
int lasm_init(const char* inp, const char* out)
{
	lasm.input_file = fopen(inp, "r");
	if(!lasm.input_file) return 0;
	lasm.output_file = fopen(out, "w");
	if(!lasm.output_file) return 0;
	lasm.last = ' ';
	lasm_tokenval.buffer[0] = '\0';
	lasm_tokenval.pc = 0;
	lasm_tokenval.integer = 0;
	lasm.symbols.length = 0;
	lasm.symbols.capacity = 2;
	lasm.symbols.labels = malloc(sizeof(char*) * lasm.symbols.capacity);
	lasm.symbols.pcs = malloc(sizeof(size_t) * lasm.symbols.capacity);
}

// close the assembler
void lasm_close()
{
	if(lasm.input_file) 
	{
		fclose(lasm.input_file);
		lasm.input_file = NULL;
	}

	if(lasm.output_file)
	{
		fclose(lasm.output_file);
		lasm.output_file = NULL;
	}

	unsigned int i; for(i = 0; i < lasm.symbols.length; i++)
		free(lasm.symbols.labels[i]);

	free(lasm.symbols.pcs);
	free(lasm.symbols.labels);

	lasm.symbols.labels = NULL;
	lasm.symbols.pcs = NULL;
}

// gets register index given register name
int lasm_get_reg(const char* name)
{
	unsigned int i; for(i = 0; i < NUM_REGS; i++)
	{
		if(!strcmp(name, lasm_regdefs[i].name))
			return i;
	}

	fprintf(stderr, "ERROR: Attempted to access non-existent register (%s)\n", name);
	return -1;
}

// gets mnemonic index given mnem name
int lasm_get_mnem(const char* name)
{
	unsigned int i; for(i = 0; i < NUM_MNEM; i++)
	{
		if(!strcmp(name, lasm_mnemdefs[i].name))
			return i;
	}

	fprintf(stderr, "ERROR: Attempted to use non-existent mnemonic (%s)\n", name);
}

// gets instruction location given label name
size_t lasm_get_pc_from_label(const char* label)
{
	unsigned int i; for(i = 0; i < lasm.symbols.length; i++)
	{
		if(!strcmp(label, lasm.symbols.labels[i]))
			return lasm.symbols.pcs[i];
	}
	return 0;
}

// puts a label in the symbol table
void lasm_symtable_put_label(const char* label, size_t pc)
{
	lasm.symbols.labels[lasm.symbols.length] = strdup(label);
	lasm.symbols.pcs[lasm.symbols.length] = pc;
	++lasm.symbols.length;
	lasm_symtable_extend();
}

// private: extends the assemblers symbol table length
void lasm_symtable_extend()
{
	while(lasm.symbols.length >= lasm.symbols.capacity)
	{
		lasm.symbols.capacity *= 2;
		lasm.symbols.labels = realloc(lasm.symbols.labels, lasm.symbols.capacity * sizeof(char*));
		lasm.symbols.pcs = realloc(lasm.symbols.pcs, lasm.symbols.capacity * sizeof(size_t));
	}
}

// read a token from the assemblers input file
int lasm_read_token()
{
	if(lasm.input_file)
	{
		if(feof(lasm.input_file)) return 0;

		while(isspace(lasm.last))
			lasm.last = fgetc(lasm.input_file);

		if(feof(lasm.input_file)) return 0;

		if(isalpha(lasm.last))
		{
			int pos = 0; 
			while(isalnum(lasm.last))
			{
				lasm_tokenval.buffer[pos] = lasm.last;
				++pos;
				lasm_tokenval.buffer[pos] = '\0';
				lasm.last = fgetc(lasm.input_file);
			}

			if(lasm.last == ':')
			{
				lasm_tokenval.type = TOKEN_LABEL;
				lasm.last = fgetc(lasm.input_file);
			}
			else
			{
				++lasm_tokenval.pc; 
				lasm_tokenval.type = TOKEN_INSTR;
			}
		}
		else if(lasm.last == '%')
		{
			lasm.last = fgetc(lasm.input_file);
			int pos = 0;
			while(isalnum(lasm.last))
			{
				lasm_tokenval.buffer[pos] = lasm.last;
				++pos;
				lasm_tokenval.buffer[pos] = '\0';
				lasm.last = fgetc(lasm.input_file);
			}
			lasm_tokenval.type = TOKEN_REGISTER;
		}
		else if(isdigit(lasm.last))
		{
			int pos = 0;

			while(isdigit(lasm.last))
			{
				lasm_tokenval.buffer[pos] = lasm.last;
				++pos;
				lasm_tokenval.buffer[pos] = '\0';
				lasm.last = fgetc(lasm.input_file);
			}
			lasm_tokenval.type = TOKEN_INTEGER;
			lasm_tokenval.integer = strtol(lasm_tokenval.buffer, NULL, 10);
		}
		else if(lasm.last == '@')
		{
			lasm.last = fgetc(lasm.input_file);
			int pos = 0;
			while(isalnum(lasm.last))
			{
				lasm_tokenval.buffer[pos] = lasm.last;
				++pos;
				lasm_tokenval.buffer[pos] = '\0';
				lasm.last = fgetc(lasm.input_file);
			}
			lasm_tokenval.type = TOKEN_INTEGER;
			lasm_tokenval.integer = lasm_get_pc_from_label(lasm_tokenval.buffer);
		}
		else if(lasm.last == EOF) 
			return 0;

		return 1;
	}

	return 0;
}

// build the symbol table
void lasm_symtable_build()
{
	if(!lasm.input_file) return;

	int parse = lasm_read_token();

	while(parse)
	{
		if(lasm_tokenval.type == TOKEN_LABEL)
		{	
			lasm_symtable_put_label(lasm_tokenval.buffer, lasm_tokenval.pc);
			printf("found label: %s\n", lasm_tokenval.buffer);
		}
		parse = lasm_read_token();
	}

	rewind(lasm.input_file);
	lasm_tokenval.buffer[0] = '\0';
	lasm_tokenval.pc = 0;
	lasm_tokenval.integer = 0;
	lasm.last = ' ';
}

// parse a token from the assemblers and spit an opcode into the file
int lasm_parse_token()
{
	if(!lasm.output_file) return 0;

	int parse = lasm_read_token();

	if(!parse) return 0;

	if(lasm_tokenval.type == TOKEN_INSTR)
	{
		int mnem_idx = lasm_get_mnem(lasm_tokenval.buffer);
		
		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRVVV)
		{
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_INTEGER);
			fprintf(lasm.output_file, "%02x%01x%05x\n", lasm_mnemdefs[mnem_idx].opcode, reg, lasm_tokenval.integer);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IR000)
		{
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x00000\n", lasm_mnemdefs[mnem_idx].opcode, reg);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IVVVV)
		{
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_INTEGER);
			fprintf(lasm.output_file, "%02x%06x\n", lasm_mnemdefs[mnem_idx].opcode, lasm_tokenval.integer);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRR00)
		{
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg2 = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x%01x0000\n", lasm_mnemdefs[mnem_idx].opcode, reg, reg2);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRRR0)
		{
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg2 = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			assert(lasm_tokenval.type == TOKEN_REGISTER);
			uint8_t reg3 = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x%01x%01x000\n", lasm_mnemdefs[mnem_idx].opcode, reg, reg2, reg3);
		}
	}
	return 1;
}

int main(int argc, char* argv[])
{
	if(argc == 3)
	{
		lasm_init(argv[1], argv[2]);
		lasm_symtable_build();
		while(lasm_parse_token());
		lasm_close();
		return 0;
	}
	fprintf(stderr, "invalid arguments to cmd line!\n");
	return 1;
}