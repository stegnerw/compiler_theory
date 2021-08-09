#include "code_gen.h"

#include <memory>
#include <stack>
#include <sstream>
#include <string>
#include <unordered_map>

#include "log.h"
#include "token.h"

CodeGen::CodeGen() : string_counter(0) {
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
    // Get handle and emit instruction
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
  // Mark implied label
  std::string label = std::to_string(fun->reg_count++);
  fun->llvm_code << "; label %" << label << " implied\n";
  fun->in_basic_block = true;
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
  fun->llvm_code << "; Store parameters\n";
  for (int idx = 0; idx < num_args; idx++) {
    std::shared_ptr<IdToken> param_tok = fun_tok->getParam(idx);
    std::string param_handle = param_tok->getLlvmHandle();
    std::string reg_handle = "%" + std::to_string(idx);
    TypeMark reg_tm = param_tok->getTypeMark();
    store(param_handle, reg_tm, reg_handle, reg_tm);
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

void CodeGen::store(std::string mem_handle, const TypeMark& mem_tm,
    std::string reg_handle, const TypeMark& reg_tm) {
  LOG(DEBUG) << "Storing " << reg_handle << " to " << mem_handle;
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot store with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Check if we have to typecast
  if (mem_tm != reg_tm) {
    reg_handle = convert(reg_tm, mem_tm, reg_handle);
  }
  // Emit the store command
  std::string llvm_var_tm = getLlvmType(mem_tm);
  fun->llvm_code << "store " << llvm_var_tm << " " << reg_handle << ", "
    << llvm_var_tm << "* " << mem_handle << "\n";
}

std::string CodeGen::loadVar(std::string mem_handle, const TypeMark& tm)
{
  LOG(DEBUG) << "Loading from " << mem_handle;
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot store with empty function stack";
    return "BAD_REG";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Emit the load command
  std::string new_reg = "%" + std::to_string(
      function_stack.top()->reg_count++);
  std::string llvm_var_tm = getLlvmType(tm);
  fun->llvm_code << new_reg << " = load " << llvm_var_tm << ", " << llvm_var_tm
    << "* " << mem_handle << "\n";
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
    std::stringstream ss;
    ss << "0x";
    float f = flt_tok->getVal();
    auto c_ptr = reinterpret_cast<unsigned char*>(&f);
    for (size_t i = 0; i < sizeof(float); i++) {
      ss << std::setw(2) << std::setfill('0') << std::hex
        << static_cast<int>(c_ptr[sizeof(float) - 1 - i]);
    }
    ss << "00000000";
    ret_val = ss.str();
  }
  // Conversion failed
  if (int_tok == nullptr && flt_tok == nullptr) {
    LOG(ERROR) << "Attempt to generate invalid number";
    ret_val = "NUM_INVALID";
  }
  return ret_val;
}

std::string CodeGen::getLitStr(std::shared_ptr<Token> tok) {
  std::string str;
  std::string handle;
  auto str_tok = std::dynamic_pointer_cast<LiteralToken<std::string>>(tok);
  if (str_tok != nullptr) {
    str = str_tok->getVal();
    handle = getStringHandle(str);
  } else {
    LOG(ERROR) << "Attempt to generate invalid string";
    return "STR_INVALID";
  }
  // Get a pointer to the string
  return getArrayPtr(handle, str.length() + 1, TYPE_STR, "0");
}

std::string CodeGen::getArrayPtr(const std::string& handle, const int& size,
    const TypeMark& tm, std::string idx_handle) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot get pointer with empty function stack";
    return "BAD_REG";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  std::string new_reg = "%" + std::to_string(fun->reg_count++);
  std::stringstream ss;
  std::string arr_type = getArrayType(tm, size);
  ss << new_reg << " = getelementptr " << arr_type << ", " << arr_type << "* "
    << handle << ", i32 0, i32 " << idx_handle << "\n";
  // Emit statement and return reg
  fun->llvm_code << ss.str();
  return new_reg;
}

