#include "code_gen.h"

#include <memory>
#include <stack>
#include <sstream>
#include <string>
#include <unordered_map>

#include "log.h"
#include "token.h"

// TODO: Anything useful to do here?
CodeGen::CodeGen() {}

std::string CodeGen::emitCode() {

  // Collect string fragments
  std::stringstream ss;

  // For the memes
  ss << "; Woah! That's some nice ASSembly there!" << std::endl;
  ss << std::endl;

  // Emit globals
  ss << "; Global definitions" << std::endl;
  ss << globals_code;
  ss << string_literals_code;
  ss << std::endl;

  // Emit declarations
  ss << "; Runtime declarations" << std::endl;
  ss << declarations_code;
  ss << std::endl;

  // Emit body code
  ss << "; Program body" << std::endl;
  ss << body_code;
  ss << std::endl;

  // Return assembly code string
  return ss.str();
}

void
CodeGen::declareVariable(std::shared_ptr<IdToken> id_tok, bool is_global) {

  // Ensure valid id token
  if (id_tok == nullptr || id_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Attempted to declare null variable";
    return;
  }

  // Get type info
  TypeMark tm = id_tok->getTypeMark();
  std::string llvm_type = getLlvmType(tm);
  int num_elements = id_tok->getNumElements();
  std::string llvm_handle;

  // Get array type (including size)
  if (num_elements > 0) {
    llvm_type = getArrayType(llvm_type, num_elements);
  }

  if (is_global) {
    llvm_handle = "@" + id_tok->getVal();
    globals_code += llvm_handle + " = global " + llvm_type
      + " zeroinitializer\n";

  //local variable
  } else {

    // Make sure we have a function on the stack
    if (function_stack.empty()) {
      LOG(ERROR) << "Could not declare local; empty function stack";
      return;
    }

    llvm_handle = "%" + id_tok->getVal();
    function_stack.top().llvm_code += llvm_handle + " = alloca " + llvm_type;
  }

  // Store handle
  id_tok->setLlvmHandle(llvm_handle);
}

void CodeGen::addFunction(std::shared_ptr<IdToken> id_tok) {
  struct Function fun(id_tok);
  function_stack.push(fun);
}

///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////

std::string CodeGen::getLlvmType(const TypeMark& tm) {
  switch (tm) {
    case TYPE_INT:
      return "i32";
    case TYPE_FLT:
      return "float";
    case TYPE_STR:
      return "i8";
    case TYPE_BOOL:
      return "i1";
    default:
      return "BAD_TYPE";
  }
}

std::string CodeGen::getArrayType(const std::string& type, const int& size) {
  return "[" + std::to_string(size) + " x " + type + "]";
}
