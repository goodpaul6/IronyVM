#include <stdio.h>

typedef enum 
{
	TOK_INSTR,
	TOK_LABEL,
	TOK_LABEL_REF,
	TOK_NUMBER 
} lasm_token_t;

typedef struct
{
	FILE* input_file;
	FILE* output_file;
	int last;
} lasm_t;