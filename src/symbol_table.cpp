#include "symbol_table.h"
#include <string>
#include <unordered_map>
#include <memory>
#include "token.h"
#include "log.h"

SymbolTable::SymbolTable() {}

SymbolTable::~SymbolTable() {
	symbol_map.clear();
}

std::shared_ptr<Token> SymbolTable::lookup(const std::string& key) {
	std::shared_ptr<Token> ret_val = nullptr;
	if (symbol_map.find(key) != symbol_map.end()) {
		ret_val = symbol_map[key];
	}
	return ret_val;
}

// Environment checks for reserved words
bool SymbolTable::insert(const std::string& key,
		const TokenType& tt) {
	bool exists = false;
	std::shared_ptr<Token> new_token = lookup(key);
	if (new_token) {
		exists = true;
	} else {
		// TODO: Figure out which token type and initialize
		new_token = std::shared_ptr<Token>(new StrToken(tt, key));
		symbol_map[key] = new_token;
	}
	return exists;
}

