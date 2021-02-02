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
		LOG::ss << "Invalid file: " << src_file;
		LOG(LOG_LEVEL::ERROR);
		LOG::ss << "Make sure it exists and you have read permissions.";
		LOG(LOG_LEVEL::ERROR);
		return false;
	}
	makeCTab();
	makeSymTab();
	return true;
}

Token* Scanner::getToken() {
	Token* tok(NULL);
	std::string v = "";
	//token_type t;
	nextChar();
	while ((curr_ct == C_WHITE) || (isComment())) {
		if (curr_ct == C_WHITE) eatWhiteSpace();
		if (isLineComment()) eatLineComment();
		if (isBlockComment()) eatBlockComment();
	}
	switch (curr_ct) {
		// Alphanumerics (symbols: IDs and RWs)
		case C_UPPER:
		case C_LOWER:
			while (true) {
				// IDs are case insensitive as per language spec
				if (curr_ct == C_UPPER) {
					curr_c += 'a' - 'A';
					curr_ct = C_LOWER;
				}
				v += static_cast<char>(curr_c);
				if (next_ct == C_UPPER || next_ct == C_LOWER
						|| next_ct == C_DIGIT) {
					nextChar();
				} else {
					break;
				}
			}
			if (sym_tab.find(v) == sym_tab.end()) {
				tok = new StrToken(TOK_IDENT, v);
			} else {
				tok = new Token(sym_tab[v]);
			}
			break;
		// Operators (Assignment handles colon)
		case C_EXPR:
			v += static_cast<char>(curr_c);
			tok = new StrToken(TOK_OP_EXPR, v);
			break;
		case C_ARITH:
			v += static_cast<char>(curr_c);
			tok = new StrToken(TOK_OP_ARITH, v);
			break;
		case C_RELAT:
			v += static_cast<char>(curr_c);
			if (next_c == '=') {
				nextChar();
				v += static_cast<char>(curr_c);
			}
			tok = new StrToken(TOK_OP_RELAT, v);
			break;
		case C_COLON:
			v += static_cast<char>(curr_c);
			if (next_c == '=') {
				nextChar();
				v += static_cast<char>(curr_c);
				tok = new StrToken(TOK_OP_ASS, v);
			} else {
				tok = new Token(TOK_COLON);
			}
			break;
		case C_TERM:
			v += static_cast<char>(curr_c);
			tok = new StrToken(TOK_OP_TERM, v);
			break;
		// Numerical constant
		case C_DIGIT:
			v += curr_c;
			while (next_ct == C_DIGIT || next_ct == C_UNDER
					|| next_ct == C_PERIOD){
				nextChar();
				v += curr_c;
			}
			tok = new FltToken(TOK_NUM, v);
			break;
		// String literal
		case C_QUOTE:
			do {
				v += static_cast<char>(curr_c);
				nextChar();
			} while ((curr_ct != C_QUOTE) && (curr_ct != C_EOF));
			if (curr_ct == C_EOF) {
				v += '"';
				reportWarn("EOF before string termination.");
			}
			tok = new StrToken(TOK_STR, v);
			break;
		// Punctuation
		case C_PERIOD:
			tok = new Token(TOK_PERIOD);
			break;
		case C_COMMA:
			tok = new Token(TOK_COMMA);
			break;
		case C_SEMICOL:
			tok = new Token(TOK_SEMICOL);
			break;
		// TOK_COLON is handled by TOK_OP_ASS since it starts with C_COLON
		case C_LPAREN:
			tok = new Token(TOK_LPAREN);
			break;
		case C_RPAREN:
			tok = new Token(TOK_RPAREN);
			break;
		case C_LBRACK:
			tok = new Token(TOK_LBRACK);
			break;
		case C_RBRACK:
			tok = new Token(TOK_RBRACK);
			break;
		case C_LBRACE:
			tok = new Token(TOK_LBRACE);
			break;
		case C_RBRACE:
			tok = new Token(TOK_RBRACE);
			break;
		case C_EOF:
			tok = new Token(TOK_EOF);
			break;
		default:
			reportError("Invalid character/token encountered");
			tok = new Token();
			break;
	}
	return tok;
}

void Scanner::nextChar() {
	if (src_fstream) {
		if (curr_c == '\n') line_number++;
		curr_c = src_fstream.get();
		curr_ct = curr_c < 0 ? C_EOF : c_type_tab[curr_c];
		next_c = src_fstream.peek();
		next_ct = next_c < 0 ? C_EOF : c_type_tab[next_c];
	} else {
		reportError("Could not read character from file.");
	}
}

bool Scanner::isComment() {
	return isLineComment() || isBlockComment();
}

bool Scanner::isLineComment() {
	return (curr_c == '/' && next_c == '/');
}

bool Scanner::isBlockComment() {
	return (curr_c == '/' && next_c == '*');
}

bool Scanner::isBlockEnd() {
	return (curr_c == '*' && next_c == '/');
}

void Scanner::eatWhiteSpace() {
	while ((curr_ct != C_EOF) && (curr_ct == C_WHITE)) {
		nextChar();
	}
}

void Scanner::eatLineComment() {
	if (isLineComment()) {
		do {
			nextChar();
		} while ((curr_c != '\n') && (curr_ct != C_EOF));
	}
}

void Scanner::eatBlockComment() {
	int block_level = 0;
	do {
		if (isBlockComment()) {
			block_level++;
			nextChar();
		} else if (isBlockEnd()) {
			block_level--;
			nextChar();
		}
		nextChar();
	} while ((block_level > 0) && (curr_ct != C_EOF));
	if (curr_ct == C_EOF) {
		reportWarn("EOF before block comment termination.");
	}
}

void Scanner::reportWarn(const std::string &msg) {
	warned = true;
	LOG::ss << "Line " << line_number << ":";
	if (line_number < 10) LOG::ss << ' ';
	LOG::ss << "\t" << msg;
	LOG(LOG_LEVEL::WARN);
}

void Scanner::reportError(const std::string &msg) {
	errored = true;
	LOG::ss << "Line " << line_number << ":";
	if (line_number < 10) LOG::ss << ' ';
	LOG::ss << "\t" << msg;
	LOG(LOG_LEVEL::ERROR);
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

