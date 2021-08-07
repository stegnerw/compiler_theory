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

Parser::Parser() : env(new Environment()), scanner(env) {}

bool Parser::init(const std::string& src_file) {
  bool init_success = true;
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
  auto prog_hdr = programHeader();
  auto prog_body = programBody();
  auto prog = std::make_unique<ast::Program>(std::move(prog_hdr),
      std::move(prog_body));
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
  return std::make_unique<ast::ProcedureDeclaration>(std::move(proc_hdr), 
      std::move(proc_body));
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
  std::list<std::unique_ptr<ast::VariableDeclaration>> param_list;
  if (match(TOK_RW_VAR)) {
    param_list = parameterList(std::move(param_list));
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
std::list<std::unique_ptr<ast::VariableDeclaration>>
Parser::parameterList(std::list<std::unique_ptr<ast::VariableDeclaration>>
    param_list) {  // Holy long declaration, Batman!
  LOG(DEBUG) << "<parameter_list>";

  // Parse parameter declaration
  std::unique_ptr<ast::VariableDeclaration> par_tok = parameter();
  if (par_tok == nullptr) {
    LOG(ERROR) << "Ill-formed parameter; skipping";
    return param_list;
  }
  function_stack.top()->addParam(par_tok->getIdTok());
  param_list.push_back(std::move(par_tok));

  // Get next argument
  if (match(TOK_COMMA)) {
    scan();
    return parameterList(std::move(param_list));
  }

  // Last argument was parsed
  return param_list;
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
  return std::make_unique<ast::VariableDeclaration>(tm, is_global, id_tok,
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
  std::list<std::unique_ptr<ast::Node>> arg_list;
  if (!match(TOK_RPAREN)) {
    arg_list = argumentList(std::move(arg_list));
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
  return std::make_unique<ast::ReturnStatement>(
      function_stack.top()->getTypeMark(), expression());
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

  // Parse LHS arithOp
  std::unique_ptr<ast::Node> lhs = arithOp();
  if (lhs == nullptr) {
    LOG(ERROR) << "Failed to parse lhs arith op";
    return nullptr;
  }

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
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();

  // Parse RHS arithOp
  std::unique_ptr<ast::Node> rhs = arithOp();
  if (rhs == nullptr) {
    LOG(ERROR) << "Failed to parse rhs arith op";
    return nullptr;
  }
  return expressionPrime(std::make_unique<ast::BinaryOp>(std::move(lhs),
        std::move(rhs), op_tok));
}

//  <arith_op> ::=
//    <relation> <arith_op_prime>
std::unique_ptr<ast::Node> Parser::arithOp() {
  LOG(DEBUG) << "<arith_op>";

  // Parse LHS relation
  std::unique_ptr<ast::Node> lhs = relation();
  if (lhs == nullptr) {
    LOG(ERROR) << "Failed to parse lhs relation";
    return nullptr;
  }
  return arithOpPrime(std::move(lhs));
}

//  <arith_op_prime> ::=
//    `+' <relation> <arith_op_prime>
//  | `-' <relation> <arith_op_prime>
//  | epsilon
std::unique_ptr<ast::Node>
Parser::arithOpPrime(std::unique_ptr<ast::Node> lhs) {
  LOG(DEBUG) << "<arith_op_prime>";
  if (!match(TOK_OP_ARITH)) {
    LOG(DEBUG) << "epsilon";
    return lhs;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();

  // Parse RHS relation
  std::unique_ptr<ast::Node> rhs = relation();
  if (rhs == nullptr) {
    LOG(ERROR) << "Failed to parse rhs relation";
    return nullptr;
  }

  return arithOpPrime(std::make_unique<ast::BinaryOp>(std::move(lhs),
        std::move(rhs), op_tok));
}

//  <relation> ::=
//    <term> <relation_prime>
std::unique_ptr<ast::Node> Parser::relation() {
  LOG(DEBUG) << "<relation>";

  // Parse lhs term
  std::unique_ptr<ast::Node> lhs = term();
  if (lhs == nullptr) {
    LOG(ERROR) << "Failed to parse lhs term";
    return nullptr;
  }
  return relationPrime(std::move(lhs));
}

//  <relation_prime> ::=
//    `<' <term> <relation_prime>
//  | `>=' <term> <relation_prime>
//  | `<=' <term> <relation_prime>
//  | `>' <term> <relation_prime>
//  | `==' <term> <relation_prime>
//  | `!=' <term> <relation_prime>
//  | epsilon
std::unique_ptr<ast::Node>
Parser::relationPrime(std::unique_ptr<ast::Node> lhs) {
  LOG(DEBUG) << "<relation_prime>";
  if (!match(TOK_OP_RELAT)) {
    LOG(DEBUG) << "epsilon";
    return lhs;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();

  // Parse RHS term
  std::unique_ptr<ast::Node> rhs = term();
  if (rhs == nullptr) {
    LOG(ERROR) << "Failed to parse rhs term";
    return nullptr;
  }
  return relationPrime(std::make_unique<ast::BinaryOp>(std::move(lhs),
        std::move(rhs), op_tok));
}

//  <term> ::=
//    <factor> <term_prime>
std::unique_ptr<ast::Node> Parser::term() {
  LOG(DEBUG) << "<term>";

  // Parse lhs factor
  std::unique_ptr<ast::Node> lhs = factor();
  if (lhs == nullptr) {
    LOG(DEBUG) << "Failed to parse lhs factor";
    return nullptr;
  }
  return termPrime(std::move(lhs));
}

//  <term_prime> ::=
//    `*' <factor> <term_prime>
//  | `/' <factor> <term_prime>
//  | epsilon
std::unique_ptr<ast::Node> Parser::termPrime(std::unique_ptr<ast::Node> lhs) {
  LOG(DEBUG) << "<term_prime>";
  if (!match(TOK_OP_TERM)) {
    LOG(DEBUG) << "epsilon";
    return lhs;
  }
  std::shared_ptr<Token> op_tok = tok;
  scan();

  // Parse RHS factor
  std::unique_ptr<ast::Node> rhs = factor();
  if (rhs == nullptr) {
    LOG(DEBUG) << "Failed to parse rhs factor";
    return nullptr;
  }
  return termPrime(std::make_unique<ast::BinaryOp>(std::move(lhs),
        std::move(rhs), op_tok));
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
  std::unique_ptr<ast::Node> node;

  // Negative sign can only happen before <number> and <name>
  if (match(TOK_OP_ARITH) && (tok->getVal() == "-")) {
    std::shared_ptr<Token> op_tok = tok;
    scan();
    if (match(TOK_IDENT)) {
      std::shared_ptr<IdToken> id_tok = std::dynamic_pointer_cast<IdToken>(
          env->lookup(tok->getVal(), false));
      if (id_tok == nullptr) {
        LOG(ERROR) << "Identifier not in this scope: " << tok->getStr();
      } else if (!id_tok->getProcedure()) {
        node = name();
      } else {
        LOG(ERROR) << "Expected variable; got: " << tok->getStr();
      }
    } else if (match(TOK_NUM)) {
      node = number();
    } else {
      LOG(ERROR) << "Minus sign must be followed by <name> or <number>.";
      LOG(ERROR) << "Got: " << tok->getStr();
    }

    // Assembly negative unary op node
    if (node != nullptr) {
      node = std::make_unique<ast::UnaryOp>(std::move(node), op_tok);
    }

  // `('<expression>`)'
  } else if (match(TOK_LPAREN)) {
    scan();
    node = expression();
    expectScan(TOK_RPAREN);

  // <procedure_call> or <name>
  } else if (match(TOK_IDENT)) {
    std::shared_ptr<IdToken> id_tok = std::dynamic_pointer_cast<IdToken>(
        env->lookup(tok->getVal(), false));
    if (!id_tok) {
      LOG(ERROR) << "Identifier not declared in this scope: "
          << tok->getStr();
    } else if (id_tok->getProcedure()) {
      node = procedureCall();
    } else {
      node = name();
    }

  // <number>
  } else if (match(TOK_NUM)) {
    node = number();

  // <string>
  } else if (match(TOK_STR)) {
    node = string();

  // `true'
  } else if (match(TOK_RW_TRUE)) {
    node = std::make_unique<ast::Literal<bool>>(TYPE_BOOL, true);
    scan();

  // `false'
  } else if (match(TOK_RW_FALSE)) {
    node = std::make_unique<ast::Literal<bool>>(TYPE_BOOL, false);
    scan();

  // Oof
  } else {
    LOG(ERROR) << "Unexpected token: " << tok->getStr();
  }
  return node;
}

//  <name> ::=
//    <identifier> [`['<expression>`]']
std::unique_ptr<ast::VariableReference> Parser::name() {
  LOG(DEBUG) << "<name>";
  return destination();  // This is the same as <destination>
}

//  <argument_list> ::=
//    <expression> `,' <argument_list>
//  | <expression>
std::list<std::unique_ptr<ast::Node>>
Parser::argumentList(std::list<std::unique_ptr<ast::Node>> arg_list) {
  LOG(DEBUG) << "<argument_list>";

  // Parse argument expression
  std::unique_ptr<ast::Node> expr = expression();
  if (expr == nullptr) {
    LOG(ERROR) << "Failed to parse expression";
    return arg_list;
  }
  arg_list.push_back(std::move(expr));

  // Get next argument
  if (match(TOK_COMMA)) {
    scan();
    return argumentList(std::move(arg_list));
  }

  // Last argument was parsed
  return arg_list;
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
    str_node = std::make_unique<ast::Literal<std::string>>(
        str_tok->getTypeMark(), str_tok->getVal());
  }
  return str_node;
}
