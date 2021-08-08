#include "parser.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "code_gen.h"
#include "environment.h"
#include "log.h"
#include "scanner.h"
#include "token.h"
#include "type_checker.h"

Parser::Parser() : env(new Environment()), scanner(env), code_gen(),
    panic_mode(false) {}

bool Parser::init(const std::string& src_file) {
  bool init_success = true;
  panic_mode = false;
  if (!scanner.init(src_file)) {
    init_success = false;
    LOG(ERROR) << "Failed to initialize parser";
    LOG(ERROR) << "See logs";
  } else {
    scan();
  }
  auto main_fun = std::make_shared<IdToken>(TOK_IDENT, "main");
  main_fun->setTypeMark(TYPE_INT);
  main_fun->setProcedure(true);
  code_gen.addFunction(main_fun);
  return init_success;
}

//  <program> ::=
//    <program_header> <program_body> `.'
bool Parser::parse() {
  LOG(INFO) << "Begin parsing";
  LOG(DEBUG) << "<program>";
  programHeader();
  programBody();
  expectToken(TOK_PERIOD);
  scan();
  LOG(INFO) << "Done parsing";
  code_gen.closeFunction();
  if (LOG::hasErrored()) {
    LOG(WARN) << "Parsing had errors; no code generated";
  }{// else {  // TODO: Add back in the else
    LOG(INFO) << "Emitting code:\n" << code_gen.emitCode();
  }
  if (tok->getType() != TOK_EOF) {
    LOG(WARN) << "Done parsing but not EOF.";
  }
  return !LOG::hasErrored();
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

void Parser::scan() {
  do {
    tok = scanner.getToken();
  } while(tok->getType() == TOK_INVALID);
}

void Parser::push_scope(std::shared_ptr<IdToken> id_tok) {
  LOG(DEBUG) << "Pushing local scope for function " << id_tok->getVal();
  env->push();
  function_stack.push(id_tok);

  // Procedure must be locally visible for recursion
  env->insert(id_tok->getVal(), id_tok, false);
}

void Parser::pop_scope() {
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot pop empty function stack";
  } else {
    LOG(DEBUG) << "Popping local scope for function "
      << function_stack.top()->getVal() << ":\n" << env->getLocalStr();
    env->pop();
    function_stack.pop();
    code_gen.closeFunction();
  }
}

bool Parser::matchToken(const TokenType& t) {
  if (tok->getType() == t) {
    LOG(DEBUG) << "Matched token " << Token::getTokenName(t);
    return true;
  }
  return false;
}

bool Parser::expectToken(const TokenType& t) {
  if (matchToken(t)) {
    LOG(DEBUG) << "Expect passed for token " << Token::getTokenName(t);
    return true;
  }
  LOG(ERROR) << "Expected " << Token::getTokenName(t)
      << ", got " << tok->getStr() << " instead";
  panic();
  return false;
}

void Parser::panic() {

  // Flag that panic mode happened so the rest of the parser can respond
  panic_mode = true;
  LOG(ERROR) << "Start panic mode";
  LOG(ERROR) << "Scanning for `;' or `EOF'";

  // Eat tokens until a sync point
  // Currently syncing on semicolon and EOF
  // Pretty basic for now, but a more advanced solution would require
  // significant infrastructure and refactoring
  while (tok->getType() != TOK_SEMICOL && tok->getType() != TOK_EOF) {
    scan();
  }
}

//  <program_header> ::=
//    `program' <identifier> `is'
void Parser::programHeader() {

  // Not going to worry about panic mode here.
  // If this part of the program is not correct, I have little hope for the
  // rest of the program...
  LOG(DEBUG) << "<program_header>";
  expectToken(TOK_RW_PROG);
  if (panic_mode) return;
  scan();
  identifier(false);
  if (panic_mode) return;
  expectToken(TOK_RW_IS);
  if (panic_mode) return;
  scan();
}

