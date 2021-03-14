#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <memory>
#include "token.h"

typedef std::unordered_map<std::string, std::shared_ptr<Token>> SymbolMap;

class SymbolTable {
public:
	SymbolTable();
	~SymbolTable();
	std::shared_ptr<Token> lookup(const std::string&);
	bool insert(const std::string&, const TokenType&);

private:
	SymbolMap symbol_map;
};

#endif // SYMBOL_TABLE_H

