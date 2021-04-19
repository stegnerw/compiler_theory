#include "parser.h"

#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "environment.h"
#include "log.h"
#include "scanner.h"
#include "token.h"
#include "type_checker.h"

Parser::Parser() : env(new Environment()), scanner(env), type_checker() {}

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

//	<program> ::=
//		<program_header> <program_body> `.'
bool Parser::parse() {
	LOG(INFO) << "Begin parsing";
	LOG(DEBUG) << "<program>";
	programHeader();
	programBody();
	expectToken(TOK_PERIOD);
	scan();
	LOG(INFO) << "Done parsing";
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
	return false;
}

//	<program_header> ::=
//		`program' <identifier> `is'
void Parser::programHeader() {
	LOG(DEBUG) << "<program_header>";
	expectToken(TOK_RW_PROG);
	scan();
	identifier(false);
	expectToken(TOK_RW_IS);
	scan();
}

//	<program_body> ::=
//			<declarations>
//		`begin'
//			<statements>
//		`end' `program'
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

//	<declarations> ::=
//		(<declaration>`;')*
void Parser::declarations(bool is_global) {
	LOG(DEBUG) << "<declarations>";
	// FIRST(<declaration>) = {global, procedure, variable}
	while (matchToken(TOK_RW_GLOB) || matchToken(TOK_RW_PROC)
			|| matchToken(TOK_RW_VAR)) {
		declaration(is_global);
		expectToken(TOK_SEMICOL);
		scan();
	}
}

//	<statements> ::=
//		(<statement>`;')*
void Parser::statements() {
	LOG(DEBUG) << "<statements>";
	// FIRST(<statement>) = {<identifier>, if, for, return}
	while(matchToken(TOK_IDENT) || matchToken(TOK_RW_IF) || matchToken(TOK_RW_FOR)
			|| matchToken(TOK_RW_RET)) {
		statement();
		expectToken(TOK_SEMICOL);
		scan();
	}
}

//	<declaration> ::=
//		[`global'] <procedure_declaration>
//	| [`global'] <variable_declaration>
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
		scan();  // TODO: Do I want to scan here? Other ways to handle errors?
		// TODO: This should go to panick mode.
	}
}

//	<procedure_declaration> ::=
//		<procedure_header> <procedure_body>
void Parser::procedureDeclaration(const bool& is_global) {
	LOG(DEBUG) << "<procedure_declaration>";
	procedureHeader(is_global);
	procedureBody();
	pop_scope();
}

//	<procedure_header>
//		`procedure' <identifier> `:' <type_mark> `('[<parameter_list>]`)'
void Parser::procedureHeader(const bool& is_global) {
	LOG(DEBUG) << "<procedure_header>";

	// This should not happen but just in case
	expectToken(TOK_RW_PROC);
	scan();
	std::shared_ptr<IdToken> id_tok = identifier(false);
	env->insert(id_tok->getVal(), id_tok, is_global);
	expectToken(TOK_COLON);
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
}

//	<parameter_list> ::=
//		<parameter>`,' <parameter_list>
//	|	<parameter>
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

//	<parameter> ::=
//		<variable_declaration>
std::shared_ptr<IdToken> Parser::parameter() {
	LOG(DEBUG) << "<parameter>";
	return variableDeclaration(false);
}

//	<procedure_body> ::=
//			<statements>
//		`begin'
//			<statements>
//		`end' `procedure'
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

//	<variable_declaration> ::=
//		`variable' <identifier> `:' <type_mark> [`['<bound>`]']
std::shared_ptr<IdToken> Parser::variableDeclaration(const bool& is_global) {
	LOG(DEBUG) << "<variable_declaration>";
	expectToken(TOK_RW_VAR);
	scan();
	std::shared_ptr<IdToken> id_tok = identifier(false);
	env->insert(id_tok->getVal(), id_tok, is_global);
	expectToken(TOK_COLON);
	scan();
	TypeMark tm = typeMark();
	if (id_tok->isValid()) {
		id_tok->setTypeMark(tm);
		id_tok->setProcedure(false);
	}
	if (matchToken(TOK_LBRACK)) {
		LOG(DEBUG) << "Variable is an array";
		scan();
		id_tok->setNumElements(bound());
		expectToken(TOK_RBRACK);
		scan();
	}
	LOG(DEBUG) << "Declared variable " << id_tok->getStr();
	return id_tok;
}

//	<type_mark> ::=
//		`integer' | `float' | `string' | `bool'
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
	}
	scan();
	return tm;
}

//	<bound> ::=
//		<number>
// TODO: Consolidate exit points of this function for readability
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