//  <program_body> ::=
//      <declarations>
//    `begin'
//      <statements>
//    `end' `program'
void Parser::programBody() {
  LOG(DEBUG) << "<program_body>";
  declarations(true); // These are global declarations by default
  LOG(DEBUG) << "Done parsing global declarations";
  LOG(DEBUG) << "Global symbol table:\n" << env->getGlobalStr();
  expectToken(TOK_RW_BEG);
  scan();
  statements();
  expectToken(TOK_RW_END);
  scan();
  expectToken(TOK_RW_PROG);
  scan();
}

//  <declarations> ::=
//    (<declaration>`;')*
void Parser::declarations(bool is_global) {
  LOG(DEBUG) << "<declarations>";
  // FIRST(<declaration>) = {global, procedure, variable}
  while (matchToken(TOK_RW_GLOB) || matchToken(TOK_RW_PROC)
      || matchToken(TOK_RW_VAR)) {
    panic_mode = false;  // Reset panic mode
    declaration(is_global);

    // Even if we entered panic mode, this should pass unless we hit EOF
    expectToken(TOK_SEMICOL);
    scan();
  }
}

//  <statements> ::=
//    (<statement>`;')*
void Parser::statements() {
  LOG(DEBUG) << "<statements>";
  // FIRST(<statement>) = {<identifier>, if, for, return}
  while(matchToken(TOK_IDENT) || matchToken(TOK_RW_IF) || matchToken(TOK_RW_FOR)
      || matchToken(TOK_RW_RET)) {
    panic_mode = false;  // Reset panic mode
    statement();

    // Even if we entered panic mode, this should pass unless we hit EOF
    expectToken(TOK_SEMICOL);
    scan();
  }
}

//  <declaration> ::=
//    [`global'] <procedure_declaration>
//  | [`global'] <variable_declaration>
void Parser::declaration(bool is_global) {
  LOG(DEBUG) << "<declaration>";
  if (matchToken(TOK_RW_GLOB)) {
    is_global = true;
    scan();
  }
  if (matchToken(TOK_RW_PROC)) {
    procedureDeclaration(is_global);
  } else if(matchToken(TOK_RW_VAR)) {
    variableDeclaration(is_global);
  } else {
    LOG(ERROR) << "Unexpected token: " << tok->getStr();
    LOG(ERROR) << "Expected: " << Token::getTokenName(TOK_RW_PROC) << " or "
        << Token::getTokenName(TOK_RW_VAR);
    panic();
  }
}

//  <procedure_declaration> ::=
//    <procedure_header> <procedure_body>
void Parser::procedureDeclaration(const bool& is_global) {
  LOG(DEBUG) << "<procedure_declaration>";
  panic_mode = false;  // Reset panic mode
  procedureHeader(is_global);
  panic_mode = false;  // Reset panic mode
  procedureBody();
  pop_scope();
}

//  <procedure_header>
//    `procedure' <identifier> `:' <type_mark> `('[<parameter_list>]`)'
void Parser::procedureHeader(const bool& is_global) {
  LOG(DEBUG) << "<procedure_header>";
  expectToken(TOK_RW_PROC);
  if (panic_mode) return;  // No need to continue
  scan();
  std::shared_ptr<IdToken> id_tok = identifier(false);
  env->insert(id_tok->getVal(), id_tok, is_global);
  expectToken(TOK_COLON);
  if (panic_mode) return;  // No need to continue
  scan();
  TypeMark tm = typeMark();
  id_tok->setTypeMark(tm);
  id_tok->setProcedure(true);

  // Begin new scope
  push_scope(id_tok);  // This adds id_tok to the new scope for recursion
  expectToken(TOK_LPAREN);
  scan();
  if (matchToken(TOK_RW_VAR)) {
    parameterList();
  }
  expectToken(TOK_RPAREN);
  scan();
  code_gen.addFunction(id_tok);
}

