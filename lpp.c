#include <stdio.h>
#include <stdlib.h>

/*
$i "filename.lasm"
$m PUSH2 x y 
(
	push %-x
	push %-y
)

PUSH2 eax ecx
*/

/* the prefix which denotes a macro */
#define MACRO_PREFIX	'$'

/* the postfixes */
#define INCLUDE_POST		'i'		// inclusion
#define LINK_POST			'l'		// linking
#define DEFINE_POST			'd'		// defining
#define MACRO_POST			'm'		// macro

/* maximum token length (in characters) */
#define MAX_TOKEN_BUFFER_SIZE	256

/* maximum format string size */
#define MAX_EXPANSION_FORMAT_SIZE	512

/* maximum define value length */
#define MAX_DEFINE_VALUE_LENGTH	256

// token types
typedef enum
{
	TOKEN_INCLUDE,
	TOKEN_MACRO,
	TOKEN_ID,
	TOKEN_OB,
	TOKEN_CB,
	TOKEN_PASTE
} lpp_tokentype;


// token struct: holds token data
struct
{
	char buffer[MAX_TOKEN_BUFFER_SIZE];		// buffer which stores the data (whether the data is stored in here depends on )
	size_t length;							// length of the buffer (for faster operations)
	lpp_tokentype type;						// type of the token
} lpp_tokenval;

// macro data (basically a format for printf as well as arg amount, etc)
typedef struct 
{
	char format[MAX_EXPANSION_FORMAT_SIZE];
	char value[MAX_DEFINE_VALUE_LENGTH];		
	size_t args;
} lpp_macro_data_t;

// the preprocessor struct
struct
{
	FILE* input_file;		// input file pointer
	FILE* output_file; 		// output file pointer
	int last;				// last char
} lpp;

// reads a token from the input file
int lpp_read_token()
{
	if(!lpp.input_file) return 0;

	if(!feof(lpp.input_file))
	{
		while(isspace(lpp.last))
			lpp.last = fgetc(lpp.input_file);


	}

	return 0;
}
