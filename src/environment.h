#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <memory>
#include <stack>
#include <string>

#include "token.h"
#include "symbol_table.h"

class Environment {
public:
  Environment();
  std::shared_ptr<Token> lookup(const std::string&, const bool&);
  bool insert(const std::string&, std::shared_ptr<Token>,
    const bool&);
  bool isReserved(const std::string&);
  void push();
  void pop();
  std::string getLocalStr();
  std::string getGlobalStr();

private:
  SymbolTable global_symbol_table;
  std::stack<SymbolTable> local_symbol_table_stack;
};

#endif // ENVIRONMENT_H
