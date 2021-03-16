#ifndef SCANNER_H
#define SCANNER_H

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "char_table.h"
#include "environment.h"
#include "token.h"

class Scanner {
public:
	Scanner(std::shared_ptr<Environment>);
	~Scanner();
	bool init(std::string &src_file);
	std::shared_ptr<Token> getToken();
	bool hasErrored();
private:
	bool errored;
	int line_number;
	CharTable char_table;
	int curr_c;
	CharType curr_ct;
	int next_c;
	CharType next_ct;
	std::shared_ptr<Environment> env;
	std::ifstream src_fstream;
	void nextChar();
	bool isComment();
	bool isLineComment();
	bool isBlockComment();
	bool isBlockEnd();
	void eatWhiteSpace();
	void eatLineComment();
	void eatBlockComment();
};

#endif // SCANNER_H

