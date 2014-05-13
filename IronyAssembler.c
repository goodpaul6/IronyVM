#include "IronyMachine.h"
#include "IronyAssembler.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

int instr_number = 0;
TOKEN_VALUE token_value;
FILE* input_file = NULL;
FILE* output_file = NULL;
char temp_buffer[MAX_STR];

int open_files(const char* inp, const char* out)
{
	input_file = fopen(inp, "r");
	
	if(!input_file)
		return -1;
	
	output_file = fopen(out, "w");
	
	if(!output_file)
		return -1;

	token_value.str = NULL;
}

int read_token()
{
	static int last = ' ';
	while(isspace(last))
		last = fgetc(input_file);

	if(last == '@')
	{
		last = fgetc(input_file);
		int pos = 0;
		while(isalpha(last))
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
		}
		last = fgetc(input_file);
		if(last == ':')
			token_value.tok = TOK_LABEL_DECL;
		else
			token_value.tok = TOK_LABEL_REF;
		token_value.instr_num = ++instr_number;
		if(token_value.str)
			free(token_value.str);
		token_value.str = strdup(temp_buffer);
		return;
	}
	else if(isalpha(last))
	{
		int pos = 0;
		while(isalpha(last))
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
		}
		last = fgetc(input_file);
		token_value.tok = TOK_INSTR;
		token_value.instr_num = ++instr_number;
		if(token_value.str)
			free(token_value.str);
		token_value.str = strdup(temp_buffer);
		return;
	}
	else if(isdigit(last))
	{
		int pos = 0;
		while(isdigit(last))
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
		}
		last = fgetc(input_file);
		token_value.tok = TOK_LDNUM;
		token_value.instr_num = ++instr_number;
		token_value.val = strtol(temp_buffer, NULL, 10);
		return;
	}
	else if(last == '#')
	{
		int pos = 0;
		last = fgetc(input_file);
		while(isdigit(last) || isalpha(last))
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
		}
		token_value.tok = TOK_HEX;
		token_value.instr_num = ++instr_number;
		token_value.val = strtol(temp_buffer, NULL, 16);
		return;
	}
	else if(last == '\n')
		read_token();
	else if(feof(input_file))
		return 0;
	return 1;
}

void write_arith_instr(int instr_val, int r_o, int r1, int r2)
{
	fprintf(output_file, "%02x%02x%02x%02x\n", instr_val, r_o, r1, r2);
}

void parse_and_convert()
{
	int w = read_token();
	while(w != 0)
	{
		if(token_value.tok == TOK_INSTR)
		{
			if(strcmp(token_value.str, LOAD_INSTR) == 0)
			{
				read_token();
				int next = token_value.val;
				read_token();
				int next2 = token_value.val;
				fprintf(output_file, "%02x%02x%04x\n", LOAD, next, next2);
			}

			if(strcmp(token_value.str, ADD_INSTR) == 0)
			{
				read_token();
				int r_o = token_value.val;
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				write_arith_instr(ADD, r_o, r1, r2);
			}

			if(strcmp(token_value.str, SUB_INSTR) == 0)
			{
				read_token();
				int r_o = token_value.val;
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				write_arith_instr(SUB, r_o, r1, r2);
			}

			if(strcmp(token_value.str, MUL_INSTR) == 0)
			{
				read_token();
				int r_o = token_value.val;
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				write_arith_instr(MUL, r_o, r1, r2);
			}

			if(strcmp(token_value.str, DIV_INSTR) == 0)
			{
				read_token();
				int r_o = token_value.val;
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				write_arith_instr(DIV, r_o, r1, r2);
			}
		}

		w = read_token();
	}
	fprintf(output_file, "\n00000000");
}

int main(int argc, char* argv[])
{
	if(argc == 3)
	{
		if(open_files(argv[1], argv[2]) == -1)
			return -1;

		parse_and_convert();

		free(token_value.str);
		fclose(input_file);
		fclose(output_file);
	}
}