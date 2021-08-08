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
  std::stringstream llvm_code;
  std::stack<int> if_stack, loop_stack;  // For label indexing
};

class CodeGen {
public:
  CodeGen();
  std::string emitCode();
  void declareVariable(std::shared_ptr<IdToken>, bool);
  void addFunction(std::shared_ptr<IdToken>);

private:

  // Code compartmentalization string streams for proper emission order
  // Globals and string literals could be combined, but my brain likes them
  // separate
  std::stringstream globals_code;  // global declarations
  std::stringstream string_literals_code;  // String literal declarations
  std::stringstream declarations_code;  // runtime declarations for print, etc.
  std::stringstream body_code;  // function definitions and their bodies

  // Stack and map to control emission and naming of functions
  // Stack avoids nesting definitions
  // Map avoids naming conflicts because LLVM procedures are global but the
  // language spec has scoping rules for function names
  std::stack<Function> function_stack;
  std::unordered_map<std::string, int> function_counter;

  // Private helper functions
  std::string getLlvmType(const TypeMark&);
  std::string getArrayType(const std::string&, const int&);

};

#endif // CODE_GEN_H
