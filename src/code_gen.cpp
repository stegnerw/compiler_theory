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
  header << "; Woah! That's some nice ASSembly there!\n";
  globals_code << "\n; Global definitions\n";
  string_literals_code << "\n; String literal definitions\n";
  declarations_code << "\n; Runtime declarations\n";
  body_code << "\n; Program body\n";
}

std::string CodeGen::emitCode() {
  std::stringstream ss;
  ss << header.str();
  ss << globals_code.str();
  ss << string_literals_code.str();
  ss << declarations_code.str();
  ss << body_code.str();
  return ss.str();
}

void
CodeGen::declareVariable(std::shared_ptr<IdToken> id_tok, bool is_global) {

  // Ensure valid id token
  if (id_tok == nullptr || id_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Attempted to declare invalid variable";
    return;
  }
  LOG(DEBUG) << "Code generating " << id_tok->getStr();

  // Get type info
  TypeMark tm = id_tok->getTypeMark();
  int num_elements = id_tok->getNumElements();
  std::string llvm_type = getArrayType(tm, num_elements);
  std::string llvm_handle;

  if (is_global) {
    llvm_handle = "@" + id_tok->getVal();
    globals_code << llvm_handle << " = global " << llvm_type
      << " zeroinitializer\n";

  //local variable
  } else {

    // Make sure we have a function on the stack
    if (function_stack.empty()) {
      LOG(ERROR) << "Could not declare local; empty function stack";
      return;
    }

    llvm_handle = "%" + id_tok->getVal();
    function_stack.top()->llvm_code << llvm_handle << " = alloca " << llvm_type
      << "\n";
  }

  // Store handle
  id_tok->setLlvmHandle(llvm_handle);
}

void CodeGen::startBasicBlock() {

  // Make sure function stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot start basic block with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();

  // Generate "implied" label explicitly
  std::string label = std::to_string(fun->reg_count++);
  startBasicBlock(label);
}

void CodeGen:: startBasicBlock(const std::string& label) {
  // Make sure function stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot start basic block with empty function stack";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();

  // Close basic block if needed
  if (fun->in_basic_block) {
    endBasicBlock(label);
  }

  // Emit label
  fun->llvm_code << "\n" << label << ":\n";
  fun->in_basic_block = true;
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
  fun->llvm_code << "define " << getLlvmType(fun_tok->getTypeMark()) << " "
    << llvm_handle;

  // Add parameters
  fun->llvm_code << "(";
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
    fun->llvm_code << param_llvm_type;// << " " << param_llvm_handle;
    fun->reg_count++;

    // Add comma if needed
    if (idx < num_args-1) {
      fun->llvm_code << ", ";
    }
  }
  fun->llvm_code << "){\n";
  function_stack.push(fun);
  startBasicBlock();

  // Allocate memory for new parameters because they are mutable
  fun->llvm_code << "; Allocate parameters\n";
  for (int idx = 0; idx < num_args; idx++) {
    declareVariable(fun_tok->getParam(idx), false);
  }

  // Store parameters for future access
  // TODO: This
  fun->llvm_code << "; Store parameters\n";
  for (int idx = 0; idx < num_args; idx++) {
    std::shared_ptr<IdToken> param_tok = fun_tok->getParam(idx);
    std::string reg = "%" + std::to_string(idx);
    TypeMark reg_tm = param_tok->getTypeMark();
    store(param_tok, reg, reg_tm);
  }
}

void CodeGen::closeFunction() {

  // Make sure stack isn't empty
  if (function_stack.empty()) {
    LOG(ERROR) << "Attempt to pop empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  std::shared_ptr<IdToken> fun_tok = fun->id_tok;

  // Generate a return statement if still in a basic block
  if (fun->in_basic_block) {
    fun->llvm_code << getBlankReturn();
  }
  fun->llvm_code << "\n}\n\n";
  body_code << fun->llvm_code.str();
  function_stack.pop();
}

void CodeGen::store(std::shared_ptr<IdToken> id_tok, std::string reg,
    const TypeMark& reg_tm) {

  // Validate id token
  if (id_tok == nullptr || id_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Invalid store generation";
    return;
  }
  LOG(DEBUG) << "Generating store " << id_tok->getStr();
  LOG(DEBUG) << "Storing to " << reg;

  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot store with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();

  // Check if we have to typecast
  TypeMark var_tm = id_tok->getTypeMark();
  if (var_tm != reg_tm) {
    reg = convert(reg_tm, var_tm, reg);
  }

  // Emit the store command
  std::string llvm_var_tm = getLlvmType(var_tm);
  std::string var_reg = id_tok->getLlvmHandle();
  fun->llvm_code << "store " << llvm_var_tm << " " << reg << ", "
    << llvm_var_tm << "* " << var_reg << "\n";
}

std::string
CodeGen::loadVar(std::shared_ptr<IdToken> id_tok, std::string reg) {
  // Validate id token
  if (id_tok == nullptr || id_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Invalid store generation";
    return "BAD_REG";
  }
  LOG(DEBUG) << "Generating store " << id_tok->getStr();
  LOG(DEBUG) << "Storing to " << reg;

  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot store with empty function stack";
    return "BAD_REG";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();

  // Emit the load command
  std::string new_reg = "%" + std::to_string(
      function_stack.top()->reg_count++);
  std::string llvm_var_tm = getLlvmType(id_tok->getTypeMark());
  fun->llvm_code << new_reg << " = load " << llvm_var_tm << ", " << llvm_var_tm
    << "* " << reg;

  return new_reg;
}

std::string CodeGen::getLitNum(std::shared_ptr<Token> tok) {
  std::string ret_val;

  // Parse int
  auto int_tok = std::dynamic_pointer_cast<LiteralToken<int>>(tok);
  if (int_tok != nullptr) {
    ret_val = std::to_string(int_tok->getVal());
  }

  // Parse float to byte string (holy moly this took me a while)
  auto flt_tok = std::dynamic_pointer_cast<LiteralToken<float>>(tok);
  if (flt_tok != nullptr) {
    //ret_val = "0x00000000";
    std::stringstream ss;
    ss << "0x";
    float f = flt_tok->getVal();
    auto c_ptr = reinterpret_cast<unsigned char*>(&f);
    for (size_t i = 0; i < sizeof(float); i++) {
      ss << std::setw(2) << std::setfill('0') << std::hex
        << static_cast<int>(c_ptr[sizeof(float) - 1 - i]);
    }
    ret_val = ss.str();
  }

  // Conversion failed
  if (int_tok == nullptr && flt_tok == nullptr) {
    LOG(ERROR) << "Attempt to generate invalid number";
    ret_val = "NUM_INVALID";
  }

  return ret_val;
}

// This actually has to load and potentially allocate globals
std::string CodeGen::getLitStr(std::shared_ptr<Token> tok) {
  std::string ret_val;

  auto str_tok = std::dynamic_pointer_cast<LiteralToken<std::string>>(tok);
  if (str_tok != nullptr) {
    std::string str = str_tok->getVal();
    std::string str_handle = getStringHandle(str);
  } else {
    LOG(ERROR) << "Attempt to generate invalid string";
    ret_val = "STR_INVALID";
  }

  return ret_val;
}

///////////////////////////////////////////////////////////////////////////////
// Comment functions
///////////////////////////////////////////////////////////////////////////////

void CodeGen::commentDecl() {
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot comment declarations; function stack empty";
    return;
  }
  function_stack.top()->llvm_code << "\n; Local variable declarations\n";
}

