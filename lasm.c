#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* the maximum length of a register name */
#define MAX_REGCHARS	4

/* the maximum length of a mnemomic name */
#define MAX_MNEMCHARS	6

/* maximum token length */
#define MAX_TOKLEN		0xFFFF

/* register amount */
#define NUM_REGS		16

/* mnemonic amount */
#define NUM_MNEM		0x27

/* place this character before register references */
#define REGISTER_MOD	'%'

/* max length of a variable name */
#define MAX_VAR_LENGTH	256

/* max amount of variables */
#define MAX_VAR_AMT		0xFFFF

/* pushi opcode (strings depend on this) */
#define PUSHI_OPCODE 	0x26

/* the operand types */
typedef enum 
{
	OPTYPE_IRVVV,
	OPTYPE_IRR00,
	OPTYPE_IRRR0,
	OPTYPE_IRRRR,
	OPTYPE_IVVVV,
	OPTYPE_IR000,
} lasm_operand_type;

// variables database
struct lasm_vars_s
{
	char names[MAX_VAR_AMT][MAX_VAR_LENGTH];
	size_t length;
} lasm_vars;

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
	{"jlt", 0x0F, OPTYPE_IVVVV},
	{"jge", 0x10, OPTYPE_IVVVV},
	{"jle", 0x11, OPTYPE_IVVVV},
	{"cmp", 0x12, OPTYPE_IRR00},
	{"ret", 0x13, OPTYPE_IVVVV},
	{"movr", 0x14, OPTYPE_IRR00},
	{"call", 0x15, OPTYPE_IRRRR},
	{"push", 0x16, OPTYPE_IR000},
	{"pop", 0x17, OPTYPE_IR000},
	{"set", 0x18, OPTYPE_IRVVV},
	{"setv", 0x19, OPTYPE_IRR00},
	{"get", 0x20, OPTYPE_IRVVV},
	{"geta", 0x21, OPTYPE_IRVVV},
	{"dref", 0x22, OPTYPE_IRR00},
	{"asl", 0x23, OPTYPE_IRR00},
	{"asr", 0x24, OPTYPE_IRR00},
	{"mask", 0x25, OPTYPE_IRR00},
	{"pushi", 0x26, OPTYPE_IVVVV}
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
	{"esl", 10},		// stack length
	{"ecx", 11},		// loop counter
	{"gr1", 12},		
	{"gr2", 13},		
	{"gr3", 14}, 		
	{"gr4", 15}		
};

// token type enum
typedef enum
{
	TOKEN_LABEL,
	TOKEN_INTEGER,
	TOKEN_REGISTER,
	TOKEN_INSTR,
	TOKEN_STRING,
} lasm_tokentype;

// the token value struct
struct lasm_token_s 
{
	lasm_tokentype type;		// the type of the token
	char buffer[MAX_TOKLEN];	// the buffer which has a string representation of the token
	intptr_t integer;			// if the token was an integer, this holds the integer value of the token
	size_t pc;					// location in program
	size_t length;				// length of string in buffer
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
	lasm_symtable_t symbols;							// used for storing and querying labels
	FILE* input_file;									// input file
	FILE* output_file;									// output file
	int last;											// last character read
	int lineno;											// line number
	size_t start_pc;									// starting pc

} lasm;

// prototypes
void lasm_symtable_extend();

// if a variable exists, this returns its index in the hash, otherwise, it creates it
size_t lasm_variable_get(const char* name, size_t length)
{
	int found = 0;
	size_t index = 0;

	unsigned int i; for(i = 0; i < lasm_vars.length; i++)
	{
		if(!strcmp(lasm_vars.names[i], name))
		{	
			found = 1;
			index = i;
		}
	}

	if(!found)
	{
		index = lasm_vars.length;
		memcpy(lasm_vars.names[i], name, length * sizeof(char)); 
		++lasm_vars.length;
	}

	return index;
}

// initialize the assemblers symtable
void lasm_init_symtable()
{
	lasm.symbols.length = 0;
	lasm.symbols.capacity = 2;
	lasm.symbols.labels = malloc(sizeof(char*) * lasm.symbols.capacity);
	lasm.symbols.pcs = malloc(sizeof(size_t) * lasm.symbols.capacity);
}

