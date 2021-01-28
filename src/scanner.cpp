#include "scanner.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include "log.h"

Scanner::Scanner() : warned(false), errored(false), line_number(0){}

bool Scanner::init(std::string &src_file) {
	warned = false;
	errored = false;
	line_number = 0;
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
	if (!src_fstream) {
		LOG(ERROR) << "Could not read from file.";
		t.type = TOK_EOF;
		return t;
	}
	t.type = TOK_EOF;
	t.val = "Need anime cat girl";
	return t;
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
				c_type_table[i] = C_PERIOD; break;
			case '_':
				c_type_table[i] = C_UNDER; break;
			case ';':
				c_type_table[i] = C_SEMICOL; break;
			case ':':
				c_type_table[i] = C_COLON; break;
			case ',':
				c_type_table[i] = C_COMMA; break;
			case '(':
				c_type_table[i] = C_LPAREN; break;
			case ')':
				c_type_table[i] = C_RPAREN; break;
			case '[':
				c_type_table[i] = C_LBRACK; break;
			case ']':
				c_type_table[i] = C_RBRACK; break;
			case '{':
				c_type_table[i] = C_LBRACE; break;
			case '}':
				c_type_table[i] = C_RBRACE; break;
			case '&':
			case '|':
				c_type_table[i] = C_EXPR; break;
			case '<':
			case '>':
			case '=':
			case '!':
				c_type_table[i] = C_RELAT; break;
			case '+':
			case '-':
				c_type_table[i] = C_ARITH; break;
			case '/':
				c_type_table[i] = C_FSLASH; break;
			case '\\':
				c_type_table[i] = C_BSLASH; break;
			case '*':
				c_type_table[i] = C_TERM; break;
			case '"':
				c_type_table[i] = C_QUOTE; break;
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				c_type_table[i] = C_WHITE; break;
			default:
				if (i >= 'a' && i <= 'z') {
					c_type_table[i] = C_LOWER;
				} else if (i >= 'A' && i <= 'Z') {
					c_type_table[i] = C_UPPER;
				}else if (i >= '0' && i <= '9') {
					c_type_table[i] = C_DIGIT;
				} else {
					c_type_table[i] = C_INVALID;
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

