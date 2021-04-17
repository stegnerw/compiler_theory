#ifndef PARSER_H
#define PARSER_H

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "environment.h"
#include "scanner.h"
#include "token.h"

class Parser {
public:
	Parser();
	bool init(const std::string&);
	bool parse();  // program

private:
	std::shared_ptr<Environment> env;
	Scanner scanner;
	std::shared_ptr<Token> tok;
	//std::shared_ptr<Token> next_tok;
	void scan();
	bool matchToken(const TokenType&);
	bool expectToken(const TokenType&);
	void programHeader();
	void programBody();
	void declarations(bool);
	void statements();
	void declaration(bool);
	void procedureDeclaration(const bool&);
	void procedureHeader(const bool&);
	void parameterList();
	void parameter();
	void procedureBody();
	void variableDeclaration(const bool&);
	TypeMark typeMark();
	int bound();
	void statement();
	void procedureCall();
	void assigmentStatement();
	TypeMark destination();
	void ifStatement();
	void loopStatement();
	TypeMark returnStatement();
	std::shared_ptr<IdToken> identifier();
	TypeMark expression();
	TypeMark expressionPrime();
	TypeMark arithOp();
	TypeMark arithOpPrime();
	TypeMark relation();
	TypeMark relationPrime();
	TypeMark term();
	TypeMark termPrime();
	TypeMark factor();
	TypeMark name();
	void argumentList();
	std::shared_ptr<Token> number();
	std::shared_ptr<LiteralToken<std::string>> string();
};

#endif // PARSER_H
