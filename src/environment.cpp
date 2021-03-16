#include "environment.h"

#include "log.h"
#include "symbol_table.h"
#include "token.h"

Environment::Environment() {
	// Populate reserved words in global symbol table
	global_symbol_table.insert("program",
		std::shared_ptr<Token>(new Token(TOK_RW_PROG)));
	global_symbol_table.insert("is",
		std::shared_ptr<Token>(new Token(TOK_RW_IS)));
	global_symbol_table.insert("begin",
		std::shared_ptr<Token>(new Token(TOK_RW_BEG)));
	global_symbol_table.insert("end",
		std::shared_ptr<Token>(new Token(TOK_RW_END)));
	global_symbol_table.insert("global",
		std::shared_ptr<Token>(new Token(TOK_RW_GLOB)));
	global_symbol_table.insert("procedure",
		std::shared_ptr<Token>(new Token(TOK_RW_PROC)));
	global_symbol_table.insert("variable",
		std::shared_ptr<Token>(new Token(TOK_RW_VAR)));
	global_symbol_table.insert("integer",
		std::shared_ptr<Token>(new Token(TOK_RW_INT)));
	global_symbol_table.insert("float",
		std::shared_ptr<Token>(new Token(TOK_RW_FLT)));
	global_symbol_table.insert("string",
		std::shared_ptr<Token>(new Token(TOK_RW_STR)));
	global_symbol_table.insert("bool",
		std::shared_ptr<Token>(new Token(TOK_RW_BOOL)));
	global_symbol_table.insert("if",
		std::shared_ptr<Token>(new Token(TOK_RW_IF)));
	global_symbol_table.insert("then",
		std::shared_ptr<Token>(new Token(TOK_RW_THEN)));
	global_symbol_table.insert("else",
		std::shared_ptr<Token>(new Token(TOK_RW_ELSE)));
	global_symbol_table.insert("for",
		std::shared_ptr<Token>(new Token(TOK_RW_FOR)));
	global_symbol_table.insert("return",
		std::shared_ptr<Token>(new Token(TOK_RW_RET)));
	global_symbol_table.insert("not",
		std::shared_ptr<Token>(new Token(TOK_RW_NOT)));
	global_symbol_table.insert("true",
		std::shared_ptr<Token>(new Token(TOK_RW_TRUE)));
	global_symbol_table.insert("false",
		std::shared_ptr<Token>(new Token(TOK_RW_FALSE)));
}

std::shared_ptr<Token> Environment::lookup(const std::string& key) {
	std::shared_ptr<Token> ret_val = nullptr;
	if (!local_symbol_table_stack.empty()) {
		ret_val = local_symbol_table_stack.top().lookup(key);
	}
	if (!ret_val) {
		ret_val = global_symbol_table.lookup(key);
	}
	return ret_val;
}

bool Environment::insert(const std::string& key, std::shared_ptr<Token> t,
		const bool& is_global) {
	bool success = false;
	if (!isReserved(key)) {
		if (is_global) {
			success = global_symbol_table.insert(key, t);
		} else if (!local_symbol_table_stack.empty()) {
			success = local_symbol_table_stack.top().insert(key, t);
		}
	}
	return success;
}

bool Environment::isReserved(const std::string& key) {
	bool is_reserved = false;
	std::shared_ptr<Token> temp_token = global_symbol_table.lookup(key);
	if (temp_token && (temp_token->getType() >= TOK_RW_PROG)
			&& (temp_token->getType() <= TOK_RW_FALSE)) {
		is_reserved = true;
	}
	return is_reserved;
}

void Environment::push() {
	local_symbol_table_stack.push(SymbolTable());
}

void Environment::pop() {
	if (!local_symbol_table_stack.empty()) {
		local_symbol_table_stack.pop();
	} else {
		LOG::ss << "Attempt to pop empty symbol table stack.";
		LOG(LOG_LEVEL::WARN);
	}
}
