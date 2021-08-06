#include "parser.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "ast.h"
#include "environment.h"
#include "log.h"
#include "scanner.h"
#include "token.h"
#include "type_checker.h"

Parser::Parser() : env(new Environment()), scanner(env), type_checker(),
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
  return init_success;
}

//  <program> ::=
//    <program_header> <program_body> `.'
std::unique_ptr<ast::Program> Parser::parse() {
  LOG(INFO) << "Begin parsing";
  LOG(DEBUG) << "<program>";
  auto prog = std::make_unique<ast::Program>(programHeader(), programBody());
  expect(TOK_PERIOD);
  scan();
  LOG(INFO) << "Done parsing";
  // TODO: Should I type check here or from main? Main might make more sense
  return prog;
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

void Parser::scan() {
  do {
    tok = scanner.getToken();
  } while(tok->getType() == TOK_INVALID);
}

void Parser::pushScope(std::shared_ptr<IdToken> id_tok) {
  LOG(DEBUG) << "Pushing local scope for function " << id_tok->getVal();
  env->push();
  function_stack.push(id_tok);

  // Procedure must be locally visible for recursion
  env->insert(id_tok->getVal(), id_tok, false);
}

void Parser::popScope() {
  if (function_stack.empty()) {
    LOG(ERROR) << "Cannot pop empty function stack";
  } else {
    LOG(DEBUG) << "Popping local scope for function "
      << function_stack.top()->getVal() << ":\n" << env->getLocalStr();
    env->pop();
    function_stack.pop();
  }
}

bool Parser::match(const TokenType& t) {
  if (tok->getType() == t) {
    LOG(DEBUG) << "Matched token " << Token::getTokenName(t);
    return true;
  }
  return false;
}

bool Parser::expect(const TokenType& t) {
  if (match(t)) {
    LOG(DEBUG) << "Expect passed for token " << Token::getTokenName(t);
    return true;
  }
  LOG(ERROR) << "Expected " << Token::getTokenName(t)
      << ", got " << tok->getStr() << " instead";
  return false;
}

bool Parser::expectScan(const TokenType& t) {
  bool matched = expect(t);
  scan();
  return matched;
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
// TODO: Error handle better; currently just parse and pray
// Then again, if you can't type a program header properly, why should I even
// try to parse your program?
std::shared_ptr<IdToken> Parser::programHeader() {
  LOG(DEBUG) << "<program_header>";
  expectScan(TOK_RW_PROG);
  std::shared_ptr<IdToken> id_tok = identifier(false);
  expectScan(TOK_RW_IS);
  return id_tok;
}

//  <program_body> ::=
//      <declarations>
//    `begin'
//      <statements>
//    `end' `program'
std::unique_ptr<ast::ProgramBody> Parser::programBody() {
  LOG(DEBUG) << "<program_body>";
  std::list<std::unique_ptr<ast::Node>> decl = declarations(true);
  LOG(DEBUG) << "Done parsing global declarations";
  LOG(DEBUG) << "Global symbol table:\n" << env->getGlobalStr();
  expectScan(TOK_RW_BEG);
  std::list<std::unique_ptr<ast::Node>> stmt = statements();
  expectScan(TOK_RW_END);
  expectScan(TOK_RW_PROG);
  return std::make_unique<ast::ProgramBody>(std::move(decl), std::move(stmt));
}

//  <declarations> ::=
//    (<declaration>`;')*
std::list<std::unique_ptr<ast::Node>> Parser::declarations(bool is_global) {
  LOG(DEBUG) << "<declarations>";

  // FIRST(<declaration>) = {global, procedure, variable}
  std::list<std::unique_ptr<ast::Node>> decl_list;
  while (match(TOK_RW_GLOB) || match(TOK_RW_PROC)
      || match(TOK_RW_VAR)) {  // TODO: !match(FOLLOW(<declaration>) instead??
      // FOLLOW(<declaration>) is always TOK_RW_BEG
    auto decl = declaration(is_global);
    if (decl == nullptr || !expectScan(TOK_SEMICOL)) {
      panic();  // TODO: Do I want to do something smarter?
      // TODO: Add TOK_RW_BEG to sync point
    } else {
      decl_list.push_back(std::move(decl));
    }
    expectScan(TOK_SEMICOL);
  }
  return decl_list;
}

//  <statements> ::=
//    (<statement>`;')*
std::list<std::unique_ptr<ast::Node>> Parser::statements() {
  LOG(DEBUG) << "<statements>";

  // FIRST(<statement>) = {<identifier>, if, for, return}
  std::list<std::unique_ptr<ast::Node>> stmt_list;
  while(match(TOK_IDENT) || match(TOK_RW_IF) || match(TOK_RW_FOR)
      || match(TOK_RW_RET)) {  // TODO: !match(FOLLOW(<statement>) instead??
      // FOLLOW(<statement>) is always TOK_RW_END
    panic_mode = false;  // Reset panic mode
    auto stmt = statement();
    if (stmt == nullptr || !expectScan(TOK_SEMICOL)) {
      panic();  // TODO: Again, something smarter?
      // TODO: Add TOK_RW_END to sync point
    } else {
      stmt_list.push_back(std::move(stmt));
    }
  }
  return stmt_list;
}

//  <declaration> ::=
//    [`global'] <procedure_declaration>
//  | [`global'] <variable_declaration>
std::unique_ptr<ast::Node> Parser::declaration(bool is_global) {
  LOG(DEBUG) << "<declaration>";

  // Detect the global keyword separately to aid the recursion
  bool is_global_kw = false;
  std::unique_ptr<ast::Node> decl = nullptr;
  if (match(TOK_RW_GLOB)) {
    is_global_kw = true;
    scan();
  }
  if (match(TOK_RW_PROC)) {
    decl = procedureDeclaration(is_global || is_global_kw);
  } else if(match(TOK_RW_VAR)) {
    decl = variableDeclaration(is_global || is_global_kw);
  } else {
    LOG(ERROR) << "Unexpected token: " << tok->getStr();
    LOG(ERROR) << "Expected: " << Token::getTokenName(TOK_RW_PROC) << " or "
        << Token::getTokenName(TOK_RW_VAR);
  }
  return decl;
}

//  <procedure_declaration> ::=
//    <procedure_header> <procedure_body>
std::unique_ptr<ast::ProcedureDeclaration>
Parser::procedureDeclaration(const bool& is_global) {
  LOG(DEBUG) << "<procedure_declaration>";
  std::unique_ptr<ast::ProcedureHeader> proc_hdr = procedureHeader(is_global);
  std::unique_ptr<ast::ProcedureBody> proc_body = procedureBody();
  popScope();

  // Verify parse was okay
  if (proc_hdr == nullptr || proc_body == nullptr) {
    LOG(ERROR) << "Failed to parse procedure";
    return nullptr;
  }
}

//  <procedure_header>
//    `procedure' <identifier> `:' <type_mark> `('[<parameter_list>]`)'
std::unique_ptr<ast::ProcedureHeader>
Parser::procedureHeader(const bool& is_global) {
  LOG(DEBUG) << "<procedure_header>";
  expectScan(TOK_RW_PROC);
  std::shared_ptr<IdToken> id_tok = identifier(false);
  env->insert(id_tok->getVal(), id_tok, is_global);
  expectScan(TOK_COLON);
  TypeMark tm = typeMark();
  id_tok->setTypeMark(tm);
  id_tok->setProcedure(true);

  // Begin new scope
  pushScope(id_tok);  // This adds id_tok to the new scope for recursion
  expectScan(TOK_LPAREN);
  if (match(TOK_RW_VAR)) {
    parameterList();
  }
  expectScan(TOK_RPAREN);
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

  if (match(TOK_COMMA)) {
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
  expectScan(TOK_RW_BEG);
  statements();
  expectScan(TOK_RW_END);
  expectScan(TOK_RW_PROC);
}

//  <variable_declaration> ::=
//    `variable' <identifier> `:' <type_mark> [`['<bound>`]']
std::unique_ptr<ast::VariableDeclaration>
Parser::variableDeclaration(const bool& is_global) {
  LOG(DEBUG) << "<variable_declaration>";

  // Making this in case panic mode happens before the call to identifier()
  // I don't want to return a nullptr
  std::shared_ptr<IdToken> id_tok(new IdToken(TOK_INVALID, ""));
  expectScan(TOK_RW_VAR);
  id_tok = identifier(false);
  env->insert(id_tok->getVal(), id_tok, is_global);
  expectScan(TOK_COLON);
  TypeMark tm = typeMark();
  id_tok->setTypeMark(tm);
  id_tok->setProcedure(false);
  if (match(TOK_LBRACK)) {
    LOG(DEBUG) << "Variable is an array";
    scan();
    id_tok->setNumElements(bound());
    expectScan(TOK_RBRACK);
  }
  LOG(DEBUG) << "Declared variable " << id_tok->getStr();
  return id_tok;
}

//  <type_mark> ::=
//    `integer' | `float' | `string' | `bool'
TypeMark Parser::typeMark() {
  LOG(DEBUG) << "<type_mark>";
  TypeMark tm = TYPE_NONE;
  if (match(TOK_RW_INT)) {
    tm = TYPE_INT;
  }
  else if (match(TOK_RW_FLT)) {
    tm = TYPE_FLT;
  }
  else if (match(TOK_RW_STR)) {
    tm = TYPE_STR;
  }
  else if (match(TOK_RW_BOOL)) {
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
  if (match(TOK_IDENT)) {
    assignmentStatement();
  } else if (match(TOK_RW_IF)) {
    ifStatement();
  } else if (match(TOK_RW_FOR)) {
    loopStatement();
  } else if (match(TOK_RW_RET)) {
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
  expectScan(TOK_LPAREN);
  if (!match(TOK_RPAREN)) {
    argumentList(0, id_tok);
  }
  expectScan(TOK_RPAREN);
  return id_tok->getTypeMark();
}

//  <assignment_statement> ::=
//    <destination> `:=' <expression>
void Parser::assignmentStatement() {
  LOG(DEBUG) << "<assignment_statement>";
  int dest_size = 0;
  TypeMark tm_dest = destination(dest_size);
  expect(TOK_OP_ASS);
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int expr_size = 0;
  TypeMark tm_expr = expression(expr_size);
  type_checker.checkCompatible(op_tok, tm_dest, tm_expr);
  type_checker.checkArraySize(op_tok, dest_size, expr_size);
}

//  <destination> ::=
//    <identifier>[`['<expression>`]']
TypeMark Parser::destination(int& size) {
  LOG(DEBUG) << "<destination>";
  expect(TOK_IDENT);
  if (panic_mode) return TYPE_NONE;  // No need to continue
  std::shared_ptr<IdToken> id_tok = identifier(true);
  if (id_tok->getProcedure()) {
    LOG(ERROR) << "Expected variable; got procedure " << id_tok->getVal();
  }
  TypeMark tm = id_tok->getTypeMark();
  size = id_tok->getNumElements();
  if (match(TOK_LBRACK)) {
    LOG(DEBUG) << "Indexing array";
    size = 0;  // If indexing, it's a single element not an array
    if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
      LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
    }
    scan();
    int idx_size = 0;
    TypeMark tm_idx = expression(idx_size);
    type_checker.checkArrayIndex(tm_idx);
    if (idx_size > 0) {
      LOG(ERROR) << "Invalid index; Expected scalar, got array";
    }
    expectScan(TOK_RBRACK);
  }
  return tm;
}

//  <if_statement> ::=
//    `if' `(' <expression> `)' `then' <statements>
//    [`else' <statements>]
//    `end' `if'
void Parser::ifStatement() {
  LOG(DEBUG) << "<if_statement>";
  expectScan(TOK_RW_IF);
  expectScan(TOK_LPAREN);

  // Ensure expression parses to `bool'
  int expr_size = 0;
  TypeMark tm = expression(expr_size);
  if (!type_checker.checkCompatible(tm, TYPE_BOOL)) {
    LOG(ERROR) << "Invalid if statement expression of type "
      << Token::getTypeMarkName(tm) << " received";
    LOG(ERROR) << "If statement expression must resolve to type "
        << Token::getTypeMarkName(TYPE_BOOL);
  } else if (expr_size > 0) {
    LOG(ERROR) << "Invalid if statement; expected scalar, got array";
  }
  expectScan(TOK_RPAREN);
  expectScan(TOK_RW_THEN);
  statements();
  if (match(TOK_RW_ELSE)) {
    LOG(DEBUG) << "Else";
    scan();
    statements();
  }
  expectScan(TOK_RW_END);
  expectScan(TOK_RW_IF);
}

//  <loop_statement> ::=
//    `for' `(' <assignment_statement>`;' <expression> `)'
//      <statements>
//    `end' `for'
void Parser::loopStatement() {
  LOG(DEBUG) << "<loop_statement>";
  expectScan(TOK_RW_FOR);
  expectScan(TOK_LPAREN);
  assignmentStatement();
  expectScan(TOK_SEMICOL);

  // Ensure expression parses to `bool'
  int expr_size = 0;
  TypeMark tm = expression(expr_size);
  if (!type_checker.checkCompatible(tm, TYPE_BOOL)) {
    LOG(ERROR) << "Invalid loop statement expression of type "
      << Token::getTypeMarkName(tm) << " received";
    LOG(ERROR) << "Loop statement expression must resolve to type "
        << Token::getTypeMarkName(TYPE_BOOL);
  } else if (expr_size > 0) {
    LOG(ERROR) << "Invalid loop statement; expected scalar, got array";
  }
  expectScan(TOK_RPAREN);
  statements();
  expectScan(TOK_RW_END);
  expectScan(TOK_RW_FOR);
}

//  <return_statement> ::=
//    `return' <expression>
void Parser::returnStatement() {
  LOG(DEBUG) << "<return_statement>";
  expectScan(TOK_RW_RET);

  // Make sure <expression> type matches return type for this function
  int expr_size = 0;
  TypeMark tm_expr = expression(expr_size);
  TypeMark tm_ret = function_stack.top()->getTypeMark();
  if (!type_checker.checkCompatible(tm_expr, tm_ret)) {
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
  std::shared_ptr<IdToken> id_tok = nullptr;
  if (expect(TOK_IDENT)) {
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
  bool bitwise_not = match(TOK_RW_NOT);
  std::shared_ptr<Token> op_tok;
  if (bitwise_not) {
    LOG(DEBUG) << "Bitwise not";
    op_tok = std::shared_ptr<Token>(new Token(TOK_OP_EXPR, "not"));
    scan();
  }
  TypeMark tm_arith = arithOp(size);

  // Check type compatibility for bitwise not
  if (bitwise_not) {
    type_checker.checkCompatible(op_tok, tm_arith);
    type_checker.checkArraySize(op_tok, size);
  }
  return expressionPrime(tm_arith, size);
}

//  <expression_prime> ::=
//    `&' <arith_op> <expression_prime>
//  | `|' <arith_op> <expression_prime>
//  | epsilon
TypeMark Parser::expressionPrime(const TypeMark& tm, int& size) {
  LOG(DEBUG) << "<expression_prime>";
  if (!match(TOK_OP_EXPR)) {
    LOG(DEBUG) << "epsilon";
  } else {
    std::shared_ptr<Token> op_tok = tok;
    scan();
    int arith_size = 0;
    TypeMark tm_arith = arithOp(arith_size);
    type_checker.checkCompatible(op_tok, tm, tm_arith);
    type_checker.checkArraySize(op_tok, size, arith_size);
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
  if (!match(TOK_OP_ARITH)) {
    LOG(DEBUG) << "epsilon";
    return tm;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int relat_size = 0;
  TypeMark tm_relat = relation(relat_size);
  type_checker.checkCompatible(op_tok, tm, tm_relat);
  type_checker.checkArraySize(op_tok, size, relat_size);

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
  if (!match(TOK_OP_RELAT)) {
    LOG(DEBUG) << "epsilon";
    return tm;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int term_size = 0;
  TypeMark tm_term = term(term_size);
  type_checker.checkCompatible(op_tok, tm, tm_term);
  type_checker.checkArraySize(op_tok, size, term_size);
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
  if (!match(TOK_OP_TERM)) {
    LOG(DEBUG) << "epsilon";
    return tm;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();
  int fact_size = 0;
  TypeMark tm_fact = factor(fact_size);
  type_checker.checkCompatible(op_tok, tm, tm_fact);
  type_checker.checkArraySize(op_tok, size, fact_size);

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
  if (match(TOK_OP_ARITH) && (tok->getVal() == "-")) {
    scan();
    if (match(TOK_IDENT)) {
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
    } else if (match(TOK_NUM)) {
      std::shared_ptr<Token> num_tok = number();
      tm = num_tok->getTypeMark();
      size = 0;  // <number> literals are scalar
    } else {
      LOG(ERROR) << "Minus sign must be followed by <name> or <number>.";
      LOG(ERROR) << "Got: " << tok->getStr();
    }

  // `('<expression>`)'
  } else if (match(TOK_LPAREN)) {
    scan();
    tm = expression(size);
    expectScan(TOK_RPAREN);

  // <procedure_call> or <name>
  } else if (match(TOK_IDENT)) {
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
  } else if (match(TOK_NUM)) {
    std::shared_ptr<Token> num_tok = number();
    tm = num_tok->getTypeMark();
    size = 0;  // <number> literals are scalars

  // <string>
  } else if (match(TOK_STR)) {
    std::shared_ptr<LiteralToken<std::string>> str_tok = string();
    tm = str_tok->getTypeMark();
    size = 0;  // <string> literals are scalars

  // `true'
  } else if (match(TOK_RW_TRUE)) {
    tm = TYPE_BOOL;
    size = 0;  // `true' literals are scalars
    scan();

  // `false'
  } else if (match(TOK_RW_FALSE)) {
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
  if (match(TOK_LBRACK)) {
    LOG(DEBUG) << "Indexing array";
    size = 0;  // If indexing, it's a single element not an array
    if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
      LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
    }
    scan();
    int idx_size = 0;
    TypeMark tm_idx = expression(idx_size);
    type_checker.checkArrayIndex(tm_idx);
    if (idx_size > 0) {
      LOG(ERROR) << "Invalid index; expected scalar, got array";
    }
    expectScan(TOK_RBRACK);
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
  } else if (!type_checker.checkCompatible(param->getTypeMark(), tm_arg)) {
    LOG(ERROR) << "Expected parameter with type "
        << Token::getTypeMarkName(param->getTypeMark()) << "; got "
        << Token::getTypeMarkName(tm_arg);
  } else if (expr_size != param->getNumElements()) {
    LOG(ERROR) << "Size of argument (" << expr_size
        << ") != size of parameter (" << param->getNumElements() << ")";
  }
  if (match(TOK_COMMA)) {
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
  if (expect(TOK_NUM)) {
    num_tok = tok;
  }
  if (!panic_mode) scan();
  return num_tok;
}

//  <string> ::=
//    `"'[^"]*`"'
std::unique_ptr<ast::Literal<std::string>> Parser::string() {

  // No expect() because we match() before entering from factor()
  LOG(DEBUG) << "<string>";
  std::unique_ptr<ast::Literal<std::string>> str_node = nullptr;
  auto str_tok = std::dynamic_pointer_cast<LiteralToken<std::string>>(tok);
  scan();
  if (str_tok == nullptr) {  // Cast should never fail but...
    LOG(ERROR) << "Failed to parse string";
  } else {
    str_node = std::make_unique<ast::Literal<std::string>>(str_tok->getVal(),
        str_tok->getTypeMark());
  }
  return str_node;
}
