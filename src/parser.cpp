#include "parser.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

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
	return !LOG::hasErrored();
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

void Parser::scan() {
	do {
		token = scanner.getToken();
	} while(token->getType() == TOK_INVALID);
}

bool Parser::matchToken(const TokenType& t) {
	LOG(DEBUG) << "Matching " << Token::getTokenName(t);
	if (token->getType() == t) {
		LOG(DEBUG) << "Match success";
		return true;
	}
	LOG(DEBUG) << "Match failed for: " << Token::getTokenName(t);
	return false;
}

bool Parser::expectToken(const TokenType& t) {
	LOG(DEBUG) << "Expecting " << Token::getTokenName(t);
	if (matchToken(t)) {
		LOG(DEBUG) << "Expect passed";
		return true;
	}
	LOG(ERROR) << "Expected " << Token::getTokenName(t)
			<< ", got " << token->getStr() << " instead";
	return false;
}

//	<program_header> ::=
//		`program' <identifier> `is'
void Parser::programHeader() {
	LOG(DEBUG) << "<program_header>";
	expectToken(TOK_RW_PROG);
	scan();
	identifier();
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
		LOG(ERROR) << "Unexpected token: " << token->getStr();
		LOG(ERROR) << "Expected: " << Token::getTokenName(TOK_RW_PROC) << " or "
				<< Token::getTokenName(TOK_RW_VAR);
		scan();  // TODO: Do I want to scan here? Other ways to handle errors?
	}
}

//	<procedure_declaration> ::=
//		<procedure_header> <procedure_body>
void Parser::procedureDeclaration(const bool& is_global) {
	LOG(DEBUG) << "<procedure_declaration>";
	procedureHeader(is_global);
	procedureBody();
	LOG(DEBUG) << "Done parsing function with local symbol table:\n"
		<< env->getLocalStr();
	env->pop();  // Remove scope
}

//	<procedure_header>
//		`procedure' <identifier> `:' <type_mark> `('[<parameter_list>]`)'
void Parser::procedureHeader(const bool& is_global) {
	LOG(DEBUG) << "<procedure_header>";

	// This should not happen but just in case
	expectToken(TOK_RW_PROC);
	scan();
	std::shared_ptr<IdToken> id_token = identifier();
	if (!env->insert(id_token->getVal(), id_token, is_global)) {
		LOG(ERROR) << "Failed to add procedure to symbol table - see logs";
	}
	expectToken(TOK_COLON);
	scan();
	TypeMark tm = typeMark();
	if (id_token->isValid()) {
		id_token->setTypeMark(tm);
		id_token->setType(TOK_ID_PROC);
	}
	env->push();  // Add new scope

	// Procedure must be locally visible for recursion
	// Only add if the procedure is not global
	if (!is_global && !env->insert(id_token->getVal(), id_token, false)) {
		LOG(ERROR) << "Failed to add procedure to local symbol table - see logs";
	}
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
	parameter();
	if (matchToken(TOK_COMMA)) {
		scan();
		parameterList();
	}
}

//	<parameter> ::=
//		<variable_declaration>
void Parser::parameter() {
	LOG(DEBUG) << "<parameter>";
	variableDeclaration(false);
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
void Parser::variableDeclaration(const bool& is_global) {
	LOG(DEBUG) << "<variable_declaration>";
	expectToken(TOK_RW_VAR);
	scan();
	std::shared_ptr<IdToken> id_token = identifier();
	if (!env->insert(id_token->getVal(), id_token, is_global)) {
		LOG(ERROR) << "Failed to add variable to symbol table - see logs";
	}
	expectToken(TOK_COLON);
	scan();
	TypeMark tm = typeMark();
	if (id_token->isValid()) {
		id_token->setTypeMark(tm);
		id_token->setType(TOK_ID_VAR);
	}
	if (matchToken(TOK_LBRACK)) {
		LOG(DEBUG) << "Variable is an array";
		scan();
		id_token->setNumElements(bound());
		expectToken(TOK_RBRACK);
		scan();
	}
	LOG(DEBUG) << "Declared variable " << id_token->getStr();
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
		LOG(ERROR) << "Invalid type mark: "
				<< token->getStr();
	}
	scan();
	return tm;
}

//	<bound> ::=
//		<number>
int Parser::bound() {
	LOG(DEBUG) << "<bound>";
	std::shared_ptr<Token> num_tok = number();
	std::shared_ptr<LiteralToken<int>> bound_tok =
			std::dynamic_pointer_cast<LiteralToken<int>>(num_tok);
	if (bound_tok) {
		return bound_tok->getVal();
	} else {
		LOG(ERROR) << "Invalid bound received: " << num_tok->getStr();
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
		LOG(DEBUG) << "Invalid statement: " << token->getStr();
	}
}

