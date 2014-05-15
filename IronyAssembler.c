#include "IronyMachine.h"
#include "IronyAssembler.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int instr_number = 0;
TOKEN_VALUE token_value;
FILE* input_file = NULL;
FILE* output_file = NULL;
char temp_buffer[MAX_STR];
char* labels[MAX_LABEL];
int last_reg = 0;

int open_files(const char* inp, const char* out)
{
	input_file = fopen(inp, "r");
	
	if(!input_file)
		return -1;
	
	output_file = fopen(out, "w");
	
	if(!output_file)
		return -1;

	token_value.instr_num = 0;
	token_value.str = NULL;

	unsigned int i; for(i = 0; i < MAX_LABEL; i++) labels[i] = NULL;
}

int get_instr_value_from_label(const char* label)
{
	unsigned int i; for(i = 0; i < MAX_LABEL; i++)
	{
		if(labels[i])
			if(strcmp(labels[i], label) == 0)
				return i;
	}
	printf("WARNING: There was an attempt to jump at a non existent label '%s'", label);
	return 0;
}

void output_string()
{
	int mem_pos = 0;
	char* c = token_value.str;
	fprintf(output_file, "%02x%02x%04x\n", LOAD, 254, last_reg);
	fprintf(output_file, "%02x%02x%04x\n", LOAD, 253, 1);
	while(*c)
	{
		fprintf(output_file, "%02xff%04x\n", LOAD, *c);
		fprintf(output_file, "%02x%02x%02x%02x\n", MSET, 254, 255, 253);
		c++;
		mem_pos++;
		fprintf(output_file, "%02x%02x%04x\n", LOAD, 254, last_reg + mem_pos);
	}
	fprintf(output_file, "%02x%02x%04x\n", LOAD, 255, 0);
	fprintf(output_file, "%02x%02x%02x%02x\n", MSET, 254, 255, 253);
}

int read_token()
{
	static int last = ' ';
	while(isspace(last))
		last = fgetc(input_file);

	if(last == ';')
	{
		last = fgetc(input_file);
		while(last != ';')
			last = fgetc(input_file);
		last = fgetc(input_file);
		read_token();
	}
	else if(isalpha(last))
	{
		int pos = 0;
		while(isalpha(last))
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
			temp_buffer[pos] = '\0';
		}

		if(last == ':')
		{
			token_value.tok = TOK_LABEL;
			token_value.instr_num = instr_number;
			last = fgetc(input_file);
			if(token_value.str)
				free(token_value.str);
			token_value.str = strdup(temp_buffer);
		}
		else
		{
			token_value.tok = TOK_INSTR_OR_LABEL;
			token_value.instr_num = instr_number++;
			if(token_value.str)
				free(token_value.str);
			token_value.str = strdup(temp_buffer);
		}
	}
	else if(isdigit(last))
	{
		int pos = 0;
		while(isdigit(last))
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
			temp_buffer[pos] = '\0';
		}
		last = fgetc(input_file);
		token_value.tok = TOK_LDNUM;
		token_value.instr_num = instr_number;
		token_value.val = strtol(temp_buffer, NULL, 10);
		last_reg = token_value.val;
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
			temp_buffer[pos] = '\0';
		}
		token_value.tok = TOK_HEX;
		token_value.instr_num = instr_number;
		token_value.val = strtol(temp_buffer, NULL, 16);
		last_reg = token_value.val;
	}
	else if(last == '"')
	{
		int pos = 0;
		last = fgetc(input_file);
		while(last != '"')
		{
			temp_buffer[pos] = last;
			last = fgetc(input_file);
			pos += 1;
			temp_buffer[pos] = '\0';
		}
		last = fgetc(input_file);
		token_value.tok = TOK_STR;
		token_value.instr_num = instr_number;
		if(token_value.str)
			free(token_value.str);
		token_value.str = strdup(temp_buffer);
	}
	else if(last == '\n')
		return read_token();
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
		if(token_value.tok == TOK_INSTR_OR_LABEL)
		{
			if(strcmp(token_value.str, LOAD_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int v = token_value.val;
				if(token_value.tok != TOK_STR)
					fprintf(output_file, "%02x%02x%04x\n", LOAD, r1, v);
				else
					output_string();
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

			if(strcmp(token_value.str, STO_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				fprintf(output_file, "%02x%02x%02x00\n", STO, r1, r2);
			}

			if(strcmp(token_value.str, CALL_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				read_token();
				int r3 = token_value.val;
				fprintf(output_file, "%02x%02x%02x%02x\n", CAL, r1, r2, r3);
			}

			if(strcmp(token_value.str, LODR_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				fprintf(output_file, "%02x%02x%02x00\n", LOADR, r1, r2);
			}

			if(strcmp(token_value.str, PRNT_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				fprintf(output_file, "%02x%02x0000\n", PRNT, r1);
			}

			if(strcmp(token_value.str, GET_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				fprintf(output_file, "%02x%02x%02x00\n", GET, r1, r2);
			}

			if(strcmp(token_value.str, ASL_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				fprintf(output_file, "%02x%02x%02x00\n", ASL, r1, r2);
			}

			if(strcmp(token_value.str, ASR_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				fprintf(output_file, "%02x%02x%02x00\n", ASR, r1, r2);
			}

			if(strcmp(token_value.str, JMP_INSTR) == 0)
			{
				read_token();
				int esp = get_instr_value_from_label(token_value.str);
				fprintf(output_file, "%02x%06x\n", JMP, esp);
			}

			if(strcmp(token_value.str, JMPF_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int esp = get_instr_value_from_label(token_value.str);
				fprintf(output_file, "%02x%02x%04x\n", JMPF, r1, esp);
			}

			if(strcmp(token_value.str, ALOC_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				fprintf(output_file, "%02x%02x%02x00\n", ALOC, r1, r2);
			}

			if(strcmp(token_value.str, FREE_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				fprintf(output_file, "%02x%02x0000\n", FREE, r1);
			}

			if(strcmp(token_value.str, MSET_INSTR) == 0)
			{
				read_token();
				int r1 = token_value.val;
				read_token();
				int r2 = token_value.val;
				read_token();
				int r3 = token_value.val;
				fprintf(output_file, "%02x%02x%02x%02x", MSET, r1, r2, r3);
			}
		}
		else if(token_value.tok == TOK_LABEL)
		{
			// TODO: FIX JUMPING BUG (CURRENTLY HACK FIXED)
			if(token_value.instr_num > 0) token_value.instr_num -= 1;
			labels[token_value.instr_num] = strdup(token_value.str);
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