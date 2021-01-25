#include "scanner.h"
#include <string>
#include <iostream>
#include "log.h"

Scanner::Scanner() : warned(false), errored(false), line_number(0){}

bool Scanner::init(std::string &src_file) {
	LOG(ERROR) << "Not implemented yet";
	return false;
}

void Scanner::reportWarn(const std::string &msg) {
	warned = true;
	LOG(WARN) << "Line " << line_number << ":\t" << msg;
}

void Scanner::reportError(const std::string &msg) {
	errored = true;
	LOG(ERROR) << "Line " << line_number << ":\t" << msg;
}