//	<statement> ::=
//		<assignment_statement>
//	|	<if_statement>
//	|	<loop_statement>
//	|	<return_statement>
void Parser::statement() {
	LOG(DEBUG) << "<statement>";
	if (matchToken(TOK_IDENT)) {
		assigmentStatement();
	} else if (matchToken(TOK_RW_IF)) {
		ifStatement();
	} else if (matchToken(TOK_RW_FOR)) {
		loopStatement();
	} else if (matchToken(TOK_RW_RET)) {
		returnStatement();
	} else {
		LOG(ERROR) << "Unexpected token: " << tok->getVal()
				<< "; expected statement";
	}
}

//	<procedure_call> ::=
//		<identifier>`('[<argument_list>]`)'
TypeMark Parser::procedureCall() {
	LOG(DEBUG) << "<procedure_call>";
	std::shared_ptr<IdToken> id_tok = identifier(true);
	expectToken(TOK_LPAREN);
	scan();
	if (!matchToken(TOK_RPAREN)) {
		argumentList(0, id_tok);
	}
	expectToken(TOK_RPAREN);
	scan();
	return id_tok->getTypeMark();
}

//	<assignment_statement> ::=
//		<destination> `:=' <expression>
void Parser::assigmentStatement() {
	LOG(DEBUG) << "<assignment_statement>";
	TypeMark tm_dest = destination();
	expectToken(TOK_OP_ASS);
	std::shared_ptr<Token> op_tok = tok;
	scan();
	TypeMark tm_expr = expression();
	type_checker.checkCompatible(op_tok, tm_dest, tm_expr);
}

//	<destination> ::=
//		<identifier>[`['<expression>`]']
// TODO: Array semantics - check if it's the whole array or just an element
TypeMark Parser::destination() {
	LOG(DEBUG) << "<destination>";
	expectToken(TOK_IDENT);
	std::shared_ptr<IdToken> id_tok = identifier(true);
	if (id_tok->getProcedure()) {
		LOG(ERROR) << "Expected variable; got procedure " << id_tok->getVal();
	}
	TypeMark tm = id_tok->getTypeMark();
	int num_elements = id_tok->getNumElements();
	if (matchToken(TOK_LBRACK)) {
		LOG(DEBUG) << "Indexing array";
		if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
			LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
		}
		num_elements = 0;
		scan();
		TypeMark tm_bound = expression();
		type_checker.checkArrayIndex(tm_bound);
		expectToken(TOK_RBRACK);
		scan();
	}
	if (num_elements > 0) {
		LOG(DEBUG) << "Destination is an array";
	} else {
		LOG(DEBUG) << "Destination is a scalar";
	}
	return tm;
}

//	<if_statement> ::=
//		`if' `(' <expression> `)' `then' <statements>
//		[`else' <statements>]
//		`end' `if'
void Parser::ifStatement() {
	LOG(DEBUG) << "<if_statement>";
	expectToken(TOK_RW_IF);
	scan();
	expectToken(TOK_LPAREN);
	scan();

	// Ensure expression parses to `bool'
	TypeMark tm = expression();
	if (!type_checker.checkCompatible(tm, TYPE_BOOL)) {
		LOG(ERROR) << "Invalid if statement expression of type "
			<< Token::getTypeMarkName(tm) << " received";
		LOG(ERROR) << "If statement expression must resolve to type "
				<< Token::getTypeMarkName(TYPE_BOOL);
	}
	expectToken(TOK_RPAREN);
	scan();
	expectToken(TOK_RW_THEN);
	scan();
	statements();
	if (matchToken(TOK_RW_ELSE)) {
		LOG(DEBUG) << "Else";
		scan();
		statements();
	}
	expectToken(TOK_RW_END);
	scan();
	expectToken(TOK_RW_IF);
	scan();
}

//	<loop_statement> ::=
//		`for' `(' <assignment_statement>`;' <expression> `)'
//			<statements>
//		`end' `for'
void Parser::loopStatement() {
	LOG(DEBUG) << "<loop_statement>";
	expectToken(TOK_RW_FOR);
	scan();
	expectToken(TOK_LPAREN);
	scan();
	assigmentStatement();
	expectToken(TOK_SEMICOL);
	scan();

	// Ensure expression parses to `bool'
	TypeMark tm = expression();
	if (!type_checker.checkCompatible(tm, TYPE_BOOL)) {
		LOG(ERROR) << "Invalid loop statement expression of type "
			<< Token::getTypeMarkName(tm) << " received";
		LOG(ERROR) << "Loop statement expression must resolve to type "
				<< Token::getTypeMarkName(TYPE_BOOL);
	}
	expectToken(TOK_RPAREN);
	scan();
	statements();
	expectToken(TOK_RW_END);
	scan();
	expectToken(TOK_RW_FOR);
	scan();
}

