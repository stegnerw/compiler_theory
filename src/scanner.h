#ifndef SCANNER_H
#define SCANNER_H

#include <fstream>
#include <string>
#include <unordered_map>

#include "char_table.h"
#include "symbol_table.h"
#include "token.h"

class Scanner {
public:
	Scanner();
	~Scanner();
	bool init(std::string &src_file);
	Token* getToken();
	bool hasErrored();
private:
	bool errored;
	int line_number;
	CharTable char_table;
	int curr_c;
	CharType curr_ct;
	int next_c;
	CharType next_ct;
	std::unordered_map<std::string, TokenType> sym_tab;
	std::ifstream src_fstream;
	void nextChar();
	bool isComment();
	bool isLineComment();
	bool isBlockComment();
	bool isBlockEnd();
	void eatWhiteSpace();
	void eatLineComment();
	void eatBlockComment();
	void reportWarn(const std::string &msg);
	void reportError(const std::string &msg);
	void makeSymTab();
};

#endif // SCANNER_H

