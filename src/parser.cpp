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
  // Scan only if it matched
  if (matched) scan();
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
      std::unique_ptr<ast::Node> decl = declaration(is_global);
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
    std::unique_ptr<ast::Node> stmt = statement();
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
  std::unique_ptr<ast::Node> decl = nullptr;
  if (match(TOK_RW_GLOB)) {
    is_global = true;
    scan();
  }
  if (match(TOK_RW_PROC)) {
    decl = procedureDeclaration(is_global);
  } else if(match(TOK_RW_VAR)) {
    decl = variableDeclaration(is_global);
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

  if (proc_hdr == nullptr || proc_body == nullptr) {
    LOG(ERROR) << "Failed to parse procedure";
    return nullptr;
  }
  return std::make_unique<ast::ProcedureDeclaration>(proc_hdr, proc_body);
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
  std::unique_ptr<ast::ParameterList> param_list = nullptr;
  if (match(TOK_RW_VAR)) {
    param_list = parameterList();
  }
  expectScan(TOK_RPAREN);

  if (id_tok == nullptr) {  // It's okay for param_list to be nullptr
    LOG(ERROR) << "Failed to parse procedure header";
    return nullptr;
  }
  return std::make_unique<ast::ProcedureHeader>(tm, is_global, id_tok,
      std::move(param_list));
}

//  <parameter_list> ::=
//    <parameter>`,' <parameter_list>
//  | <parameter>
// TODO: This could be a little cleaner if I have time...
std::unique_ptr<ast::ParameterList> Parser::parameterList() {
  LOG(DEBUG) << "<parameter_list>";
  std::unique_ptr<ast::VariableDeclaration> par_tok = parameter();
  if (par_tok == nullptr) {
    LOG(ERROR) << "Ill-formed parameter; skipping";
    return nullptr;
  }
  if (match(TOK_COMMA)) {
    scan();
    return std::make_unique<ast::ParameterList>(par_tok, parameterList());
  }
  return std::make_unique<ast::ParameterList>(par_tok, nullptr);
}

//  <parameter> ::=
//    <variable_declaration>
std::unique_ptr<ast::VariableDeclaration> Parser::parameter() {
  LOG(DEBUG) << "<parameter>";
  return variableDeclaration(false);
}

//  <procedure_body> ::=
//      <statements>
//    `begin'
//      <statements>
//    `end' `procedure'
std::unique_ptr<ast::ProcedureBody> Parser::procedureBody() {
  LOG(DEBUG) << "<procedure_body>";
  std::list<std::unique_ptr<ast::Node>> decl_list = declarations(false);
  if (!expectScan(TOK_RW_BEG)) {
    return nullptr;
  }
  std::list<std::unique_ptr<ast::Node>> stmt_list = statements();
  if (!expectScan(TOK_RW_END) || !expectScan(TOK_RW_PROC)) {
    return nullptr;
  }
  return std::make_unique<ast::ProcedureBody>(std::move(decl_list),
      std::move(stmt_list));
}