//  <parameter_list> ::=
//    <parameter>`,' <parameter_list>
//  | <parameter>
void Parser::parameterList() {
  LOG(DEBUG) << "<parameter_list>";
  std::shared_ptr<IdToken> par_tok = parameter();
  if (!par_tok->isValid()) {
    LOG(ERROR) << "Ill-formed parameter: " << par_tok->getStr() << "; skipping";
  } else {
    function_stack.top()->addParam(par_tok);
  }

  if (matchToken(TOK_COMMA)) {
    scan();
    parameterList();
  }
}

//  <parameter> ::=
//    <variable_declaration>
std::shared_ptr<IdToken> Parser::parameter() {
  LOG(DEBUG) << "<parameter>";
  return variableDeclaration(false);
}

//  <procedure_body> ::=
//      <statements>
//    `begin'
//      <statements>
//    `end' `procedure'
void Parser::procedureBody() {
  LOG(DEBUG) << "<procedure_body>";
  declarations(false);
  expectToken(TOK_RW_BEG);
  scan();
  statements();
  expectToken(TOK_RW_END);
  scan();
  expectToken(TOK_RW_PROC);
  scan();
}

//  <variable_declaration> ::=
//    `variable' <identifier> `:' <type_mark> [`['<bound>`]']
std::shared_ptr<IdToken> Parser::variableDeclaration(const bool& is_global) {
  LOG(DEBUG) << "<variable_declaration>";

  // Making this in case panic mode happens before the call to identifier()
  // I don't want to return a nullptr
  std::shared_ptr<IdToken> id_tok(new IdToken(TOK_INVALID, ""));
  expectToken(TOK_RW_VAR);
  if (panic_mode) return id_tok;  // No need to continue
  scan();
  id_tok = identifier(false);
  env->insert(id_tok->getVal(), id_tok, is_global);
  expectToken(TOK_COLON);
  if (panic_mode) return id_tok;  // No need to continue
  scan();
  TypeMark tm = typeMark();
  id_tok->setTypeMark(tm);
  id_tok->setProcedure(false);
  if (matchToken(TOK_LBRACK)) {
    LOG(DEBUG) << "Variable is an array";
    scan();
    id_tok->setNumElements(bound());
    expectToken(TOK_RBRACK);
    if (panic_mode) return id_tok;  // No need to continue
    scan();
  }
  code_gen.declareVariable(id_tok, is_global);
  LOG(DEBUG) << "Declared variable " << id_tok->getStr();
  return id_tok;
}

//  <type_mark> ::=
//    `integer' | `float' | `string' | `bool'
TypeMark Parser::typeMark() {
  LOG(DEBUG) << "<type_mark>";
  TypeMark tm = TYPE_NONE;
  if (matchToken(TOK_RW_INT)) {
    tm = TYPE_INT;
  }
  else if (matchToken(TOK_RW_FLT)) {
    tm = TYPE_FLT;
  }
  else if (matchToken(TOK_RW_STR)) {
    tm = TYPE_STR;
  }
  else if (matchToken(TOK_RW_BOOL)) {
    tm = TYPE_BOOL;
  }
  else {
    LOG(ERROR) << "Expected type mark, got: " << tok->getVal();
    panic();
    return tm;
  }
  scan();
  return tm;
}

//  <bound> ::=
//    <number>
int Parser::bound() {
  LOG(DEBUG) << "<bound>";
  std::shared_ptr<Token> num_tok = number();
  std::shared_ptr<LiteralToken<int>> bound_tok =
      std::dynamic_pointer_cast<LiteralToken<int>>(num_tok);
  if (bound_tok) {
    int bound_val = bound_tok->getVal();
    if (bound_val < 1) {
      LOG(ERROR) << "Bound must be at least 1; received bound " << bound_val;
      LOG(WARN) << "Using bound of 1";
      return 1;
    }
    return bound_val;
  } else {
    LOG(ERROR) << "Invalid bound received: " << num_tok->getVal();
    LOG(WARN) << "Using bound of 1";
    return 1;
  }
}

