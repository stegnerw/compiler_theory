#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <fstream>
#include <unordered_map>

enum c_type {
	C_INVALID,	// Anything not listed below - the default
	C_UPPER,	// [A-Z]
	C_LOWER,	// [a-z]
	C_DIGIT,	// [0-9]
	C_PERIOD,	// .
	C_UNDER,	// _
	C_SEMICOL,	// ;
	C_COLON,	// :
	C_COMMA,	// ,
	C_LPAREN,	// (
	C_RPAREN,	// )
	C_LBRACK,	// [
	C_RBRACK,	// ]
	C_LBRACE,	// {
	C_RBRACE,	// }
	C_EXPR,		// & |
	C_RELAT,	// < > = !
	C_ARITH,	// + -
	C_FSLASH,	// /
	C_BSLASH,	// [\]
	C_TERM,		// * (note / is technically term but is used in comments))
	C_QUOTE,	// "
	C_WHITE		// space, tab, newline, carriage return
};

/*
 * Token Types (from page 112):
 * Each keyword gets its own token type
 * Each operator class gets its own token type
 * One token type represents all identifiers
 * One token type for numerical constants (ints and floats)
 * One token type for string literals
 * Each punctuation gets its own token type
 */
enum token_type {
	TOK_INVALID,	// Invalid token - the default
	TOK_RW_PROG,	// program
	TOK_RW_IS,		// is
	TOK_RW_BEG,		// begin
	TOK_RW_END,		// end
	TOK_RW_GLOB,	// global
	TOK_RW_PROC,	// procedure
	TOK_RW_VAR,		// variable
	TOK_RW_TYPE,	// type
	TOK_RW_INT,		// integer
	TOK_RW_FLT,		// float
	TOK_RW_STR,		// string
	TOK_RW_BOOL,	// bool
	TOK_RW_ENUM,	// enum
	TOK_RW_IF,		// if
	TOK_RW_THEN,	// then
	TOK_RW_ELSE,	// else
	TOK_RW_FOR,		// for
	TOK_RW_RET,		// return
	TOK_RW_NOT,		// not
	TOK_RW_TRUE,	// true
	TOK_RW_FALSE,	// false
	TOK_OP_EXPR,	// & |
	TOK_OP_ARITH,	// + -
	TOK_OP_RELAT,	// < <= > >= == !=
	TOK_OP_ASS,		// :=
	TOK_OP_TERM,	// * /
	TOK_IDENT,		// Identifiers
	TOK_NUM,		// Numbers (float and int)
	TOK_STR,		// String literals: "[^"]"
	TOK_PER,		// .
	TOK_COMMA,		// ,
	TOK_SEMICOL,	// ;
	TOK_COL,		// :
	TOK_LPAREN,		// (
	TOK_RPAREN,		// )
	TOK_LBRACK,		// [
	TOK_RBRACK,		// ]
	TOK_LBRACE,		// {
	TOK_RBRACE,		// }
	TOK_EOF			// End of file reached
};

// I have a small brain and cannot get the union to work...
struct token {
	token_type type;
	std::string val;
	token() : type(TOK_INVALID), val("Uninitialized") {};
};

class Scanner {
	public:
		Scanner();
		bool init(std::string &src_file);
		token getToken();
	private:
		bool warned;
		bool errored;
		int line_number;
		c_type c_type_table[128];
		std::unordered_map<std::string, token_type> sym_tab;
		std::ifstream src_fstream;
		void reportWarn(const std::string &msg);
		void reportError(const std::string &msg);
		void makeCTab();
		void makeSymTab();
};

#endif // SCANNER_H

