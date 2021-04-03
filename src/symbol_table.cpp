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
std::shared_ptr<Token> SymbolTable::insert(const std::string& key,
		std::shared_ptr<Token> t) {
	std::shared_ptr<Token> new_token = lookup(key);
	if (!new_token) {
		symbol_map[key] = t;
		new_token = t;
	} else {
		LOG(WARN) << "Attempt to add duplicate symbol: " << key;
		new_token = nullptr;
	}
	return new_token;
}