std::string CodeGen::binaryOp(std::string lhs_handle, const TypeMark& lhs_tm,
    std::string rhs_handle, const TypeMark& rhs_tm, const TypeMark& cast_tm,
    std::shared_ptr<Token> op_tok) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot op with empty function stack";
    return "BAD_REG";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Make sure op token is okay
  if (op_tok == nullptr || op_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Cannot generate binary op; invalid token";
    return "BAD_REG";
  }
  TokenType tt = op_tok->getType();
  std::string op = op_tok->getVal();
  // Convert types if needed
  if (lhs_tm != cast_tm) {
    lhs_handle = convert(lhs_tm, cast_tm, lhs_handle);
  }
  if (rhs_tm != cast_tm) {
    rhs_handle = convert(rhs_tm, cast_tm, rhs_handle);
  }
  std::string type = getLlvmType(cast_tm);
  // Select the operation
  std::string opcode;
  std::string op_type;
  std::string prefix;
  if (cast_tm == TYPE_FLT) prefix = "f";
  switch (tt) {
    case TOK_OP_ARITH:
      if (op == "+") {
        opcode = "add";
      } else {
        opcode = "sub";
      }
      break;
    case TOK_OP_TERM:
      if (op == "*") {
        opcode = "mul";
      } else {
        opcode = "div";
        if (cast_tm == TYPE_INT) prefix = "s";  // signed division
      }
      break;
    case TOK_OP_RELAT:
      opcode = "cmp";
      if (cast_tm == TYPE_INT) prefix = "i";
      if (op == "==") {
        op_type = "eq ";
      } else if (op == "<") {
        op_type = "slt ";
      } else if (op == "<=") {
        op_type = "sle ";
      } else if (op == ">") {
        op_type = "sgt ";
      } else if (op == ">=") {
        op_type = "sge ";
      } else if (op == "!=") {
        op_type = "ne ";
      }
      break;
    case TOK_OP_EXPR:
      if (op == "&") {
        opcode = "and";
      } else {
        opcode = "or";
      }
    default:
      LOG(ERROR) << "Could not generate binary operator";
  }
  // Do the operation
  std::string res_reg = "%" + std::to_string(fun->reg_count++);
  fun->llvm_code << res_reg << " = " << prefix << opcode << " " << op_type
    << type << " " << lhs_handle << ", " << rhs_handle << "\n";
  return res_reg;
}

std::string CodeGen::unaryOp(std::string lhs_handle, const TypeMark& lhs_tm,
     std::shared_ptr<Token> op_tok) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot op with empty function stack";
    return "BAD_REG";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Make sure op token is okay
  if (op_tok == nullptr || op_tok->getType() == TOK_INVALID) {
    LOG(ERROR) << "Cannot generate binary op; invalid token";
    return "BAD_REG";
  }
  TokenType tt = op_tok->getType();
  std::string op = op_tok->getVal();

  // Emit operations
  std::string res_reg = "%" + std::to_string(fun->reg_count++);
  std::string type = getLlvmType(lhs_tm);
  if (tt == TOK_OP_ARITH && op == "-") {
    if (lhs_tm == TYPE_INT || lhs_tm == TYPE_BOOL) {
      fun->llvm_code << res_reg << " = sub " << type << " 0, " << lhs_handle
        << "\n";
    } else {
      fun->llvm_code << res_reg << " = fsub " << type << " 0.0, " << lhs_handle
        << "\n";
    }
  } else if (tt == TOK_OP_EXPR && op == "not") {
    // xor w/ -1 because no bitwise not in llvm
    fun->llvm_code << res_reg << " = xor " << type << " -1, " << lhs_handle
      << "\n";
  } else {
    LOG(ERROR) << "Could not generate unary operator";
  }
  return res_reg;
}

std::string CodeGen::procCallBegin(const std::string& proc_handle,
    const TypeMark& tm) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot call with empty function stack";
    return "BAD_REG";
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  std::string res_reg = "%" + std::to_string(fun->reg_count++);
  fun->llvm_code << res_reg << " = call " << getLlvmType(tm) << " "
    << proc_handle << "(";
  return res_reg;
}

