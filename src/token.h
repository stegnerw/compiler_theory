#ifndef TOKEN_H
#define TOKEN_H

#include <sstream>

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
	TOK_PERIOD,		// .
	TOK_COMMA,		// ,
	TOK_SEMICOL,	// ;
	TOK_COLON,		// :
	TOK_LPAREN,		// (
	TOK_RPAREN,		// )
	TOK_LBRACK,		// [
	TOK_RBRACK,		// ]
	TOK_LBRACE,		// {
	TOK_RBRACE,		// }
	TOK_EOF			// End of file
};

class Token {
public:
	Token();
	virtual ~Token() {};
	Token(const token_type &t);
	virtual std::string const getStr();
	token_type const getType();
protected:
	static const std::string type_names[41];
	token_type type;
};

// TOK_IDs, TOK_OPs, and TOK_STRs (for now)
class StrToken : public Token {
public:
	StrToken(const token_type &t, const std::string &v);
	std::string const getVal();
	std::string const getStr();
protected:
	std::string val;
};

// TOK_INT
class IntToken : public Token {
public:
	IntToken(const token_type &t, const std::string &v);
	int const getVal();
	std::string const getStr();
protected:
	int val;
};

// TOK_FLT
class FltToken : public Token {
public:
	FltToken(const token_type &t, const std::string &v);
	double const getVal();
	std::string const getStr();
protected:
	double val;
};

#endif // TOKEN_H