//  <variable_declaration> ::=
//    `variable' <identifier> `:' <type_mark> [`['<bound>`]']
std::unique_ptr<ast::VariableDeclaration>
Parser::variableDeclaration(const bool& is_global) {
  LOG(DEBUG) << "<variable_declaration>";
  expectScan(TOK_RW_VAR);
  std::shared_ptr<IdToken> id_tok = identifier(false);
  if (id_tok == nullptr) {
    LOG(ERROR) << "Invalid identifier; skipping variable declaration";
    return nullptr;
  }
  env->insert(id_tok->getVal(), id_tok, is_global);
  expectScan(TOK_COLON);
  TypeMark tm = typeMark();
  id_tok->setTypeMark(tm);
  id_tok->setProcedure(false);

  // Array shenanigans
  std::unique_ptr<ast::Literal<float>> bound_node = nullptr;
  if (match(TOK_LBRACK)) {
    LOG(DEBUG) << "Variable is an array";
    scan();
    bound_node = bound();
    expectScan(TOK_RBRACK);
  }
  LOG(DEBUG) << "Declared variable " << id_tok->getStr();
  return std::make_unique<ast::VariableDeclaration>(tm, id_tok,
      std::move(bound_node));
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
std::unique_ptr<ast::Literal<float>> Parser::bound() {
  LOG(DEBUG) << "<bound>";
  std::unique_ptr<ast::Literal<float>> bound_node = number();
  if (bound_node == nullptr) {
    LOG(ERROR) << "Failed to parse bound";
    return nullptr;
  }
  return bound_node;
}

//  <statement> ::=
//    <assignment_statement>
//  | <if_statement>
//  | <loop_statement>
//  | <return_statement>
std::unique_ptr<ast::Node> Parser::statement() {
  LOG(DEBUG) << "<statement>";
  std::unique_ptr<ast::Node> stmt = nullptr;
  if (match(TOK_IDENT)) {
    stmt = assignmentStatement();
  } else if (match(TOK_RW_IF)) {
    stmt = ifStatement();
  } else if (match(TOK_RW_FOR)) {
    stmt = loopStatement();
  } else if (match(TOK_RW_RET)) {
    stmt = returnStatement();
  } else {
    LOG(ERROR) << "Unexpected token: " << tok->getVal()
        << "; expected statement";
  }
  return stmt;
}

//  <procedure_call> ::=
//    <identifier>`('[<argument_list>]`)'
std::unique_ptr<ast::ProcedureCall> Parser::procedureCall() {
  LOG(DEBUG) << "<procedure_call>";
  std::shared_ptr<IdToken> id_tok = identifier(true);
  if (id_tok == nullptr || !id_tok->getProcedure()) {
    LOG(ERROR) << "Failed to parse procedure call identifier";
    return nullptr;
  }
  if (!expectScan(TOK_LPAREN)) {
    LOG(WARN) << "Skipping procedure call";
    return nullptr;
  }
  std::unique_ptr<ast::ArgumentList> arg_list = nullptr;
  if (!match(TOK_RPAREN)) {
    arg_list = argumentList(id_tok);
  }
  if (!expectScan(TOK_RPAREN)) {
    LOG(WARN) << "Skipping procedure call";
    return nullptr;
  }
  return std::make_unique<ast::ProcedureCall>(id_tok, std::move(arg_list));
}

//  <assignment_statement> ::=
//    <destination> `:=' <expression>
std::unique_ptr<ast::AssignmentStatement> Parser::assignmentStatement() {
  LOG(DEBUG) << "<assignment_statement>";
  std::unique_ptr<ast::VariableReference> dest = destination();
  std::shared_ptr<Token> op_tok = tok;
  if (!expectScan(TOK_OP_ASS)) {
    LOG(WARN) << "Skipping assignment statement";
    return nullptr;
  }
  std::unique_ptr<ast::Node> expr = expression();

  // Check valid parse
  if (dest == nullptr || expr == nullptr) {
    LOG(ERROR) << "Could not parse assignment statement";
    LOG(WARN) << "Skipping assignment statement";
    return nullptr;
  }
  return std::make_unique<ast::AssignmentStatement>(std::move(dest),
      std::move(expr));
}

//  <destination> ::=
//    <identifier>[`['<expression>`]']
std::unique_ptr<ast::VariableReference> Parser::destination() {
  LOG(DEBUG) << "<destination>";
  if (!expect(TOK_IDENT)) {
    LOG(ERROR) << "Identifier not found; skipping destination";
    return nullptr;
  }
  std::shared_ptr<IdToken> id_tok = identifier(true);
  if (id_tok == nullptr || id_tok->getProcedure()) {
    LOG(ERROR) << "Invalid identifier; skipping destination";
    return nullptr;
  }

  // Array shenanigans
  std::unique_ptr<ast::Node> expr = nullptr;
  if (match(TOK_LBRACK)) {
    LOG(DEBUG) << "Indexing array";
    if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
      LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
    }
    scan();
    expr = expression();
    expectScan(TOK_RBRACK);
  }
  return std::make_unique<ast::VariableReference>(id_tok, std::move(expr));
}

//  <if_statement> ::=
//    `if' `(' <expression> `)' `then' <statements>
//    [`else' <statements>]
//    `end' `if'
std::unique_ptr<ast::IfStatement> Parser::ifStatement() {
  LOG(DEBUG) << "<if_statement>";
  if (!expectScan(TOK_RW_IF) || !expectScan(TOK_LPAREN)) {
    LOG(ERROR) << "Skipping if statement";
    return nullptr;
  }
  std::unique_ptr<ast::Node> expr = expression();
  if (expr == nullptr || !expectScan(TOK_RPAREN) || !expectScan(TOK_RW_THEN)) {
    LOG(ERROR) << "Skipping if statement";
    return nullptr;
  }
  std::list<std::unique_ptr<ast::Node>> then_stmt_list = statements();
  std::list<std::unique_ptr<ast::Node>> else_stmt_list;
  if (match(TOK_RW_ELSE)) {
    LOG(DEBUG) << "Else";
    scan();
    else_stmt_list = statements();
  }
  expectScan(TOK_RW_END);
  expectScan(TOK_RW_IF);
  return std::make_unique<ast::IfStatement>(std::move(expr),
      std::move(then_stmt_list), std::move(else_stmt_list));
}

