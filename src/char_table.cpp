#include "char_table.h"

CharTable::CharTable() {
	for (int i = 0; i < 128; i++) {
		switch (i) {
			case '.':
				char_type_table[i] = C_PERIOD; break;
			case '_':
				char_type_table[i] = C_UNDER; break;
			case ';':
				char_type_table[i] = C_SEMICOL; break;
			case ':':
				char_type_table[i] = C_COLON; break;
			case ',':
				char_type_table[i] = C_COMMA; break;
			case '(':
				char_type_table[i] = C_LPAREN; break;
			case ')':
				char_type_table[i] = C_RPAREN; break;
			case '[':
				char_type_table[i] = C_LBRACK; break;
			case ']':
				char_type_table[i] = C_RBRACK; break;
			case '{':
				char_type_table[i] = C_LBRACE; break;
			case '}':
				char_type_table[i] = C_RBRACE; break;
			case '&':
			case '|':
				char_type_table[i] = C_EXPR; break;
			case '<':
			case '>':
			case '=':
			case '!':
				char_type_table[i] = C_RELAT; break;
			case '+':
			case '-':
				char_type_table[i] = C_ARITH; break;
			case '/':
			case '*':
				char_type_table[i] = C_TERM; break;
			case '"':
				char_type_table[i] = C_QUOTE; break;
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				char_type_table[i] = C_WHITE; break;
			default:
				if (i >= 'a' && i <= 'z') {
					char_type_table[i] = C_LOWER;
				} else if (i >= 'A' && i <= 'Z') {
					char_type_table[i] = C_UPPER;
				}else if (i >= '0' && i <= '9') {
					char_type_table[i] = C_DIGIT;
				} else {
					char_type_table[i] = C_INVALID;
				}
		}
	}
}

CharType CharTable::getCharType(int c) {
	if (c < 0) {
		return C_EOF;
	} else if (c < 128) {
		return char_type_table[c];
	}
	return C_INVALID;
}