void CodeGen::procArg(std::string arg_handle, const TypeMark& arg_tm,
    const TypeMark& param_tm, const bool& comma) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add arg with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  if (comma) fun->llvm_code << ", ";
  if (arg_tm != param_tm) {
    arg_handle = convert(arg_tm, param_tm, arg_handle);
  }
  fun->llvm_code << getLlvmType(param_tm) << " " << arg_handle;
}

void CodeGen::procCallEnd() {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot close call with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  fun->llvm_code << ")\n";
}

void CodeGen::returnStmt(std::string expr_handle, const TypeMark& expr_tm,
    const TypeMark& ret_tm) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add return with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  if (expr_tm != ret_tm) {
    expr_handle =convert(expr_tm, ret_tm, expr_handle);
  }
  fun->llvm_code << "ret " << getLlvmType(ret_tm) << " " << expr_handle
    << "\n\n";
  fun->in_basic_block = false;
}

void CodeGen::ifStmt(std::string expr_handle, const TypeMark& expr_tm) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  if (expr_tm != TYPE_BOOL) {
    expr_handle =convert(expr_tm, TYPE_BOOL, expr_handle);
  }
  std::string then_label = ".then." + std::to_string(fun->if_count);
  std::string else_label = ".else." + std::to_string(fun->if_count);
  fun->if_stack.push(fun->if_count++);
  fun->llvm_code << "br i1 " << expr_handle << ", label %" << then_label
    << ", label %" << else_label << "\n\n";
  fun->llvm_code << then_label << ":\n";
}

void CodeGen::elseStmt() {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Also check if stack
  if (fun->if_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty if stack";
    return;
  }
  int if_num = fun->if_stack.top();
  std::string else_label = ".else." + std::to_string(if_num);
  std::string end_label = ".endif." + std::to_string(if_num);
  if (fun->in_basic_block) {
    fun->llvm_code << "br label %" << end_label << "\n\n";
  }
  fun->llvm_code << else_label << ":\n";
}

void CodeGen::endIf() {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Also check if stack
  if (fun->if_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty if stack";
    return;
  }
  int if_num = fun->if_stack.top();
  fun->if_stack.pop();
  std::string end_label = ".endif." + std::to_string(if_num);
  if (fun->in_basic_block) {
    fun->llvm_code << "br label %" << end_label << "\n\n";
  }
  fun->llvm_code << end_label << ":\n";
}

void CodeGen::forLabel() {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  std::string for_label = ".for." + std::to_string(fun->loop_count);
  if (fun->in_basic_block) {
    fun->llvm_code << "br label %" << for_label << "\n\n";
  }
  fun->llvm_code << for_label << ":\n";
  fun->loop_stack.push(fun->loop_count);
}

void CodeGen::forStmt(std::string expr_handle, const TypeMark& expr_tm) {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add if with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Also check loop stack
  if (fun->loop_stack.empty()) {
    LOG(ERROR) << "Cannot add loop with empty loop stack";
    return;
  }
  int for_num = fun->loop_stack.top();
  if (expr_tm != TYPE_BOOL) {
    expr_handle =convert(expr_tm, TYPE_BOOL, expr_handle);
  }
  std::string body_label = ".body." + std::to_string(for_num);
  std::string end_label = ".endfor." + std::to_string(for_num);
  fun->if_stack.push(fun->if_count++);
  fun->llvm_code << "br i1 " << expr_handle << ", label %" << body_label
    << ", label %" << end_label << "\n\n";
  fun->llvm_code << body_label << ":\n";
}
void CodeGen::endFor() {
  // Make sure stack is okay
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot add for with empty function stack";
    return;
  }
  std::shared_ptr<struct Function> fun = function_stack.top();
  // Also check for stack
  if (fun->loop_stack.empty()) {
    LOG(ERROR) << "Cannot add for with empty for stack";
    return;
  }
  int for_num = fun->loop_stack.top();
  fun->loop_stack.pop();
  std::string end_label = ".endfor." + std::to_string(for_num);
  if (fun->in_basic_block) {
    fun->llvm_code << "br label %" << end_label << "\n\n";
  }
  fun->llvm_code << end_label << ":\n";
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
  return ret + "  ; auto-generated return";
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
  function_stack.top()->llvm_code << ss.str() << "\n";
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