//  <statement> ::=
//    <assignment_statement>
//  | <if_statement>
//  | <loop_statement>
//  | <return_statement>
void Parser::statement() {
  LOG(DEBUG) << "<statement>";
  if (matchToken(TOK_IDENT)) {
    assignmentStatement();
  } else if (matchToken(TOK_RW_IF)) {
    ifStatement();
  } else if (matchToken(TOK_RW_FOR)) {
    loopStatement();
  } else if (matchToken(TOK_RW_RET)) {
    returnStatement();
  } else {
    LOG(ERROR) << "Unexpected token: " << tok->getVal()
        << "; expected statement";
    panic();
  }
}

//  <procedure_call> ::=
//    <identifier>`('[<argument_list>]`)'
TypeMark Parser::procedureCall() {
  LOG(DEBUG) << "<procedure_call>";
  std::shared_ptr<IdToken> id_tok = identifier(true);
  if (!id_tok->getProcedure()) {
    LOG(ERROR) << "Expected procedure; got variable " << id_tok->getVal();
  }
  expectToken(TOK_LPAREN);
  if (panic_mode) return TYPE_NONE;  // No need to continue
  scan();
  if (!matchToken(TOK_RPAREN)) {
    argumentList(0, id_tok);
  }
  expectToken(TOK_RPAREN);
  if (!panic_mode) scan();
  return id_tok->getTypeMark();
}

//  <assignment_statement> ::=
//    <destination> `:=' <expression>
void Parser::assignmentStatement() {
  LOG(DEBUG) << "<assignment_statement>";
  int dest_size = 0;
  TypeMark tm_dest = destination(dest_size);
  expectToken(TOK_OP_ASS);
  if (panic_mode) return;  // No need to continue
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int expr_size = 0;
  TypeMark tm_expr = expression(expr_size);
  type_checker::checkCompatible(op_tok, tm_dest, tm_expr);
  type_checker::checkArraySize(op_tok, dest_size, expr_size);
}

//  <destination> ::=
//    <identifier>[`['<expression>`]']
TypeMark Parser::destination(int& size) {
  LOG(DEBUG) << "<destination>";
  expectToken(TOK_IDENT);
  if (panic_mode) return TYPE_NONE;  // No need to continue
  std::shared_ptr<IdToken> id_tok = identifier(true);
  if (id_tok->getProcedure()) {
    LOG(ERROR) << "Expected variable; got procedure " << id_tok->getVal();
  }
  TypeMark tm = id_tok->getTypeMark();
  size = id_tok->getNumElements();
  if (matchToken(TOK_LBRACK)) {
    LOG(DEBUG) << "Indexing array";
    size = 0;  // If indexing, it's a single element not an array
    if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
      LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
    }
    scan();
    int idx_size = 0;
    TypeMark tm_idx = expression(idx_size);
    type_checker::checkArrayIndex(tm_idx);
    if (idx_size > 0) {
      LOG(ERROR) << "Invalid index; Expected scalar, got array";
    }
    expectToken(TOK_RBRACK);
    if (panic_mode) return TYPE_NONE;  // No need to continue
    scan();
  }
  return tm;
}

