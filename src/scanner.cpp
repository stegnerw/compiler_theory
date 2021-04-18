#include "scanner.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "char_table.h"
#include "environment.h"
#include "log.h"
#include "token.h"

////////////////////////////////////////////////////////////////////////////////
// Public
////////////////////////////////////////////////////////////////////////////////

Scanner::Scanner(std::shared_ptr<Environment> e) :
		line_number(1),
		env(e){}

Scanner::~Scanner() {
	if (src_fstream) {
		src_fstream.close();
	}
}

bool Scanner::init(const std::string& src_file) {
	LOG(INFO) << "Initializing scanner for the file " << src_file;
	line_number = 1;
	LOG::line_number = line_number;
	src_fstream.open(src_file, std::ios::in);
	if (!src_fstream) {
		LOG(ERROR) << "Failed to initialize scanner";
		LOG(ERROR) << "Invalid file: " << src_file;
		LOG(ERROR) << "Make sure it exists and you have read permissions";
		return false;
	}
	LOG(INFO) << "Scanner initialized successfully";
	return true;
}

std::shared_ptr<Token> Scanner::getToken() {
	std::shared_ptr<Token> tok(nullptr);
	std::string v = "";
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
						|| next_ct == C_DIGIT || next_ct == C_UNDER) {
					nextChar();
				} else {
					break;
				}
			}
			if (!env->isReserved(v)) {
				tok = std::shared_ptr<Token>(new IdToken(TOK_IDENT, v));
			} else {
				tok = env->lookup(v);
			}
			break;
		// Operators (Assignment handles colon)
		case C_EXPR:
			v += static_cast<char>(curr_c);
			tok = std::shared_ptr<Token>(new Token(TOK_OP_EXPR, v));
			break;
		case C_ARITH:
			v += static_cast<char>(curr_c);
			tok = std::shared_ptr<Token>(new Token(TOK_OP_ARITH, v));
			break;
		case C_RELAT:
			v += static_cast<char>(curr_c);
			if (next_c == '=') {
				nextChar();
				v += static_cast<char>(curr_c);
			}
			tok = std::shared_ptr<Token>(new Token(TOK_OP_RELAT, v));
			break;
		case C_COLON:
			v += static_cast<char>(curr_c);
			if (next_c == '=') {
				nextChar();
				v += static_cast<char>(curr_c);
				tok = std::shared_ptr<Token>(new Token(TOK_OP_ASS, v));
			} else {
				tok = std::shared_ptr<Token>(new Token(TOK_COLON, ":"));
			}
			break;
		case C_TERM:
			v += static_cast<char>(curr_c);
			tok = std::shared_ptr<Token>(new Token(TOK_OP_TERM, v));
			break;
		// Numerical constant
		case C_DIGIT:
			v += curr_c;
			while (next_ct == C_DIGIT || next_ct == C_UNDER || next_ct == C_PERIOD){
				nextChar();
				// Skip underscores
				if (curr_ct != C_UNDER) {
					v += curr_c;
				}
			}
			// If there is a decimal make it a float, else int
			if (v.find('.') == std::string::npos) {
				tok = std::shared_ptr<Token>(new LiteralToken<int>(TOK_NUM,
						std::stoi(v), TYPE_INT));
			} else {
				tok = std::shared_ptr<Token>(new LiteralToken<float>(TOK_NUM,
						std::stof(v), TYPE_FLT));
			}
			break;
		// String literal
		case C_QUOTE:
			do {
				v += static_cast<char>(curr_c);
				nextChar();
			} while ((curr_ct != C_QUOTE) && (curr_ct != C_EOF));
			if (curr_ct == C_EOF) {
				v += '"';
				LOG(ERROR) << "EOF before string termination; assuming closed";
			}
			tok = std::shared_ptr<Token>(new LiteralToken<std::string>(TOK_STR, v,
					TYPE_STR));
			break;
		// Punctuation
		case C_PERIOD:
			tok = std::shared_ptr<Token>(new Token(TOK_PERIOD,
					std::string(1, static_cast<char>(curr_c))));
			break;
		case C_COMMA:
			tok = std::shared_ptr<Token>(new Token(TOK_COMMA,
					std::string(1, static_cast<char>(curr_c))));
			break;
		case C_SEMICOL:
			tok = std::shared_ptr<Token>(new Token(TOK_SEMICOL,
					std::string(1, static_cast<char>(curr_c))));
			break;
		// TOK_COLON is handled by TOK_OP_ASS since it starts with C_COLON
		case C_LPAREN:
			tok = std::shared_ptr<Token>(new Token(TOK_LPAREN,
					std::string(1, static_cast<char>(curr_c))));
			break;
		case C_RPAREN:
			tok = std::shared_ptr<Token>(new Token(TOK_RPAREN,
					std::string(1, static_cast<char>(curr_c))));
			break;
		case C_LBRACK:
			tok = std::shared_ptr<Token>(new Token(TOK_LBRACK,
					std::string(1, static_cast<char>(curr_c))));
			break;
		case C_RBRACK:
			tok = std::shared_ptr<Token>(new Token(TOK_RBRACK,
					std::string(1, static_cast<char>(curr_c))));
			break;
		case C_EOF:
			tok = std::shared_ptr<Token>(new Token(TOK_EOF, "<EOF>"));
			break;
		default:
			std::stringstream ss;
			LOG(ERROR) << "Invalid character/token encountered: "
					<< static_cast<char>(curr_c)
					<< "; treating as whitespace";
			tok = std::shared_ptr<Token>(new Token());
			break;
	}
	LOG(DEBUG) << tok->getStr();
	return tok;
}

////////////////////////////////////////////////////////////////////////////////
// Private
////////////////////////////////////////////////////////////////////////////////

void Scanner::nextChar() {
	if (src_fstream) {
		if (curr_c == '\n') {
			line_number++;
			LOG::line_number = line_number;
		}
		curr_c = src_fstream.get();
		curr_ct = curr_c < 0 ? C_EOF : char_table.getCharType(curr_c);
		next_c = src_fstream.peek();
		next_ct = next_c < 0 ? C_EOF : char_table.getCharType(next_c);
	} else {
		LOG(ERROR) << "Failed to read source file";
		curr_c = -1;
		curr_ct = C_EOF;
		next_c = -1;
		next_ct = C_EOF;
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
		LOG(WARN) << "EOF before block comment termination; assuming closed";
	}
}
