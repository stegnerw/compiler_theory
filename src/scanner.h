#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <fstream>
#include <unordered_map>
#include "token.h"

class Scanner {
	public:
		Scanner();
		~Scanner();
		bool init(std::string &src_file);
		Token* getToken();
	private:
		bool warned;
		bool errored;
		int line_number;
		int curr_c;
		c_type curr_ct;
		int next_c;
		c_type next_ct;
		c_type c_type_tab[128];
		std::unordered_map<std::string, token_type> sym_tab;
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
		void makeCTab();
		void makeSymTab();
};

#endif // SCANNER_H

