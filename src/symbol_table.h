#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <memory>
#include <string>
#include <unordered_map>

#include "token.h"

typedef std::unordered_map<std::string, std::shared_ptr<Token>> SymbolMap;

class SymbolTable {
public:
	SymbolTable();
	~SymbolTable();
	std::shared_ptr<Token> lookup(const std::string&);
	bool insert(const std::string&, std::shared_ptr<Token>);
	std::string getStr();

private:
	SymbolMap symbol_map;
};

#endif // SYMBOL_TABLE_H
