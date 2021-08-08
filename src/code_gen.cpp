#include "code_gen.h"

#include <memory>
#include <stack>
#include <sstream>
#include <string>
#include <unordered_map>

#include "log.h"
#include "token.h"

// TODO: Anything useful to do here?
CodeGen::CodeGen() {
  header += "; Woah! That's some nice ASSembly there!\n";
  globals_code += "\n; Global definitions\n";
  string_literals_code += "\n; String literal definitions\n";
  declarations_code += "\n; Runtime declarations\n";
  body_code += "\n; Program body\n";
}

std::string CodeGen::emitCode() {
  std::stringstream ss;
  ss << header;
  ss << globals_code;
  ss << string_literals_code;
  ss << declarations_code;
  ss << body_code;
  return ss.str();
}

void
CodeGen::declareVariable(std::shared_ptr<IdToken> id_tok, bool is_global) {

  // Ensure valid id token
  if (id_tok == nullptr || id_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Attempted to declare invalid variable";
    return;
  }

  // Get type info
  TypeMark tm = id_tok->getTypeMark();
  int num_elements = id_tok->getNumElements();
  std::string llvm_type = getArrayType(tm, num_elements);
  std::string llvm_handle;

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
    function_stack.top()->llvm_code += llvm_handle + " = alloca " + llvm_type
      + "\n";
  }

  // Store handle
  id_tok->setLlvmHandle(llvm_handle);
}

// TODO: I think I technically need to allocate memory for the parameters and
// then store the values.
// Shouldn't be too hard but I have more important things to add...
void CodeGen::addFunction(std::shared_ptr<IdToken> fun_tok) {

  // Make sure id token is valid
  if (fun_tok == nullptr || !fun_tok->getProcedure()) {
    LOG(ERROR) << "Attempt to add invalid function";
    return;
  }
  auto fun = std::make_shared<struct Function>(fun_tok);

  // Produce function type and name
  std::string llvm_handle = "@" + fun_tok->getVal();
  int fun_count = function_counter[fun_tok->getVal()]++;
  if (fun_count > 0) {
    llvm_handle += std::to_string(fun_count);
  }
  fun_tok->setLlvmHandle(llvm_handle);
  fun->llvm_code += "define " + getLlvmType(fun_tok->getTypeMark()) + " "
    + llvm_handle;

  // Add parameters
  fun->llvm_code += "(";
  int num_args = fun_tok->getNumElements();
  for (int idx = 0; idx < num_args; idx++) {
    std::shared_ptr<IdToken> param_tok = fun_tok->getParam(idx);
    if (param_tok == nullptr || param_tok->getType() == TOK_INVALID) {
      LOG(ERROR) << "Invalid parameter when generating function "
        << fun_tok->getVal();
      return;
    }

    // Get type and handle (naming them by register)
    //std::string param_llvm_handle = "%" + std::to_string(fun->reg_count++);
    std::string param_llvm_type = getArrayType(param_tok->getTypeMark(),
        param_tok->getNumElements());
    // My llvm-as doesn't like me putting the handle
    fun->llvm_code += param_llvm_type;// + " " + param_llvm_handle;
    fun->reg_count++;

    // Add comma if needed
    if (idx < num_args-1) {
      fun->llvm_code += ", ";
    }
  }
  fun->llvm_code += "){\n";
  fun->reg_count++;
  function_stack.push(fun);

  // Allocate memory for new parameters because they are mutable
  fun->llvm_code += "; Allocate parameters\n";
  for (int idx = 0; idx < num_args; idx++) {
    declareVariable(fun_tok->getParam(idx), false);
  }

  // Store parameters for future access
  // TODO: This
  fun->llvm_code += "; Store parameters (coming soon!)\n";
}

void CodeGen::closeFunction() {

  // Make sure stack isn't empty
  if (function_stack.empty()) {
    LOG(ERROR) << "Attempt to pop empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  std::shared_ptr<IdToken> fun_tok = fun->id_tok;

  // Generate a return statement just in case
  fun->llvm_code += getBlankReturn() + "\n}\n";
  body_code += fun->llvm_code;
  function_stack.pop();
}

///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////

std::string CodeGen::getLlvmType(const TypeMark& tm) {
  switch (tm) {
    case TYPE_INT:
      return "i32";
      break;
    case TYPE_FLT:
      return "float";
      break;
    case TYPE_STR:
      return "i8";
      break;
    case TYPE_BOOL:
      return "i1";
      break;
    default:
      return "BAD_TYPE";
      break;
  }
}

std::string CodeGen::getArrayType(const TypeMark& tm, const int& size) {
  if (size == 0) return getLlvmType(tm);
  return "[" + std::to_string(size) + " x " + getLlvmType(tm) + "]";
}

std::string CodeGen::getBlankReturn() {

  // Make sure we have a function to return from
  if (function_stack.empty()) {
    LOG(ERROR) << "No function to return from";
    return "BAD RETURN";
  }
  TypeMark tm = function_stack.top()->id_tok->getTypeMark();

  // Generate return
  std::string ret = "ret " + getLlvmType(tm);
  switch (tm) {
    case TYPE_INT:
      ret += " 0";
      break;
    case TYPE_FLT:
      ret += " 0.0";
      break;
    case TYPE_BOOL:
      ret += " false";
      break;
    case TYPE_STR:
      ret += "* ";
      break;
      // TODO: Do the stuff and things
    default:
      ret += " 0";
      break;
  }

  return ret;
}
