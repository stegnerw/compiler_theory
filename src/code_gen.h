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
  bool in_basic_block;
  Function(const std::shared_ptr<IdToken>& id_tok) :
    id_tok(id_tok), in_basic_block(false) {}
};

class CodeGen {
public:
  CodeGen();
  std::string emitCode();
  void declareVariable(std::shared_ptr<IdToken>, bool);
  void startBasicBlock();
  void startBasicBlock(const std::string&);
  void addFunction(std::shared_ptr<IdToken>);
  void closeFunction();
  void store(std::shared_ptr<IdToken>, std::string, const TypeMark&);
  std::string loadVar(std::shared_ptr<IdToken>, std::string);

  // Comment functions
  void commentDecl();

private:

  // Code compartmentalization strings for proper emission order
  // Globals and string literals could be combined, but my brain likes them
  // separate
  std::stringstream header;
  std::stringstream globals_code;  // global declarations
  std::stringstream string_literals_code;  // String literal declarations
  std::stringstream declarations_code;  // runtime declarations for print, etc.
  std::stringstream body_code;  // function definitions and their bodies

  // Stack and map to control emission and naming of functions
  // Stack avoids nesting definitions
  // Map avoids naming conflicts because LLVM procedures are global but the
  // language spec has scoping rules for function names
  std::stack<std::shared_ptr<Function>> function_stack;
  std::unordered_map<std::string, int> function_counter;

  // String tracking
  //std::unordered_map<std::string, std::string> string_map;
  //int string_counter;

  // Private helper functions
  std::string getLlvmType(const TypeMark&);
  std::string getArrayType(const TypeMark&, const int&);
  std::string getBlankReturn();
  std::string convert(const TypeMark&, const TypeMark&, std::string);
  void endBasicBlock(const std::string&);

};

#endif // CODE_GEN_H
