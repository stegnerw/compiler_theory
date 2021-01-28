#include "scanner.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include "log.h"

Scanner::Scanner() : warned(false), errored(false), line_number(1) {}

Scanner::~Scanner() {
	if (src_fstream) src_fstream.close();
}

bool Scanner::init(std::string &src_file) {
	warned = false;
	errored = false;
	line_number = 1;
	src_fstream.open(src_file, std::ios::in);
	if (!src_fstream) {
		LOG(ERROR) << "Invalid file: " << src_file;
		LOG(ERROR) << "Make sure it exists and you have read"
			<< " permissions.";
		return false;
	}
	makeCTab();
	makeSymTab();
	return true;
}

token Scanner::getToken() {
	struct token t;
	std::string val = "";
	int c = src_fstream.get();
	while ((c_type_tab[c] == C_WHITE) || (isComment(c))) {
		eatWhiteSpace(c);
		eatLineComment(c);
		eatBlockComment(c);
	}
	if (c == EOF) {
		t.type	= TOK_EOF;
		t.val	= "EOF";
		return t;
	}
	c_type ct = c_type_tab[c];
	switch (ct) {
		// Alphanumerics (symbols: IDs and RWs)
		// Operators (Assignment handles colon)
		// Numerical constant
		// String literal
		// Punctuation
		case C_PERIOD:
			t.type	= TOK_PERIOD;
			t.val	= ".";
			break;
		case C_COMMA:
			t.type	= TOK_COMMA;
			t.val	= ",";
			break;
		case C_SEMICOL:
			t.type	= TOK_SEMICOL;
			t.val	= ";";
			break;
		// TOK_COLON is handled by TOK_OP_ASS since it starts with C_COLON
		case C_LPAREN:
			t.type	= TOK_LPAREN;
			t.val	= "(";
			break;
		case C_RPAREN:
			t.type	= TOK_RPAREN;
			t.val	= ")";
			break;
		case C_LBRACK:
			t.type	= TOK_LBRACK;
			t.val	= "[";
			break;
		case C_RBRACK:
			t.type	= TOK_RBRACK;
			t.val	= "]";
			break;
		case C_LBRACE:
			t.type	= TOK_LBRACE;
			t.val	= "{";
			break;
		case C_RBRACE:
			t.type	= TOK_RBRACE;
			t.val	= "}";
			break;
		default:
			reportError("Invalid character/token encountered");
			t.type	= TOK_INVALID;
			t.val	= static_cast<char>(c);
			break;
	}
	return t;
}

bool Scanner::isComment(int c) {
	return isLineComment(c) || isBlockComment(c);
}

bool Scanner::isLineComment(int c) {
	int next_c = src_fstream.peek();
	return (c == '/' && next_c == '/');
}

bool Scanner::isBlockComment(int c) {
	int next_c = src_fstream.peek();
	return (c == '/' && next_c == '*');
}

bool Scanner::isBlockEnd(int c) {
	int next_c = src_fstream.peek();
	return (c == '*' && next_c == '/');
}

void Scanner::eatWhiteSpace(int &c) {
	while ((c != EOF) && (c_type_tab[c] == C_WHITE)) {
		if (c == '\n') line_number++;
		c = src_fstream.get();
	}
}

void Scanner::eatLineComment(int &c) {
	if (isLineComment(c)) {
		do {
			c = src_fstream.get();
		} while ((c != '\n') && (c != EOF));
	}
}

void Scanner::eatBlockComment(int &c) {
	int block_level = 0;
	do {
		if (isBlockComment(c)) {
			block_level++;
			c = src_fstream.get();
		}
		if (isBlockEnd(c)) {
			block_level--;
			c = src_fstream.get();
		}
		if (c == '\n') line_number++;
		c = src_fstream.get();
	} while ((block_level > 0) && (c != EOF));
}

void Scanner::reportWarn(const std::string &msg) {
	warned = true;
	LOG(WARN) << "Line " << line_number << ":\t" << msg;
}

void Scanner::reportError(const std::string &msg) {
	errored = true;
	LOG(ERROR) << "Line " << line_number << ":\t" << msg;
}

void Scanner::makeCTab() {
	for (int i = 0; i < 128; i++) {
		switch (i) {
			case '.':
				c_type_tab[i] = C_PERIOD; break;
			case '_':
				c_type_tab[i] = C_UNDER; break;
			case ';':
				c_type_tab[i] = C_SEMICOL; break;
			case ':':
				c_type_tab[i] = C_COLON; break;
			case ',':
				c_type_tab[i] = C_COMMA; break;
			case '(':
				c_type_tab[i] = C_LPAREN; break;
			case ')':
				c_type_tab[i] = C_RPAREN; break;
			case '[':
				c_type_tab[i] = C_LBRACK; break;
			case ']':
				c_type_tab[i] = C_RBRACK; break;
			case '{':
				c_type_tab[i] = C_LBRACE; break;
			case '}':
				c_type_tab[i] = C_RBRACE; break;
			case '&':
			case '|':
				c_type_tab[i] = C_EXPR; break;
			case '<':
			case '>':
			case '=':
			case '!':
				c_type_tab[i] = C_RELAT; break;
			case '+':
			case '-':
				c_type_tab[i] = C_ARITH; break;
			case '/':
			case '*':
				c_type_tab[i] = C_TERM; break;
			case '"':
				c_type_tab[i] = C_QUOTE; break;
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				c_type_tab[i] = C_WHITE; break;
			default:
				if (i >= 'a' && i <= 'z') {
					c_type_tab[i] = C_LOWER;
				} else if (i >= 'A' && i <= 'Z') {
					c_type_tab[i] = C_UPPER;
				}else if (i >= '0' && i <= '9') {
					c_type_tab[i] = C_DIGIT;
				} else {
					c_type_tab[i] = C_INVALID;
				}
		}
	}
}

void Scanner::makeSymTab() {
	sym_tab["program"]		= TOK_RW_PROG;
	sym_tab["is"]			= TOK_RW_IS;
	sym_tab["begin"]		= TOK_RW_BEG;
	sym_tab["end"]			= TOK_RW_END;
	sym_tab["global"]		= TOK_RW_GLOB;
	sym_tab["procedure"]	= TOK_RW_PROC;
	sym_tab["variable"]		= TOK_RW_VAR;
	sym_tab["type"]			= TOK_RW_TYPE;
	sym_tab["integer"]		= TOK_RW_INT;
	sym_tab["float"]		= TOK_RW_FLT;
	sym_tab["string"]		= TOK_RW_STR;
	sym_tab["bool"]			= TOK_RW_BOOL;
	sym_tab["enum"]			= TOK_RW_ENUM;
	sym_tab["if"]			= TOK_RW_IF;
	sym_tab["then"]			= TOK_RW_THEN;
	sym_tab["else"]			= TOK_RW_ELSE;
	sym_tab["for"]			= TOK_RW_FOR;
	sym_tab["return"]		= TOK_RW_RET;
	sym_tab["not"]			= TOK_RW_NOT;
	sym_tab["true"]			= TOK_RW_TRUE;
	sym_tab["false"]		= TOK_RW_FALSE;
}

