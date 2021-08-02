#include "environment.h"

#include "log.h"
#include "symbol_table.h"
#include "token.h"

Environment::Environment() {

  // Populate reserved words in global symbol table
  global_symbol_table.insert("program",
    std::shared_ptr<Token>(new Token(TOK_RW_PROG, "program")));
  global_symbol_table.insert("is",
    std::shared_ptr<Token>(new Token(TOK_RW_IS, "is")));
  global_symbol_table.insert("begin",
    std::shared_ptr<Token>(new Token(TOK_RW_BEG, "begin")));
  global_symbol_table.insert("end",
    std::shared_ptr<Token>(new Token(TOK_RW_END, "end")));
  global_symbol_table.insert("global",
    std::shared_ptr<Token>(new Token(TOK_RW_GLOB, "global")));
  global_symbol_table.insert("procedure",
    std::shared_ptr<Token>(new Token(TOK_RW_PROC, "procedure")));
  global_symbol_table.insert("variable",
    std::shared_ptr<Token>(new Token(TOK_RW_VAR, "variable")));
  global_symbol_table.insert("integer",
    std::shared_ptr<Token>(new Token(TOK_RW_INT, "integer")));
  global_symbol_table.insert("float",
    std::shared_ptr<Token>(new Token(TOK_RW_FLT, "float")));
  global_symbol_table.insert("string",
    std::shared_ptr<Token>(new Token(TOK_RW_STR, "string")));
  global_symbol_table.insert("bool",
    std::shared_ptr<Token>(new Token(TOK_RW_BOOL, "bool")));
  global_symbol_table.insert("if",
    std::shared_ptr<Token>(new Token(TOK_RW_IF, "if")));
  global_symbol_table.insert("then",
    std::shared_ptr<Token>(new Token(TOK_RW_THEN, "then")));
  global_symbol_table.insert("else",
    std::shared_ptr<Token>(new Token(TOK_RW_ELSE, "else")));
  global_symbol_table.insert("for",
    std::shared_ptr<Token>(new Token(TOK_RW_FOR, "for")));
  global_symbol_table.insert("return",
    std::shared_ptr<Token>(new Token(TOK_RW_RET, "return")));
  global_symbol_table.insert("not",
    std::shared_ptr<Token>(new Token(TOK_RW_NOT, "not")));
  global_symbol_table.insert("true",
    std::shared_ptr<Token>(new Token(TOK_RW_TRUE, "true")));
  global_symbol_table.insert("false",
    std::shared_ptr<Token>(new Token(TOK_RW_FALSE, "false")));

  // Add builtin functions to the global scope
  std::shared_ptr<IdToken> builtin_tok;
  std::shared_ptr<IdToken> param_tok;

  // getBool() : bool value
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "getbool"));
  builtin_tok->setTypeMark(TYPE_BOOL);
  builtin_tok->setProcedure(true);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // getInteger() : integer value
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT,
      "getinteger"));
  builtin_tok->setTypeMark(TYPE_INT);
  builtin_tok->setProcedure(true);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // getFloat() : float value
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "getfloat"));
  builtin_tok->setTypeMark(TYPE_FLT);
  builtin_tok->setProcedure(true);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // getString() : string value
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "getstring"));
  builtin_tok->setTypeMark(TYPE_STR);
  builtin_tok->setProcedure(true);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // putBool(bool value) : bool
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "putbool"));
  builtin_tok->setTypeMark(TYPE_BOOL);
  builtin_tok->setProcedure(true);
  param_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "param"));
  param_tok->setTypeMark(TYPE_BOOL);
  builtin_tok->addParam(param_tok);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // putInteger(integer value) : bool
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT,
      "putinteger"));
  builtin_tok->setTypeMark(TYPE_BOOL);
  builtin_tok->setProcedure(true);
  param_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "param"));
  param_tok->setTypeMark(TYPE_INT);
  builtin_tok->addParam(param_tok);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // putFloat(float value) : bool
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "putfloat"));
  builtin_tok->setTypeMark(TYPE_BOOL);
  builtin_tok->setProcedure(true);
  param_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "param"));
  param_tok->setTypeMark(TYPE_FLT);
  builtin_tok->addParam(param_tok);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // putString(string value) : bool
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "putstring"));
  builtin_tok->setTypeMark(TYPE_BOOL);
  builtin_tok->setProcedure(true);
  param_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "param"));
  param_tok->setTypeMark(TYPE_STR);
  builtin_tok->addParam(param_tok);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);

  // sqrt(integer value) : float
  builtin_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "sqrt"));
  builtin_tok->setTypeMark(TYPE_FLT);
  builtin_tok->setProcedure(true);
  param_tok = std::shared_ptr<IdToken>(new IdToken(TOK_IDENT, "param"));
  param_tok->setTypeMark(TYPE_INT);
  builtin_tok->addParam(param_tok);
  global_symbol_table.insert(builtin_tok->getVal(), builtin_tok);
}

std::shared_ptr<Token> Environment::lookup(const std::string& key,
    const bool& error) {
  std::shared_ptr<Token> ret_val = nullptr;
  if (!local_symbol_table_stack.empty()) {
    ret_val = local_symbol_table_stack.top().lookup(key);
  }
  if (!ret_val) {
    ret_val = global_symbol_table.lookup(key);
  }
  if (error && !ret_val) {
    LOG(ERROR) << "Identifier not in scope: " << key;
  }
  return ret_val;
}

bool Environment::insert(const std::string& key,
    std::shared_ptr<Token> t, const bool& is_global) {
  bool success = false;
  if (!isReserved(key)) {
    if (is_global) {
      LOG(DEBUG) << "Adding global";
      success = global_symbol_table.insert(key, t);
    } else if (!local_symbol_table_stack.empty()) {
      LOG(DEBUG) << "Adding local";
      success = local_symbol_table_stack.top().insert(key, t);
    } else {
      LOG(ERROR) << "Attempt to add local symbol with no local symbol table";
    }
  } else {
    LOG(ERROR) << "Cannot overwrite reserved word: " << key;
  }
  if (success) {
    LOG(DEBUG) << "Added " << t->getStr()
        << " to symbol table with key " << key;
  } else {
    LOG(ERROR) << "Failed to add " << t->getStr()
        << " to symbol table with key " << key;
  }
  return success;
}

bool Environment::isReserved(const std::string& key) {
  bool is_reserved = false;
  std::shared_ptr<Token> temp_tok = global_symbol_table.lookup(key);
  if (temp_tok && (temp_tok->getType() >= TOK_RW_PROG)
      && (temp_tok->getType() <= TOK_RW_FALSE)) {
    is_reserved = true;
  }
  return is_reserved;
}

void Environment::push() {
  LOG(DEBUG) << "Pushing symbol table stack";
  local_symbol_table_stack.push(SymbolTable());
}

void Environment::pop() {
  if (!local_symbol_table_stack.empty()) {
    LOG(DEBUG) << "Popping symbol table stack";
    local_symbol_table_stack.pop();
  } else {
    LOG(ERROR) << "Attempt to pop empty symbol table stack";
  }
}

std::string Environment::getLocalStr() {
  if (!local_symbol_table_stack.empty()) {
    return local_symbol_table_stack.top().getStr();
  } else {
    LOG(ERROR) << "Attempt to get local symbol table string with empty stack";
    return "\n";
  }
}

std::string Environment::getGlobalStr() {
  return global_symbol_table.getStr();
}
