#ifndef SCANNER_H
#define SCANNER_H

#include <string>

struct token{
	int type;
	union {
		std::string stringValue;
		int intValue;
		double floatValue;
	} val;
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
		void reportWarn(const std::string &msg);
		void reportError(const std::string &msg);
};

#endif // SCANNER_H

