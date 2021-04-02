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
	bool errored;
	std::shared_ptr<Token> token;
	//std::shared_ptr<Token> next_token;
	void scan();
	bool expectToken(const TokenType&);
	bool assertToken(const TokenType&);
	void programHeader();
	void programBody();
	void declaration();
	void procedureDeclaration();
	void procedureHeader();
	void parameterList();
	void parameter();
	void procedureBody();
	void variableDeclaration();
	void typeMark();
	void bound();
	void statement();
	void procedureCall();
	void assigmentStatement();
	void destination();
	void ifStatement();
	void loopStatement();
	void returnStatement();
	void identifier();
	void expression();
	void expressionPrime();
	void arithOp();
	void arithOpPrime();
	void relation();
	void relationPrime();
	void term();
	void termPrime();
	void factor();
	void name();
	void argumentList();
	void number();
	void string();
};

#endif // PARSER_H