//	<return_statement> ::=
//		`return' <expression>
TypeMark Parser::returnStatement() {
	LOG(DEBUG) << "<return_statement>";
	expectToken(TOK_RW_RET);
	scan();

	// Make sure <expression> type matches return type for this function
	TypeMark tm_expr = expression();
	TypeMark tm_ret = function_stack.top()->getTypeMark();
	if (!type_checker.checkCompatible(tm_expr, tm_ret)) {
		LOG(ERROR) << "Expression type " << Token::getTypeMarkName(tm_expr)
				<< " not compatible with return type "
				<< Token::getTypeMarkName(tm_ret);
	}
	return TYPE_NONE;
}

//	<identifier> ::=
//		[a-zA-Z][a-zA-Z0-9_]*
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

//	<expression> ::=
//		[`not'] <arith_op> <expression_prime>
TypeMark Parser::expression() {
	LOG(DEBUG) << "<expression>";
	bool bitwise_not = matchToken(TOK_RW_NOT);
	std::shared_ptr<Token> op_tok;
	if (bitwise_not) {
		LOG(DEBUG) << "Bitwise not";
		op_tok = std::shared_ptr<Token>(new Token(TOK_OP_EXPR, "not"));
		scan();
	}
	TypeMark tm_arith = arithOp();

	// Check type compatibility for bitwise not
	if (bitwise_not) {
		type_checker.checkCompatible(op_tok, tm_arith);
	}
	return expressionPrime(tm_arith);
}

//	<expression_prime> ::=
//		`&' <arith_op> <expression_prime>
//	|	`|' <arith_op> <expression_prime>
//	|	epsilon
TypeMark Parser::expressionPrime(const TypeMark& tm) {
	LOG(DEBUG) << "<expression_prime>";
	if (!matchToken(TOK_OP_EXPR)) {
		LOG(DEBUG) << "epsilon";
	} else {
		std::shared_ptr<Token> op_tok = tok;
		scan();
		TypeMark tm_arith = arithOp();
		type_checker.checkCompatible(op_tok, tm, tm_arith);
		expressionPrime(tm_arith);
	}
	return tm;
}

//	<arith_op> ::=
//		<relation> <arith_op_prime>
TypeMark Parser::arithOp() {
	LOG(DEBUG) << "<arith_op>";
	TypeMark tm_relat = relation();

	// tm_relat and tm_arith are checked in arithOpPrime
	return arithOpPrime(tm_relat);
}

//	<arith_op_prime> ::=
//		`+' <relation> <arith_op_prime>
//	|	`-' <relation> <arith_op_prime>
//	|	epsilon
TypeMark Parser::arithOpPrime(const TypeMark& tm) {
	LOG(DEBUG) << "<arith_op_prime>";
	if (!matchToken(TOK_OP_ARITH)) {
		LOG(DEBUG) << "epsilon";
		return tm;
	}
	std::shared_ptr<Token> op_tok = tok;
	scan();
	TypeMark tm_relat = relation();
	type_checker.checkCompatible(op_tok, tm, tm_relat);

	// If either side is `float', cast to `float'
	TypeMark tm_result;
	if ((tm == TYPE_FLT) || (tm_relat == TYPE_FLT)) {
		tm_result = TYPE_FLT;
	} else {
		tm_result = TYPE_INT;
	}
	return arithOpPrime(tm_result);
}

//	<relation> ::=
//		<term> <relation_prime>
TypeMark Parser::relation() {
	LOG(DEBUG) << "<relation>";
	TypeMark tm_term = term();
	return relationPrime(tm_term);
}

//	<relation_prime> ::=
//		`<' <term> <relation_prime>
//	|	`>=' <term> <relation_prime>
//	|	`<=' <term> <relation_prime>
//	|	`>' <term> <relation_prime>
//	|	`==' <term> <relation_prime>
//	|	`!=' <term> <relation_prime>
//	|	epsilon
TypeMark Parser::relationPrime(const TypeMark& tm) {
	LOG(DEBUG) << "<relation_prime>";
	if (!matchToken(TOK_OP_RELAT)) {
		LOG(DEBUG) << "epsilon";
		return tm;
	}
	std::shared_ptr<Token> op_tok = tok;
	scan();
	TypeMark tm_term = term();
	type_checker.checkCompatible(op_tok, tm, tm_term);
	relationPrime(tm_term);
	return TYPE_BOOL;
}

//	<term> ::=
//		<factor> <term_prime>
TypeMark Parser::term() {
	LOG(DEBUG) << "<term>";
	TypeMark tm_fact = factor();
	return termPrime(tm_fact);
}