//  <if_statement> ::=
//    `if' `(' <expression> `)' `then' <statements>
//    [`else' <statements>]
//    `end' `if'
void Parser::ifStatement() {
  LOG(DEBUG) << "<if_statement>";
  expectToken(TOK_RW_IF);
  if (panic_mode) return;  // No need to continue
  scan();
  expectToken(TOK_LPAREN);
  if (panic_mode) return;  // No need to continue
  scan();

  // Ensure expression parses to `bool'
  int expr_size = 0;
  TypeMark tm = expression(expr_size);
  if (!type_checker::checkCompatible(tm, TYPE_BOOL)) {
    LOG(ERROR) << "Invalid if statement expression of type "
      << Token::getTypeMarkName(tm) << " received";
    LOG(ERROR) << "If statement expression must resolve to type "
        << Token::getTypeMarkName(TYPE_BOOL);
  } else if (expr_size > 0) {
    LOG(ERROR) << "Invalid if statement; expected scalar, got array";
  }
  expectToken(TOK_RPAREN);
  if (panic_mode) return;  // No need to continue
  scan();
  expectToken(TOK_RW_THEN);
  if (panic_mode) return;  // No need to continue
  scan();
  statements();
  if (matchToken(TOK_RW_ELSE)) {
    LOG(DEBUG) << "Else";
    scan();
    statements();
  }
  expectToken(TOK_RW_END);
  if (panic_mode) return;  // No need to continue
  scan();
  expectToken(TOK_RW_IF);
  if (panic_mode) return;  // No need to continue
  scan();
}

//  <loop_statement> ::=
//    `for' `(' <assignment_statement>`;' <expression> `)'
//      <statements>
//    `end' `for'
void Parser::loopStatement() {
  LOG(DEBUG) << "<loop_statement>";
  expectToken(TOK_RW_FOR);
  if (panic_mode) return;  // No need to continue
  scan();
  expectToken(TOK_LPAREN);
  if (panic_mode) return;  // No need to continue
  scan();
  assignmentStatement();
  expectToken(TOK_SEMICOL);
  if (panic_mode) return;  // No need to continue
  scan();

  // Ensure expression parses to `bool'
  int expr_size = 0;
  TypeMark tm = expression(expr_size);
  if (!type_checker::checkCompatible(tm, TYPE_BOOL)) {
    LOG(ERROR) << "Invalid loop statement expression of type "
      << Token::getTypeMarkName(tm) << " received";
    LOG(ERROR) << "Loop statement expression must resolve to type "
        << Token::getTypeMarkName(TYPE_BOOL);
  } else if (expr_size > 0) {
    LOG(ERROR) << "Invalid loop statement; expected scalar, got array";
  }
  expectToken(TOK_RPAREN);
  if (panic_mode) return;  // No need to continue
  scan();
  statements();
  expectToken(TOK_RW_END);
  if (panic_mode) return;  // No need to continue
  scan();
  expectToken(TOK_RW_FOR);
  if (panic_mode) return;  // No need to continue
  scan();
}

//  <return_statement> ::=
//    `return' <expression>
void Parser::returnStatement() {
  LOG(DEBUG) << "<return_statement>";
  expectToken(TOK_RW_RET);
  if (panic_mode) return;  // No need to continue
  scan();

  // Make sure <expression> type matches return type for this function
  int expr_size = 0;
  TypeMark tm_expr = expression(expr_size);
  TypeMark tm_ret = function_stack.top()->getTypeMark();
  if (!type_checker::checkCompatible(tm_expr, tm_ret)) {
    LOG(ERROR) << "Expression type " << Token::getTypeMarkName(tm_expr)
        << " not compatible with return type "
        << Token::getTypeMarkName(tm_ret);
  }

  // Return types are scalar only (unless I misunderstand the spec)
  if (expr_size > 0) {
    LOG(ERROR) << "Invalid return type; expected scalar, got array";
  }
}

//  <identifier> ::=
//    [a-zA-Z][a-zA-Z0-9_]*
std::shared_ptr<IdToken> Parser::identifier(const bool& lookup) {
  LOG(DEBUG) << "<identifier>";
  std::shared_ptr<IdToken> id_tok;
  if (expectToken(TOK_IDENT)) {
    id_tok = std::dynamic_pointer_cast<IdToken>(tok);
    scan();
    if (lookup) {
      id_tok = std::dynamic_pointer_cast<IdToken>(env->lookup(id_tok->getVal(),
          true));
      if (!id_tok) {
        id_tok = std::shared_ptr<IdToken>(new IdToken(TOK_INVALID, ""));
      }
    }
  } else {
    id_tok = std::shared_ptr<IdToken>(new IdToken(TOK_INVALID, ""));
  }
  return id_tok;
}

