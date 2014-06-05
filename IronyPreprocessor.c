#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_LENGTH	256
#define MAX_INCLUDE_AMT		256

char* included_files[MAX_INCLUDE_AMT];
int cur_include = 0;

void free_all_allocs();

int included_previously(const char* filename)
{
	unsigned int i; for(i = 0; i < MAX_INCLUDE_AMT; i++)
	{
		if(!included_files[i]) continue;
		if(strcmp(included_files[i], filename) == 0)
			return 1;
	}
	return 0;
}

char* get_file_as_string(const char* filename)
{
	if(process_file(filename, "temp_out"))
	{
		fprintf(stderr, "ERROR: Unable to process included file. Aborting...\n");
		free_all_allocs();
		abort();
	}
	
	FILE* file = fopen("temp_out", "r");
	if(!file)
		return NULL;
	fseek(file, 0L, SEEK_END);
	size_t sz = ftell(file);
	if(sz == 0)
		return NULL;
	rewind(file);
	char* buff = malloc(sizeof(char) * sz);
	int pos = 0;
	while(sz)
	{
		buff[pos] = fgetc(file);
		pos += 1;
		sz -= 1;
	}
	buff[pos - 1] = '\0';
	fclose(file);
	return buff;
}

int process_file(const char* inp_fname, const char* out_fname)
{
	FILE* file = fopen(inp_fname, "r");
	FILE* output = fopen(out_fname, "w");
	if(!file)
	{
		fprintf(stderr, "ERROR: Supplied file is invalid.\n");
		return 1;
	}
	
	if(!output)
	{
		fprintf(stderr, "ERROR: Not able to create output file.\n");
		return 1;
	}
	
	int last = ' ';
	char fname_buf[MAX_FILENAME_LENGTH];
	int pos = 0;
	
	included_files[cur_include] = strdup(inp_fname);
	
	while(!feof(file))
	{
		last = fgetc(file);

		if(last == '$')
		{
			last = fgetc(file);
			
			while(last != '$')
			{
				fname_buf[pos] = last;
				last = fgetc(file);
				pos += 1;
				fname_buf[pos] = '\0';
			}
				
			if(!included_previously(fname_buf))
			{
				printf("filename: %s\n", fname_buf);
			
				included_files[cur_include] = strdup(fname_buf);
				++cur_include;
				char* file_as_buf = get_file_as_string(fname_buf);
				fname_buf[0] = '\0';
			
				fputs(file_as_buf, output);
				printf("file buf: %s\n", file_as_buf);
				free(file_as_buf);
			}
			else
				printf("included previously: %s\n", fname_buf); 
		}
		else
			fputc(last, output);
		pos = 0;
	}
	
	fclose(file);
	fclose(output);
	return 0;
}

void free_all_allocs()
{
	unsigned int i; for(i = 0; i < MAX_INCLUDE_AMT; i++)
	{
		if(!included_files[i]) continue;
		else free(included_files[i]);
	}
	remove("temp_out");
}

int main(int argc, char* argv[])
{
	if(argc == 3)
	{
		unsigned int i; for(i = 0; i < MAX_INCLUDE_AMT; i++)
			included_files[i] = NULL;
		if(!process_file(argv[1], argv[2]))
		{
			free_all_allocs();
			return 1;
		}
		free_all_allocs();
	}
	else
	{
		fprintf(stderr, "ERROR: Invalid amount of arguments supplied (program_name input_file_name output_file_name)!\n");
		return 1;
	}

	return 0;
}