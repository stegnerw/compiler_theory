#include "parser.h"

#include <fstream>
#include <string>
#include <unordered_map>

#include "environment.h"
#include "log.h"
#include "scanner.h"
#include "token.h"

Parser::Parser() : env(new Environment()), scanner(env), errored(false) {}

bool Parser::init(const std::string& src_file) {
	bool init_success = true;
	errored = false;
	if (!scanner.init(src_file)) {
		init_success = false;
		LOG(ERROR) << "Failed to initialize parser";
		LOG(ERROR) << "See logs";
		errored = true;
	} else {
		scan();
	}
	return init_success;
}

bool Parser::parse() {
	while (token->getType() != TOK_EOF) {
		scan();
	}
	return !errored;
}

void Parser::scan() {
	token = scanner.getToken();
	if (scanner.hasErrored()) {
		errored = true;
	}
}