//  <expression> ::=
//    [`not'] <arith_op> <expression_prime>
TypeMark Parser::expression(int& size) {
  LOG(DEBUG) << "<expression>";
  bool bitwise_not = matchToken(TOK_RW_NOT);
  std::shared_ptr<Token> op_tok;
  if (bitwise_not) {
    LOG(DEBUG) << "Bitwise not";
    op_tok = std::shared_ptr<Token>(new Token(TOK_OP_EXPR, "not"));
    scan();
  }
  TypeMark tm_arith = arithOp(size);

  // Check type compatibility for bitwise not
  if (bitwise_not) {
    type_checker::checkCompatible(op_tok, tm_arith);
    type_checker::checkArraySize(op_tok, size);
  }
  return expressionPrime(tm_arith, size);
}

//  <expression_prime> ::=
//    `&' <arith_op> <expression_prime>
//  | `|' <arith_op> <expression_prime>
//  | epsilon
TypeMark Parser::expressionPrime(const TypeMark& tm, int& size) {
  LOG(DEBUG) << "<expression_prime>";
  if (!matchToken(TOK_OP_EXPR)) {
    LOG(DEBUG) << "epsilon";
  } else {
    std::shared_ptr<Token> op_tok = tok;
    scan();
    int arith_size = 0;
    TypeMark tm_arith = arithOp(arith_size);
    type_checker::checkCompatible(op_tok, tm, tm_arith);
    type_checker::checkArraySize(op_tok, size, arith_size);
    size = std::max(size, arith_size);
    expressionPrime(tm_arith, size);
  }
  return tm;
}

//  <arith_op> ::=
//    <relation> <arith_op_prime>
TypeMark Parser::arithOp(int& size) {
  LOG(DEBUG) << "<arith_op>";
  TypeMark tm_relat = relation(size);

  // tm_relat and tm_arith are checked in arithOpPrime
  return arithOpPrime(tm_relat, size);
}