//	<procedure_call> ::=
//		<identifier>`('[<argument_list>]`)'
void Parser::procedureCall() {
	LOG(DEBUG) << "<procedure_call>";
	identifier();
	expectToken(TOK_LPAREN);
	scan();
	if (!matchToken(TOK_RPAREN)) {
		argumentList();
	}
	expectToken(TOK_RPAREN);
	scan();
}

//	<assignment_statement> ::=
//		<destination> `:=' <expression>
void Parser::assigmentStatement() {
	LOG(DEBUG) << "<assignment_statement>";
	destination();
	expectToken(TOK_OP_ASS);
	scan();
	expression();
}

//	<destination> ::=
//		<identifier>[`['<expression>`]']
// TODO: Check array indexing
// TODO: Type checking
TypeMark Parser::destination() {
	LOG(DEBUG) << "<destination>";
	identifier();
	if (matchToken(TOK_LBRACK)) {
		scan();
		expression();
		expectToken(TOK_RBRACK);
		scan();
	}
	return TYPE_NONE;
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
	expression();
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
	expression();
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
// TODO: Type checking
TypeMark Parser::returnStatement() {
	LOG(DEBUG) << "<return_statement>";
	expectToken(TOK_RW_RET);
	scan();
	expression();
	return TYPE_NONE;
}

//	<identifier> ::=
//		[a-zA-Z][a-zA-Z0-9_]*
std::shared_ptr<IdToken> Parser::identifier() {
	LOG(DEBUG) << "<identifier>";
	std::shared_ptr<IdToken> id_token;
	if (expectToken(TOK_IDENT)) {
		id_token = std::dynamic_pointer_cast<IdToken>(token);
		scan();
	} else {
		id_token = std::shared_ptr<IdToken>(new IdToken(TOK_INVALID, ""));
	}
	return id_token;
}

//	<expression> ::=
//		[`not'] <arith_op> <expression_prime>
// TODO: Type checking
TypeMark Parser::expression() {
	LOG(DEBUG) << "<expression>";
	bool bitwise_not = matchToken(TOK_RW_NOT);
	if (bitwise_not) {
		LOG(DEBUG) << "Bitwise not";
		scan();
	}
	arithOp();
	expressionPrime();
	return TYPE_NONE;
}

//	<expression_prime> ::=
//		`&' <arith_op> <expression_prime>
//	|	`|' <arith_op> <expression_prime>
//	|	epsilon
// TODO: Type checking
TypeMark Parser::expressionPrime() {
	LOG(DEBUG) << "<expression_prime>";
	if (matchToken(TOK_OP_EXPR)) {
		LOG(DEBUG) << "Bitwise " << token->getVal();
		scan();
		arithOp();
		expressionPrime();
	} else {
		LOG(DEBUG) << "epsilon";
	}
	return TYPE_NONE;
}

//	<arith_op> ::=
//		<relation> <arith_op_prime>
// TODO: Type checking
TypeMark Parser::arithOp() {
	LOG(DEBUG) << "<arith_op>";
	relation();
	arithOpPrime();
	return TYPE_NONE;
}

//	<arith_op_prime> ::=
//		`+' <relation> <arith_op_prime>
//	|	`-' <relation> <arith_op_prime>
//	|	epsilon
// TODO: Type checking
TypeMark Parser::arithOpPrime() {
	LOG(DEBUG) << "<arith_op_prime>";
	if (matchToken(TOK_OP_ARITH)) {
		LOG(DEBUG) << "Arithmetic: " << token->getStr();
		scan();
		relation();
		arithOpPrime();
	} else {
		LOG(DEBUG) << "epsilon";
	}
	return TYPE_NONE;
}

//	<relation> ::=
//		<term> <relation_prime>
// TODO: Type checking
TypeMark Parser::relation() {
	LOG(DEBUG) << "<relation>";
	term();
	relationPrime();
	return TYPE_NONE;
}

//	<relation_prime> ::=
//		`<' <term> <relation_prime>
//	|	`>=' <term> <relation_prime>
//	|	`<=' <term> <relation_prime>
//	|	`>' <term> <relation_prime>
//	|	`==' <term> <relation_prime>
//	|	`!=' <term> <relation_prime>
//	|	epsilon
// TODO: Type checking
TypeMark Parser::relationPrime() {
	LOG(DEBUG) << "<relation_prime>";
	if (matchToken(TOK_OP_RELAT)) {
		LOG(DEBUG) << "Relation: " << token->getStr();
		scan();
		term();
		relationPrime();
	} else {
		LOG(DEBUG) << "epsilon";
	}
	return TYPE_NONE;
}

//	<term> ::=
//		<factor> <term_prime>
// TODO: Type checking
TypeMark Parser::term() {
	LOG(DEBUG) << "<term>";
	factor();
	termPrime();
	return TYPE_NONE;
}

//	<term_prime> ::=
//		`*' <factor> <term_prime>
//	|	`/' <factor> <term_prime>
//	|	epsilon
// TODO: Type checking
TypeMark Parser::termPrime() {
	LOG(DEBUG) << "<term_prime>";
	if (matchToken(TOK_OP_TERM)) {
		LOG(DEBUG) << "Term: " << token->getStr();
		scan();
		factor();
		termPrime();
	} else {
		LOG(DEBUG) << "epsilon";
	}
	return TYPE_NONE;
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
	if (matchToken(TOK_OP_ARITH) && (token->getVal() == "-")) {
		scan();
		if (matchToken(TOK_IDENT)) {
			std::shared_ptr<IdToken> id_tok = std::dynamic_pointer_cast<IdToken>(
					env->lookup(token->getVal()));
			if (!id_tok) {
				LOG(ERROR) << "Identifier not declared in this scope: "
						<< token->getStr();
			} else if (id_tok->getType() == TOK_ID_VAR) {
				name();
			} else {
				LOG(ERROR) << "Expected variable; got: " << token->getStr();
			}
		} else if (matchToken(TOK_NUM)) {
			number();
		} else {
			LOG(ERROR) << "Minus sign must be followed by <name> or <number>.";
			LOG(ERROR) << "Got: " << token->getStr();
		}

	// `('<expression>`)'
	} else if (matchToken(TOK_LPAREN)) {
		scan();
		expression();
		expectToken(TOK_RPAREN);
		scan();

	// <procedure_call> or <name>
	} else if (matchToken(TOK_IDENT)) {
		// TODO: How to tell procedure call vs name? Add something to id_token
		std::shared_ptr<IdToken> id_tok = std::dynamic_pointer_cast<IdToken>(
				env->lookup(token->getVal()));
		if (!id_tok) {
			LOG(ERROR) << "Identifier not declared in this scope: "
					<< token->getStr();
		} else if (id_tok->getType() == TOK_ID_VAR) {
			name();
		} else if (id_tok->getType() == TOK_ID_PROC) {
			procedureCall();
		} else {
			LOG(ERROR) << "Unknown identifier: " << id_tok->getStr();
		}

	// <number>
	} else if (matchToken(TOK_NUM)) {
		number();

	// <string>
	} else if (matchToken(TOK_STR)) {
		string();

	// `true'
	} else if (matchToken(TOK_RW_TRUE)) {
		LOG(DEBUG) << token->getStr();
		scan();

	// `false'
	} else if (matchToken(TOK_RW_FALSE)) {
		LOG(DEBUG) << token->getStr();
		scan();

	// Something went wrong
	} else {
		LOG(ERROR) << "Invalid factor: " << token->getStr();
	}
	return tm;
}

//	<name> ::=
//		<identifier> [`['<expression>`]']
// TODO: Array bounds checking
TypeMark Parser::name() {
	LOG(DEBUG) << "<name>";
	std::shared_ptr<IdToken> id_tok = identifier();
	TypeMark tm = id_tok->getTypeMark();
	if (matchToken(TOK_LBRACK)) {
		// TODO: Check if it's actually an array
		LOG(DEBUG) << "Indexing";
		scan();
		expression();
		expectToken(TOK_RBRACK);
		scan();
	}
	// TODO: How to return the index/ID?
	return tm;
}

//	<argument_list> ::=
//		<expression> `,' <argument_list>
//	|	<expression>
void Parser::argumentList() {
	LOG(DEBUG) << "<argument_list>";
	expression();
	if (matchToken(TOK_COMMA)) {
		scan();
		argumentList();
	}
}

//	<number> ::=
//		[0-9][0-9_]*[.[0-9_]*]
std::shared_ptr<Token> Parser::number() {
	LOG(DEBUG) << "<number>";
	std::shared_ptr<Token> num_tok = std::shared_ptr<Token>(new Token());
	if (expectToken(TOK_NUM)) {
		num_tok = token;
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
		str_tok = std::dynamic_pointer_cast<LiteralToken<std::string>>(token);
	} else {
		LOG(ERROR) << "Using empty string";
	}
	scan();
	return str_tok;
}
