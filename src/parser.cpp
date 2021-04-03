#include "parser.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "environment.h"
#include "log.h"
#include "scanner.h"
#include "token.h"

Parser::Parser() : env(new Environment()), scanner(env), errored(false) {}

bool Parser::init(const std::string& src_file) {
	bool init_success = true;
	errored = false;
	if (!scanner.init(src_file)) {
		init_success = false;
		LOG(ERROR) << "Failed to initialize parser";
		LOG(ERROR) << "See logs";
		errored = true;
	} else {
		scan();
	}
	return init_success;
}

//	<program> ::=
//		<program_header> <program_body> .
bool Parser::parse() {
	LOG(INFO) << "Begin parsing";
	LOG(DEBUG) << "Parsing <program>";
	programHeader();
	programBody();
	expectToken(TOK_PERIOD);
	LOG(INFO) << "Done parsing";
	return !errored;
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

void Parser::scan() {
	do {
		token = scanner.getToken();
		if (scanner.hasErrored()) {
			errored = true;
		}
	} while(token->getType() == TOK_INVALID);
}

bool Parser::matchToken(const TokenType& t) {
	LOG(DEBUG) << "Matching " << Token::getTokenName(t);
	if (token->getType() == t) {
		LOG(DEBUG) << "Match success";
		scan();
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
	LOG(ERROR) << "Expect failed - expected " << Token::getTokenName(t)
			<< ", got " << Token::getTokenName(token->getType()) << " instead";
	errored = true;
	scan();
	return false;
}

//	<program_header> ::=
//		program <identifier> is
void Parser::programHeader() {
	LOG(DEBUG) << "Parsing <program_header>";
	expectToken(TOK_RW_PROG);
	// The program name does not get added to the symbol table
	identifier(false, false);
	expectToken(TOK_RW_IS);
}

//	<program_body> ::=
//			(<declaration>;)*
//		begin
//			(<statement>;)*
//		end program
void Parser::programBody() {
	LOG(DEBUG) << "Parsing <program_body>";
	// FIRST(<declaration>) = {global, procedure, variable}
	while ((token->getType() == TOK_RW_GLOB) || (token->getType() == TOK_RW_PROC)
			|| (token->getType() == TOK_RW_VAR)) {
		declaration(true);
		expectToken(TOK_SEMICOL);
	}
	expectToken(TOK_RW_BEG);
	// FIRST(<statement>) = {<identifier>, if, for, return}
	while((token->getType() == TOK_IDENT) || (token->getType() == TOK_RW_IF)
			|| (token->getType() == TOK_RW_FOR) || (token->getType() == TOK_RW_RET)) {
		statement();
		expectToken(TOK_SEMICOL);
	}
	expectToken(TOK_RW_END);
	expectToken(TOK_RW_PROG);
}

//	<declaration> ::=
//		[global] <procedure_declaration>
//	| [global] <variable_declaration>
void Parser::declaration(bool is_global) {
	LOG(DEBUG) << "Parsing <declaration>";
	is_global = matchToken(TOK_RW_GLOB) || is_global;
	if (matchToken(TOK_RW_PROC)) {
		procedureDeclaration(is_global);
	} else if(matchToken(TOK_RW_VAR)) {
		variableDeclaration(is_global);
	} else {
		LOG(ERROR) << "Unexpected token: " << Token::getTokenName(token->getType());
		LOG(ERROR) << "Expected: " << Token::getTokenName(TOK_RW_PROC) << ", "
				<< Token::getTokenName(TOK_RW_VAR);
		errored = true;
	}
}

//	<procedure_declaration> ::=
//		<procedure_header> <procedure_body>
void Parser::procedureDeclaration(const bool& is_global) {
	LOG(DEBUG) << "Parsing <procedure_declaration>";
	procedureHeader(is_global);
	procedureBody();
}

//	<procedure_header>
//		procedure <identifier> : <type_mark> ( [<parameter_list>] )
void Parser::procedureHeader(const bool& is_global) {
	LOG(DEBUG) << "Parsing <procedure_header>";
	// Already ate TOK_RW_PROC
	std::shared_ptr<IdToken> id_token = identifier(is_global, true);
	if (!id_token) {
		LOG(ERROR) << "Failed to add procedure - see logs";
	}
	env->push();
	expectToken(TOK_COLON);
	TypeMark tm = typeMark();
	id_token->setTypeMark(tm);
	expectToken(TOK_LPAREN);
	if (matchToken(TOK_RW_VAR)) {
		parameterList();
	}
	expectToken(TOK_RPAREN);
}

//	<parameter_list> ::=
//		<parameter>, <parameter_list>
//	|	<parameter>
void Parser::parameterList() {
	LOG(DEBUG) << "Parsing <parameter_list>";
	parameter();
	if (matchToken(TOK_COMMA)) {
		parameterList();
	}
}

//	<parameter> ::=
//		<variable_declaration>
void Parser::parameter() {
	LOG(DEBUG) << "Parsing <parameter>";
	variableDeclaration(false);
}

//	<procedure_body> ::=
//			(<declaration>;)*
//		begin
//			(<statement>;)*
//		end procedure
void Parser::procedureBody() {
	LOG(DEBUG) << "Parsing <procedure_body>";
	// TODO: Parse procedure body
	env->pop();
}

//	<variable_declaration> ::=
//		variable <identifier> : <type_mark> [[ <bound> ]]
void Parser::variableDeclaration(const bool& is_global) {
	LOG(DEBUG) << "Parsing <variable_declaration>";
	std::shared_ptr<IdToken> id_token = identifier(is_global, true);
	expectToken(TOK_COLON);
	TypeMark tm = typeMark();
	id_token->setTypeMark(tm);
	LOG(DEBUG) << "Declared variable " << id_token->getStr();
	if (matchToken(TOK_LBRACK)) {
		bound();
		expectToken(TOK_RBRACK);
		LOG(DEBUG) << "Variable is an array";
	}
}

//	<type_mark> ::=
//		integer | float | string | bool
TypeMark Parser::typeMark() {
	LOG(DEBUG) << "Parsing <type_mark>";
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
		LOG(ERROR) << "Expected <type_mark>; got: "
				<< Token::getTokenName(token->getType());
		errored = true;
	}
	return tm;
}

//	<bound> ::=
//		<number>
void Parser::bound() {
	LOG(DEBUG) << "Parsing <bound>";
	number();
}

//	<statement> ::=
//		<assignment_statement>
//	|	<if_statement>
//	|	<loop_statement>
//	|	<return_statement>
void Parser::statement() {
	LOG(DEBUG) << "Parsing <statement>";
}

//	<procedure_call> ::=
//		<identifier>( [<argument_list>] )
void Parser::procedureCall() {
	LOG(DEBUG) << "Parsing <procedure_call>";
}

//	<assignment_statement> ::=
//		<destination> := <expression>
void Parser::assigmentStatement() {
	LOG(DEBUG) << "Parsing <assignment_statement>";
}

//	<destination> ::=
//		<identifier>[[ <expression> ]]
void Parser::destination() {
	LOG(DEBUG) << "Parsing <destination>";
}

//	<if_statement> ::=
//		if ( <expression> ) then (<statement>;)*
//		[else (<statement>;)*]
//		end if
void Parser::ifStatement() {
	LOG(DEBUG) << "Parsing <if_statement>";
}

//	<loop_statement> ::=
//		for ( <assignment_statement>; <expression> )
//			(<statement>;)*
//		end for
void Parser::loopStatement() {
	LOG(DEBUG) << "Parsing <loop_statement>";
}

//	<return_statement> ::=
//		return <expression>
void Parser::returnStatement() {
	LOG(DEBUG) << "Parsing <return_statement>";
}

//	<identifier> ::=
//		[a-zA-Z][a-zA-Z0-9]*
std::shared_ptr<IdToken> Parser::identifier(const bool& is_global,
		const bool& add_symbol) {
	LOG(DEBUG) << "Parsing <identifier>";
	std::shared_ptr<IdToken> id_token = std::dynamic_pointer_cast<IdToken>(token);
	if (expectToken(TOK_IDENT)) {

		// Add to symbol table if applicable
		// TODO: Improve readability here? Trying to avoid too many nested ifs
		if (add_symbol
				&& !env->insert(id_token->getLexeme(), id_token, is_global)) {
			LOG(ERROR) << "Failed to add symbol to symbol table: "
					<< id_token->getStr();
			errored = true;
			id_token = nullptr;
		}
	} else {

		// Invalidate id_token if not <identifier>
		// It is probably already null from failing the cast
		id_token = nullptr;
	}
	return id_token;
}

//	<expression> ::=
//		[not] <arith_op> <expression_prime>
void Parser::expression() {
	LOG(DEBUG) << "Parsing <expression>";
	bool bitwise_not = matchToken(TOK_RW_NOT);
	if (bitwise_not) {
		LOG(DEBUG) << "Bitwise not";
	}
	arithOp();
	expressionPrime();
}

//	<expression_prime> ::=
//		& <arith_op> <expression_prime>
//	|	| <arith_op> <expression_prime>
//	|	epsilon
void Parser::expressionPrime() {
	LOG(DEBUG) << "Parsing <expression_prime>";
	std::shared_ptr<OpToken> op_token = std::dynamic_pointer_cast<OpToken>(token);
	if (matchToken(TOK_OP_EXPR)) {
		LOG(DEBUG) << "Bitwise " << op_token->getVal();
		arithOp();
		expressionPrime();
	} else {
		LOG(DEBUG) << "epsilon";
	}
}

//	<arith_op> ::=
//		<relation> <arith_op_prime>
void Parser::arithOp() {
	LOG(DEBUG) << "Parsing <arith_op>";
	relation();
	arithOpPrime();
}

//	<arith_op_prime> ::=
//		+ <relation> <arith_op_prime>
//	|	- <relation> <arith_op_prime>
//	|	epsilon
void Parser::arithOpPrime() {
	LOG(DEBUG) << "Parsing <arith_op_prime>";
	std::shared_ptr<OpToken> op_token = std::dynamic_pointer_cast<OpToken>(token);
	if (matchToken(TOK_OP_ARITH)) {
		LOG(DEBUG) << "Arithmetic " << op_token->getVal();
		relation();
		arithOpPrime();
	} else {
		LOG(DEBUG) << "epsilon";
	}
}

//	<relation> ::=
//		<term> <relation_prime>
void Parser::relation() {
	LOG(DEBUG) << "Parsing <relation>";
}

//	<relation_prime> ::=
//		< <term> <relation_prime>
//	|	>= <term> <relation_prime>
//	|	<= <term> <relation_prime>
//	|	> <term> <relation_prime>
//	|	== <term> <relation_prime>
//	|	!= <term> <relation_prime>
//	|	epsilon
void Parser::relationPrime() {
	LOG(DEBUG) << "Parsing <relation_prime>";
}

//	<term> ::=
//		<factor> <term_prime>
void Parser::term() {
	LOG(DEBUG) << "Parsing <term>";
}

//	<term_prime> ::=
//		* <factor> <term_prime>
//	|	/ <factor> <term_prime>
//	|	epsilon
void Parser::termPrime() {
	LOG(DEBUG) << "Parsing <term_prime>";
}

//	<factor> ::=
//		( <expression> )
//	|	<procedure_call>
//	|	[-] <name>
//	|	[-] <number>
//	|	<string>
//	|	true
//	|	false
void Parser::factor() {
	LOG(DEBUG) << "Parsing <factor>";
}

//	<name> ::=
//		<identifier> [[ <expression> ]]
void Parser::name() {
	LOG(DEBUG) << "Parsing <name>";
}

//	<argument_list> ::=
//		<expression> , <argument_list>
//	|	<expression>
void Parser::argumentList() {
	LOG(DEBUG) << "Parsing <argument_list>";
}

//	<number> ::=
//		[0-9][0-9_]*[.[0-9_]*]
void Parser::number() {
	LOG(DEBUG) << "Parsing <number>";
	expectToken(TOK_NUM);
}

//	<string> ::=
//		"[^"]*"
void Parser::string() {
	LOG(DEBUG) << "Parsing <string>";
}
