#include "token.h"

#include <string>

////////////////////////////////////////////////////////////////////////////////
// Name lists
////////////////////////////////////////////////////////////////////////////////

const std::string Token::token_names[NUM_TOK_ENUMS] = {
	"INVALID",
	"PROGRAM",
	"IS",
	"BEGIN",
	"END",
	"GLOBAL",
	"PROCEDURE",
	"VARIABLE",
	"INTEGER",
	"FLOAT",
	"STRING",
	"BOOL",
	"IF",
	"THEN",
	"ELSE",
	"FOR",
	"RETURN",
	"NOT",
	"TRUE",
	"FALSE",
	"EXPRESSION",
	"ARITHMETIC",
	"RELATION",
	"ASSIGNMENT",
	"TERM",
	"IDENTIFIER",
	"VARIABLE_IDENTIFIER",
	"VARIABLE_PROCEDURE",
	"LITERAL_NUMBER",
	"LITERAL_STRING",
	"PERIOD",
	"COMMA",
	"SEMICOLON",
	"COLON",
	"L_PARENTHESIS",
	"R_PARENTHESIS",
	"L_BRACKET",
	"R_BRACKET",
	"END_OF_FILE"
};

const std::string Token::type_mark_names[NUM_TYPE_ENUMS] = {
	"NONE",
	"INT",
	"FLT",
	"STR",
	"BOOL",
};