// initialize the assembler
int lasm_init(const char* inp, const char* out, int append, size_t start)
{
	lasm.start_pc = start;
	lasm.input_file = fopen(inp, "r");
	if(!lasm.input_file) return 0;
	lasm.output_file = fopen(out, append ? "a+" : "w+");
	if(!lasm.output_file) return 0;
	lasm.last = ' ';
	lasm_tokenval.buffer[0] = '\0';
	lasm_tokenval.pc = start;
	lasm_tokenval.integer = 0;
	lasm.lineno = 1;
}

// close the assembler files
void lasm_close_files()
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
}

// close the assembler
void lasm_close()
{
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

// prints the symbol table
void lasm_symtable_debug()
{
	unsigned int i; for(i = 0; i < lasm.symbols.length; i++)
		printf("label %s at pc %u\n", lasm.symbols.labels[i], lasm.symbols.pcs[i]);
}

// handle escape sequences 
void lasm_handle_escape_seq()
{
	if(lasm.last == '\\')
	{
		lasm.last = fgetc(lasm.input_file);
		switch(lasm.last)
		{
			case 'n':
				lasm.last = '\n';
			break;
			case 'r':
				lasm.last = '\r';
			break;
			case 't':
				lasm.last = '\t';
			break;
			case '0':
				lasm.last = '\0';
			break;
			case '\'':
				lasm.last = '\'';
			break;
			case '"':
				lasm.last = '\"';
			break;
			case '\\':
				lasm.last = '\\';
			break;
		}
	}
}

// assemble a string into a sequence of stack instructions
void lasm_lex_string()
{
	lasm.last = fgetc(lasm.input_file);

	int pos = 0;
	while(lasm.last != '"')
	{
		lasm_handle_escape_seq();
		// each character amounts to one instruction
		++lasm_tokenval.pc;	
		lasm_tokenval.buffer[pos] = lasm.last;
		lasm.last = fgetc(lasm.input_file);
		++pos;
		lasm_tokenval.buffer[pos] = '\0';
	}
	lasm_tokenval.length = pos;
}	

// outputs a string as a series of instructions
void lasm_output_string(const char* str, size_t len)
{
	fprintf(lasm.output_file, "%02x%06x\n", PUSHI_OPCODE, '\0');

	int pos = len;

	while(pos >= 0)
	{
		int ch = str[pos];
		fprintf(lasm.output_file, "%02x%06x\n", PUSHI_OPCODE, ch);
		pos -= 1;
	}
}

// read a token from the assemblers input file
int lasm_read_token()
{
	if(lasm.input_file)
	{
		if(feof(lasm.input_file)) return 0;

		while(isspace(lasm.last))
		{
			if(lasm.last == '\n' || lasm.last == '\r') ++lasm.lineno;
			lasm.last = fgetc(lasm.input_file);
		}

		if(feof(lasm.input_file)) return 0;

		if(isalpha(lasm.last))
		{
			int pos = 0; 
			while(isalnum(lasm.last) || lasm.last == '_')
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
		else if(lasm.last == '"')
		{
			lasm_lex_string();
			lasm.last = fgetc(lasm.input_file);
			lasm_tokenval.type = TOKEN_STRING;
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
			while(isalnum(lasm.last) || lasm.last == '_')
			{
				lasm_tokenval.buffer[pos] = lasm.last;
				++pos;
				lasm_tokenval.buffer[pos] = '\0';
				lasm.last = fgetc(lasm.input_file);
			}
			lasm_tokenval.type = TOKEN_INTEGER;
			lasm_tokenval.integer = lasm_get_pc_from_label(lasm_tokenval.buffer);
		}
		else if(lasm.last == '#')
		{
			lasm.last = fgetc(lasm.input_file);
			int pos = 0;
			while(isalnum(lasm.last) || lasm.last == '_')
			{
				lasm_tokenval.buffer[pos] = lasm.last;
				++pos;
				lasm_tokenval.buffer[pos] = '\0';
				lasm.last = fgetc(lasm.input_file);
			}
			lasm_tokenval.type = TOKEN_INTEGER;
			lasm_tokenval.integer = lasm_variable_get(lasm_tokenval.buffer, pos);
		}
		else if(lasm.last == '\'')
		{
			lasm.last = fgetc(lasm.input_file);
			lasm_handle_escape_seq();
			lasm_tokenval.type = TOKEN_INTEGER;
			lasm_tokenval.integer = lasm.last;
			lasm.last= fgetc(lasm.input_file);
		}
		else if(lasm.last == ';')
		{
			lasm.last = fgetc(lasm.input_file);
			while(lasm.last != ';' && lasm.last != EOF)
				lasm.last = fgetc(lasm.input_file);
			lasm.last = fgetc(lasm.input_file);

			if(lasm.last == EOF) return 0;
		}
		else if(lasm.last == EOF) 
			return 0;

		return 1;
	}

	return 0;
}

// build the symbol table (optionally move the relocation program counter if reloc_pc is not null)
void lasm_symtable_build(size_t* reloc_pc)
{
	if(!lasm.input_file) return;

	int parse = lasm_read_token();

	while(parse)
	{
		if(lasm_tokenval.type == TOKEN_LABEL)
			lasm_symtable_put_label(lasm_tokenval.buffer, lasm_tokenval.pc);
		else if(lasm_tokenval.type == TOKEN_INSTR)
		{
			if(reloc_pc) *reloc_pc = (*reloc_pc + 1);
		}
		parse = lasm_read_token();
	}

	rewind(lasm.input_file);
	lasm_tokenval.buffer[0] = '\0';
	lasm_tokenval.pc = lasm.start_pc;
	lasm_tokenval.integer = 0;
	lasm.last = ' ';
	lasm.lineno = 1;
}

void lasm_expect_token_type(lasm_tokentype type)
{
	if(lasm_tokenval.type != type)
		fprintf(stderr, "ERROR: At line %i\nExpected token type %i, but received %i\n", lasm.lineno, type, lasm_tokenval.type);

	assert(lasm_tokenval.type == type);
}

// parse a token from the assemblers and spit an opcode into the file
int lasm_parse_token(size_t* instr)
{
	if(!lasm.output_file) return 0;

	int parse = lasm_read_token();

	if(!parse) return 0;

	if(lasm_tokenval.type == TOKEN_INSTR)
	{
		*instr = (*instr) + 1;
		int mnem_idx = lasm_get_mnem(lasm_tokenval.buffer);
		
		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRVVV)
		{
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_INTEGER);
			fprintf(lasm.output_file, "%02x%01x%05x\n", lasm_mnemdefs[mnem_idx].opcode, reg, lasm_tokenval.integer);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IR000)
		{
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x00000\n", lasm_mnemdefs[mnem_idx].opcode, reg);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IVVVV)
		{
			lasm_read_token();
			lasm_expect_token_type(TOKEN_INTEGER);
			fprintf(lasm.output_file, "%02x%06x\n", lasm_mnemdefs[mnem_idx].opcode, lasm_tokenval.integer);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRR00)
		{
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg2 = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x%01x0000\n", lasm_mnemdefs[mnem_idx].opcode, reg, reg2);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRRR0)
		{
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg2 = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg3 = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x%01x%01x000\n", lasm_mnemdefs[mnem_idx].opcode, reg, reg2, reg3);
		}

		if(lasm_mnemdefs[mnem_idx].optype == OPTYPE_IRRRR)
		{
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg2 = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg3 = lasm_get_reg(lasm_tokenval.buffer);
			lasm_read_token();
			lasm_expect_token_type(TOKEN_REGISTER);
			uint8_t reg4 = lasm_get_reg(lasm_tokenval.buffer);
			fprintf(lasm.output_file, "%02x%01x%01x%01x%01x00\n", lasm_mnemdefs[mnem_idx].opcode, reg, reg2, reg3, reg4);
		}
	}
	else if(lasm_tokenval.type == TOKEN_STRING)
		lasm_output_string(lasm_tokenval.buffer, lasm_tokenval.length);
	return 1;
}

int main(int argc, char* argv[])
{
	if(argc >= 3)
	{
		int append = 0;
		size_t reloc_pc = 0;

		lasm_init_symtable();
		
		// build symtable from both files
		unsigned int i; for(i = 2; i < argc; i++)
		{
			lasm_init(argv[i], argv[1], append, reloc_pc);
			lasm_symtable_build(&reloc_pc);
			lasm_close_files();
		}

		lasm_symtable_debug();

		// reset the relocation program counter
		reloc_pc = 0;

		// output the object files given the symbol data
		for(i = 2; i < argc; i++)
		{
			lasm_init(argv[i], argv[1], append, reloc_pc);
			while(lasm_parse_token(&reloc_pc));
			lasm_close_files();

			append = 1;
		}

		lasm_close();
		return 0;
	}
	fprintf(stderr, "invalid arguments to cmd line!\n");
	return 1;
}