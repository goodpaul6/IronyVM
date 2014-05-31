#include <stdio.h>
#include <stdlib.h>

#define MAX_FILENAME_LENGTH	256

char* get_file_as_string(const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if(!file)
		return NULL;
	fseek(file, 0L, SEEK_END);
	size_t sz = ftell(file);
	if(sz == 0)
		return NULL;
	rewind(file);
	char* buff = malloc(sizeof(char) * sz);
	int pos = 0;
	while(!feof(file))
	{
		buff[pos] = fgetc(file);
		pos += 1;
	}
	fclose(file);
	return buff;
}

int main(int argc, char* argv[])
{
	if(argc == 2)
	{
		FILE* file = fopen(argv[1], "r+");
		if(!file)
		{
			fprintf(stderr, "ERROR: Supplied file is invalid.\n");
			return 1;
		}
		
		int last = ' ';
		char fname_buf[MAX_FILENAME_LENGTH];
		int pos = 0;
		
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
				
				char* file_as_buf = get_file_as_string(fname_buf);
				fname_buf[0] = '\0';
				fprintf(file, "\n%s\n", file_as_buf);
				free(file_as_buf);
			}
			
			pos = 0;
		}
		
		fclose(file);
	}
	else
	{
		fprintf(stderr, "ERROR: Invalid amount of arguments supplied (program_name file_name)!\n");
		return 1;
	}

	return 0;
}