void CodeGen::commentStmt() {
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot comment statements; function stack empty";
    return;
  }
  function_stack.top()->llvm_code << "\n; Statements\n";
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
    return "; BAD RETURN";
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

std::string CodeGen::convert(const TypeMark& start_tm, const TypeMark& end_tm,
    std::string reg) {

  // Make sure function stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot convert; empty function stack";
    return "; BAD CONVERSION";
  }
  std::string new_reg = "%" + std::to_string(
      function_stack.top()->reg_count++);
  std::stringstream ss;
  ss << new_reg << " = ";

  switch (start_tm) {
    case TYPE_INT:
      switch (end_tm) {
        case TYPE_FLT:  // signed int to floating point
          ss << "sitofp " << getLlvmType(start_tm) << " " << reg << " to "
            << getLlvmType(end_tm);
          break;
        case TYPE_BOOL:  // integer compare not equal
          ss << "icmp ne " << getLlvmType(start_tm) << " " << reg << ", 0";
          break;
        default:
          LOG(ERROR) << "Bad type conversion";
          LOG(ERROR) << Token::getTypeMarkName(start_tm) << " to "
           << Token::getTypeMarkName(end_tm);
          return "; BAD CONVERSION";
      }
      break;

    case TYPE_FLT:
      switch (end_tm) {
        case TYPE_INT:  // floating point to signed int
          ss << "fptosi " << getLlvmType(start_tm) << " " << reg << " to "
            << getLlvmType(end_tm);
          break;
        default:
          LOG(ERROR) << "Bad type conversion";
          LOG(ERROR) << Token::getTypeMarkName(start_tm) << " to "
           << Token::getTypeMarkName(end_tm);
          return "; BAD CONVERSION";
      }
      break;

    case TYPE_BOOL:
      switch (end_tm) {
        case TYPE_INT:  // zero sign extend
          ss << "zext " << getLlvmType(start_tm) << " " << reg << " to "
            << getLlvmType(end_tm);
          break;
        default:
          LOG(ERROR) << "Bad type conversion";
          LOG(ERROR) << Token::getTypeMarkName(start_tm) << " to "
           << Token::getTypeMarkName(end_tm);
          return "; BAD CONVERSION";
      }
      break;

    default:
      LOG(ERROR) << "Bad type conversion";
      LOG(ERROR) << Token::getTypeMarkName(start_tm) << " to "
       << Token::getTypeMarkName(end_tm);
      return "; BAD CONVERSION";
  }

  // Emit conversion and return new register reference
  function_stack.top()->llvm_code << ss.str();
  return new_reg;
}

// End the current basic block and jump to the next one
void CodeGen::endBasicBlock(const std::string& next_label) {

  // Make sure function stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot end basic block with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();

  // Emit unconditional branch to next block
  fun->llvm_code << "br label %" << next_label << "\n";
  fun->in_basic_block = false;
}

std::string CodeGen::getStringHandle(const std::string& str) {
  std::cout << "Getting handle for " << str << std::endl;
  if (string_map.find(str) != string_map.end()) {
    return string_map[str];
  }

  // Add new handle to map
  std::string handle = "@.str." + std::to_string(string_counter++);
  string_map[str] = handle;

  // Just make it a hex string to avoid problems
  std::stringstream ss;
  for (unsigned int i = 0; i < str.length(); i++) {
    int c = str[i];
    ss << "\\" << std::setw(2) << std::setfill('0') << std::hex << c;
  }
  ss << "\\00";

  // Emit the global declaration and return handle
  string_literals_code << handle << " = constant "
    << getArrayType(TYPE_STR, str.length() + 1) << " c\"" << ss.str() << "\"";
  return handle;
}