//  <arith_op_prime> ::=
//    `+' <relation> <arith_op_prime>
//  | `-' <relation> <arith_op_prime>
//  | epsilon
TypeMark Parser::arithOpPrime(const TypeMark& tm, int& size) {
  LOG(DEBUG) << "<arith_op_prime>";
  if (!matchToken(TOK_OP_ARITH)) {
    LOG(DEBUG) << "epsilon";
    return tm;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int relat_size = 0;
  TypeMark tm_relat = relation(relat_size);
  type_checker::checkCompatible(op_tok, tm, tm_relat);
  type_checker::checkArraySize(op_tok, size, relat_size);

  // If either side is `float', cast to `float'
  TypeMark tm_result;
  if ((tm == TYPE_FLT) || (tm_relat == TYPE_FLT)) {
    tm_result = TYPE_FLT;
  } else {
    tm_result = TYPE_INT;
  }
  size = std::max(size, relat_size);
  return arithOpPrime(tm_result, size);
}

//  <relation> ::=
//    <term> <relation_prime>
TypeMark Parser::relation(int& size) {
  LOG(DEBUG) << "<relation>";
  TypeMark tm_term = term(size);
  return relationPrime(tm_term, size);
}

//  <relation_prime> ::=
//    `<' <term> <relation_prime>
//  | `>=' <term> <relation_prime>
//  | `<=' <term> <relation_prime>
//  | `>' <term> <relation_prime>
//  | `==' <term> <relation_prime>
//  | `!=' <term> <relation_prime>
//  | epsilon
TypeMark Parser::relationPrime(const TypeMark& tm, int& size) {
  LOG(DEBUG) << "<relation_prime>";
  if (!matchToken(TOK_OP_RELAT)) {
    LOG(DEBUG) << "epsilon";
    return tm;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int term_size = 0;
  TypeMark tm_term = term(term_size);
  type_checker::checkCompatible(op_tok, tm, tm_term);
  type_checker::checkArraySize(op_tok, size, term_size);
  size = std::max(size, term_size);
  relationPrime(tm_term, size);
  return TYPE_BOOL;
}

//  <term> ::=
//    <factor> <term_prime>
TypeMark Parser::term(int& size) {
  LOG(DEBUG) << "<term>";
  TypeMark tm_fact = factor(size);
  return termPrime(tm_fact, size);
}

//  <term_prime> ::=
//    `*' <factor> <term_prime>
//  | `/' <factor> <term_prime>
//  | epsilon
TypeMark Parser::termPrime(const TypeMark& tm, int& size) {
  LOG(DEBUG) << "<term_prime>";
  if (!matchToken(TOK_OP_TERM)) {
    LOG(DEBUG) << "epsilon";
    return tm;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int fact_size = 0;
  TypeMark tm_fact = factor(fact_size);
  type_checker::checkCompatible(op_tok, tm, tm_fact);
  type_checker::checkArraySize(op_tok, size, fact_size);

  // If either side is `float', cast to `float'
  TypeMark tm_result;
  if ((tm == TYPE_FLT) || (tm_fact == TYPE_FLT)) {
    tm_result = TYPE_FLT;
  } else {
    tm_result = TYPE_INT;
  }
  size = std::max(size, fact_size);
  return termPrime(tm_result, size);
}

//  <factor> ::=
//    `('<expression>`)'
//  | <procedure_call>
//  | [`-'] <name>
//  | [`-'] <number>
//  | <string>
//  | `true'
//  | `false'
TypeMark Parser::factor(int& size) {
  LOG(DEBUG) << "<factor>";
  TypeMark tm = TYPE_NONE;

  // Negative sign can only happen before <number> and <name>
  if (matchToken(TOK_OP_ARITH) && (tok->getVal() == "-")) {
    scan();
    if (matchToken(TOK_IDENT)) {
      std::shared_ptr<IdToken> id_tok = std::dynamic_pointer_cast<IdToken>(
          env->lookup(tok->getVal(), false));
      if (!id_tok) {
        LOG(ERROR) << "Identifier not declared in this scope: "
            << tok->getStr();
      } else if (!id_tok->getProcedure()) {
        tm = name(size);
      } else {
        LOG(ERROR) << "Expected variable; got: " << tok->getStr();
      }
    } else if (matchToken(TOK_NUM)) {
      std::shared_ptr<Token> num_tok = number();
      tm = num_tok->getTypeMark();
      size = 0;  // <number> literals are scalar
    } else {
      LOG(ERROR) << "Minus sign must be followed by <name> or <number>.";
      LOG(ERROR) << "Got: " << tok->getStr();
    }

  // `('<expression>`)'
  } else if (matchToken(TOK_LPAREN)) {
    scan();
    tm = expression(size);
    expectToken(TOK_RPAREN);
    if (panic_mode) return tm;  // No need to continue
    scan();

  // <procedure_call> or <name>
  } else if (matchToken(TOK_IDENT)) {
    std::shared_ptr<IdToken> id_tok = std::dynamic_pointer_cast<IdToken>(
        env->lookup(tok->getVal(), false));
    if (!id_tok) {
      LOG(ERROR) << "Identifier not declared in this scope: "
          << tok->getStr();
    } else if (id_tok->getProcedure()) {
      tm = procedureCall();
      size = 0;  // Procedure calls return scalars
    } else {
      tm = name(size);
    }

  // <number>
  } else if (matchToken(TOK_NUM)) {
    std::shared_ptr<Token> num_tok = number();
    tm = num_tok->getTypeMark();
    size = 0;  // <number> literals are scalars

  // <string>
  } else if (matchToken(TOK_STR)) {
    std::shared_ptr<LiteralToken<std::string>> str_tok = string();
    tm = str_tok->getTypeMark();
    size = 0;  // <string> literals are scalars

  // `true'
  } else if (matchToken(TOK_RW_TRUE)) {
    tm = TYPE_BOOL;
    size = 0;  // `true' literals are scalars
    scan();

  // `false'
  } else if (matchToken(TOK_RW_FALSE)) {
    tm = TYPE_BOOL;
    size = 0;  // `false' literals are scalars
    scan();

  // Oof
  } else {
    LOG(ERROR) << "Unexpected token: " << tok->getStr();
    tm = TYPE_NONE;
    size = 0;
    panic();
  }
  return tm;
}

//  <name> ::=
//    <identifier> [`['<expression>`]']
TypeMark Parser::name(int& size) {
  LOG(DEBUG) << "<name>";
  std::shared_ptr<IdToken> id_tok = identifier(true);
  if (id_tok->getProcedure()) {
    LOG(ERROR) << "Expected variable; got procedure " << id_tok->getVal();
  }
  TypeMark tm = id_tok->getTypeMark();
  size = id_tok->getNumElements();
  if (matchToken(TOK_LBRACK)) {
    LOG(DEBUG) << "Indexing array";
    size = 0;  // If indexing, it's a single element not an array
    if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
      LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
    }
    scan();
    int idx_size = 0;
    TypeMark tm_idx = expression(idx_size);
    type_checker::checkArrayIndex(tm_idx);
    if (idx_size > 0) {
      LOG(ERROR) << "Invalid index; expected scalar, got array";
    }
    expectToken(TOK_RBRACK);
    if (panic_mode) return tm;  // No need to continue
    scan();
  }
  return tm;
}

//  <argument_list> ::=
//    <expression> `,' <argument_list>
//  | <expression>
void Parser::argumentList(const int& idx, std::shared_ptr<IdToken> fun_tok) {
  LOG(DEBUG) << "<argument_list>";
  int expr_size = 0;
  TypeMark tm_arg = expression(expr_size);
  std::shared_ptr<IdToken> param = fun_tok->getParam(idx);
  if (!param) {
    LOG(ERROR) << "Unexpected parameter with type "
        << Token::getTypeMarkName(tm_arg);
  } else if (!type_checker::checkCompatible(param->getTypeMark(), tm_arg)) {
    LOG(ERROR) << "Expected parameter with type "
        << Token::getTypeMarkName(param->getTypeMark()) << "; got "
        << Token::getTypeMarkName(tm_arg);
  } else if (expr_size != param->getNumElements()) {
    LOG(ERROR) << "Size of argument (" << expr_size
        << ") != size of parameter (" << param->getNumElements() << ")";
  }
  if (matchToken(TOK_COMMA)) {
    scan();
    argumentList(idx + 1, fun_tok);
  } else if (idx < fun_tok->getNumElements() - 1) {
    LOG(ERROR) << "Not enough parameters for procedure call "
        << fun_tok->getVal();
  }
}

//  <number> ::=
//    [0-9][0-9_]*[.[0-9_]*]
std::shared_ptr<Token> Parser::number() {
  LOG(DEBUG) << "<number>";
  std::shared_ptr<Token> num_tok = std::shared_ptr<Token>(new Token());
  if (expectToken(TOK_NUM)) {
    num_tok = tok;
  }
  if (!panic_mode) scan();
  return num_tok;
}

//  <string> ::=
//    `"'[^"]*`"'
std::shared_ptr<LiteralToken<std::string>> Parser::string() {
  LOG(DEBUG) << "<string>";
  std::shared_ptr<LiteralToken<std::string>> str_tok =
      std::shared_ptr<LiteralToken<std::string>>(
        new LiteralToken<std::string>(TOK_STR, "", TYPE_STR));
  if (expectToken(TOK_STR)) {
    str_tok = std::dynamic_pointer_cast<LiteralToken<std::string>>(tok);
  } else {
    LOG(ERROR) << "Using empty string";
  }
  if (!panic_mode) scan();
  return str_tok;
}
