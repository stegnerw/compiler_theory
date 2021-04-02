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
bool SymbolTable::insert(const std::string& key, std::shared_ptr<Token> t) {
	bool success = true;
	if (lookup(key)) {
		success = false;
	} else {
		symbol_map[key] = t;
	}
	return success;
}
