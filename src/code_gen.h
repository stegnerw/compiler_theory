#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <sstream>
#include <unordered_map>

#include "token.h"

struct Function {
  int if_count, loop_count, reg_count;
  std::shared_ptr<IdToken> id_tok;
  std::string llvm_code;
  std::stack<int> if_stack, loop_stack;  // For label indexing
  Function(const std::shared_ptr<IdToken>& id_tok) : id_tok(id_tok) {}
};

class CodeGen {
public:
  CodeGen();
  std::string emitCode();
  void declareVariable(std::shared_ptr<IdToken>, bool);
  void addFunction(std::shared_ptr<IdToken>);
  void closeFunction();
  void storeVariable(std::shared_ptr<IdToken>, std::string);

private:

  // Code compartmentalization strings for proper emission order
  // Globals and string literals could be combined, but my brain likes them
  // separate
  std::string header;
  std::string globals_code;  // global declarations
  std::string string_literals_code;  // String literal declarations
  std::string declarations_code;  // runtime declarations for print, etc.
  std::string body_code;  // function definitions and their bodies

  // Stack and map to control emission and naming of functions
  // Stack avoids nesting definitions
  // Map avoids naming conflicts because LLVM procedures are global but the
  // language spec has scoping rules for function names
  std::stack<std::shared_ptr<Function>> function_stack;
  std::unordered_map<std::string, int> function_counter;

  // Private helper functions
  std::string getLlvmType(const TypeMark&);
  std::string getArrayType(const TypeMark&, const int&);
  std::string getBlankReturn();

};

#endif // CODE_GEN_H