//  <loop_statement> ::=
//    `for' `(' <assignment_statement>`;' <expression> `)'
//      <statements>
//    `end' `for'
std::unique_ptr<ast::LoopStatement> Parser::loopStatement() {
  LOG(DEBUG) << "<loop_statement>";
  if (!expectScan(TOK_RW_FOR) || !expectScan(TOK_LPAREN)) {
    LOG(ERROR) << "Skipping loop statement";
    return nullptr;
  }
  std::unique_ptr<ast::AssignmentStatement> assign = assignmentStatement();
  if (assign == nullptr || !expectScan(TOK_SEMICOL)) {
    LOG(ERROR) << "Skipping loop statement";
    return nullptr;
  }

  std::unique_ptr<ast::Node> expr = expression();
  if (expr == nullptr || !expectScan(TOK_RPAREN)) {
    LOG(ERROR) << "Skipping loop statement";
    return nullptr;
  }
  std::list<std::unique_ptr<ast::Node>> stmt_list = statements();
  expectScan(TOK_RW_END);
  expectScan(TOK_RW_FOR);
  return std::make_unique<ast::LoopStatement>(std::move(assign),
      std::move(expr), std::move(stmt_list));
}

//  <return_statement> ::=
//    `return' <expression>
std::unique_ptr<ast::ReturnStatement> Parser::returnStatement() {
  LOG(DEBUG) << "<return_statement>";
  if (!expectScan(TOK_RW_RET)) {
    LOG(ERROR) << "Skipping return statement";
    return nullptr;
  }
  return std::make_unique<ast::ReturnStatement>(expression());
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
      if (id_tok == nullptr) {
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
std::unique_ptr<ast::Node> Parser::expression() {
  LOG(DEBUG) << "<expression>";
  bool bitwise_not = match(TOK_RW_NOT);
  if (bitwise_not) {
    LOG(DEBUG) << "Bitwise not";
    scan();
  }
  //op_tok = std::make_shared<Token>(TOK_OP_EXPR, "not");
  std::unique_ptr<ast::Node> lhs = arithOp();

  // Add not operator if applicable
  if (bitwise_not) {
    lhs = std::make_unique<ast::UnaryOp>(std::move(lhs),
        std::make_shared<Token>(TOK_OP_EXPR, "not"));
  }

  return expressionPrime(std::move(lhs));
}

//  <expression_prime> ::=
//    `&' <arith_op> <expression_prime>
//  | `|' <arith_op> <expression_prime>
//  | epsilon
std::unique_ptr<ast::Node>
Parser::expressionPrime(std::unique_ptr<ast::Node> lhs) {
  LOG(DEBUG) << "<expression_prime>";
  if (!match(TOK_OP_EXPR)) {
    LOG(DEBUG) << "epsilon";
    return lhs;
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
std::unique_ptr<ast::Node> Parser::arithOp() {
  LOG(DEBUG) << "<arith_op>";
  TypeMark tm_relat = relation(size);

  // tm_relat and tm_arith are checked in arithOpPrime
  return arithOpPrime(tm_relat, size);
}

//  <arith_op_prime> ::=
//    `+' <relation> <arith_op_prime>
//  | `-' <relation> <arith_op_prime>
//  | epsilon
std::unique_ptr<ast::Node> Parser::arithOpPrime(std::unique_ptr<Node> lhs) {
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
std::unique_ptr<ast::Node> Parser::relation() {
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
std::unique_ptr<ast::Node> Parser::relationPrime(std::unique_ptr<Node> lhs) {
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
std::unique_ptr<ast::Node> Parser::term() {
  LOG(DEBUG) << "<term>";
  TypeMark tm_fact = factor(size);
  return termPrime(tm_fact, size);
}

//  <term_prime> ::=
//    `*' <factor> <term_prime>
//  | `/' <factor> <term_prime>
//  | epsilon
std::unique_ptr<ast::Node> Parser::termPrime(std::unique_ptr<Node> lhs) {
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
std::unique_ptr<ast::Node> Parser::factor() {
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
std::unique_ptr<ast::VariableReference> Parser::name(int& size) {
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
std::unique_ptr<ast::ArgumentList>
Parser::argumentList(std::shared_ptr<IdToken> fun_tok) {
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
std::unique_ptr<ast::Literal<float>> Parser::number() {
  LOG(DEBUG) << "<number>";
  std::shared_ptr<Token> num_tok = tok;
  if (!expectScan(TOK_NUM)) {  // No number
    LOG(ERROR) << "Expected number, got "
      << Token::getTokenName(tok->getType());
    return nullptr;
  }

  // Store all literals as float
  auto flt_num_tok = std::dynamic_pointer_cast<LiteralToken<float>>(num_tok);
  if (flt_num_tok == nullptr) {
    LOG(ERROR) << "Failed to cast number token to float";
    return nullptr;
  }
  auto num_node = std::make_unique<ast::Literal<float>>(
      flt_num_tok->getTypeMark(), flt_num_tok->getVal());
  return num_node;
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
