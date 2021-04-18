#include "symbol_table.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
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
		std::shared_ptr<Token> t) {
	bool success = false;
	std::shared_ptr<Token> tok = lookup(key);
	if (!tok) {
		symbol_map[key] = t;
		success = true;
	} else {
		LOG(ERROR) << "Symbol already exists with name: " << key;
	}
	return success;
}

std::string SymbolTable::getStr() {
	std::stringstream ss;
	for (auto it : symbol_map) {
		ss << it.first << ": " << it.second->getStr() << "\n";
	}
	return ss.str();
}