//	<term_prime> ::=
//		`*' <factor> <term_prime>
//	|	`/' <factor> <term_prime>
//	|	epsilon
TypeMark Parser::termPrime(const TypeMark& tm) {
	LOG(DEBUG) << "<term_prime>";
	if (!matchToken(TOK_OP_TERM)) {
		LOG(DEBUG) << "epsilon";
		return tm;
	}
	std::shared_ptr<Token> op_tok = tok;
	scan();
	TypeMark tm_fact = factor();
	type_checker.checkCompatible(op_tok, tm, tm_fact);

	// If either side is `float', cast to `float'
	TypeMark tm_result;
	if ((tm == TYPE_FLT) || (tm_fact == TYPE_FLT)) {
		tm_result = TYPE_FLT;
	} else {
		tm_result = TYPE_INT;
	}
	return termPrime(tm_result);
}

//	<factor> ::=
//		`('<expression>`)'
//	|	<procedure_call>
//	|	[`-'] <name>
//	|	[`-'] <number>
//	|	<string>
//	|	`true'
//	|	`false'
// TODO: Type checking
TypeMark Parser::factor() {
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
				tm = name();
			} else {
				LOG(ERROR) << "Expected variable; got: " << tok->getStr();
			}
		} else if (matchToken(TOK_NUM)) {
			std::shared_ptr<Token> num_tok = number();
			tm = num_tok->getTypeMark();
		} else {
			LOG(ERROR) << "Minus sign must be followed by <name> or <number>.";
			LOG(ERROR) << "Got: " << tok->getStr();
		}

	// `('<expression>`)'
	} else if (matchToken(TOK_LPAREN)) {
		scan();
		tm = expression();
		expectToken(TOK_RPAREN);
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
		} else {
			tm = name();
		}

	// <number>
	} else if (matchToken(TOK_NUM)) {
		std::shared_ptr<Token> num_tok = number();
		tm = num_tok->getTypeMark();

	// <string>
	} else if (matchToken(TOK_STR)) {
		std::shared_ptr<LiteralToken<std::string>> str_tok = string();
		tm = str_tok->getTypeMark();

	// `true'
	} else if (matchToken(TOK_RW_TRUE)) {
		tm = TYPE_BOOL;
		scan();

	// `false'
	} else if (matchToken(TOK_RW_FALSE)) {
		tm = TYPE_BOOL;
		scan();

	// Oof
	} else {
		LOG(ERROR) << "Unexpected token: " << tok->getStr();
		tm = TYPE_NONE;
	}
	return tm;
}

//	<name> ::=
//		<identifier> [`['<expression>`]']
// TODO: Array bounds checking
TypeMark Parser::name() {
	LOG(DEBUG) << "<name>";
	std::shared_ptr<IdToken> id_tok = identifier(true);
	if (id_tok->getProcedure()) {
		LOG(ERROR) << "Expected variable; got procedure " << id_tok->getVal();
	}
	TypeMark tm = id_tok->getTypeMark();
	if (matchToken(TOK_LBRACK)) {
		LOG(DEBUG) << "Indexing array";
		if (id_tok->getProcedure() || (id_tok->getNumElements() < 1)) {
			LOG(ERROR) << "Attempt to index non-array symbol " << id_tok->getVal();
		}
		scan();
		TypeMark tm_idx = expression();
		type_checker.checkArrayIndex(tm_idx);
		expectToken(TOK_RBRACK);
		scan();
	}
	// TODO: How to return the index/ID?
	return tm;
}

//	<argument_list> ::=
//		<expression> `,' <argument_list>
//	|	<expression>
void Parser::argumentList(const int& idx, std::shared_ptr<IdToken> fun_tok) {
	LOG(DEBUG) << "<argument_list>";
	TypeMark tm_arg = expression();
	std::shared_ptr<IdToken> param = fun_tok->getParam(idx);
	if (!param) {
		LOG(ERROR) << "Unexpected parameter with type "
				<< Token::getTypeMarkName(tm_arg);
	} else if (!type_checker.checkCompatible(param->getTypeMark(), tm_arg)) {
		LOG(ERROR) << "Expected parameter with type "
				<< Token::getTypeMarkName(param->getTypeMark()) << "; got "
				<< Token::getTypeMarkName(tm_arg);
	}
	if (matchToken(TOK_COMMA)) {
		scan();
		argumentList(idx + 1, fun_tok);
	} else if (idx < fun_tok->getNumElements() - 1) {
		LOG(ERROR) << "Not enough parameters for procedure call "
				<< fun_tok->getVal();
	}
}

//	<number> ::=
//		[0-9][0-9_]*[.[0-9_]*]
std::shared_ptr<Token> Parser::number() {
	LOG(DEBUG) << "<number>";
	std::shared_ptr<Token> num_tok = std::shared_ptr<Token>(new Token());
	if (expectToken(TOK_NUM)) {
		num_tok = tok;
	}
	scan();
	return num_tok;
}

//	<string> ::=
//		`"'[^"]*`"'
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
	scan();
	return str_tok;
}
