#include "environment.h"

#include "log.h"
#include "symbol_table.h"
#include "token.h"

Environment::Environment() {
	// Populate reserved words in global symbol table
	global_symbol_table.insert("program",
		std::shared_ptr<Token>(new Token(TOK_RW_PROG, "program")));
	global_symbol_table.insert("is",
		std::shared_ptr<Token>(new Token(TOK_RW_IS, "is")));
	global_symbol_table.insert("begin",
		std::shared_ptr<Token>(new Token(TOK_RW_BEG, "begin")));
	global_symbol_table.insert("end",
		std::shared_ptr<Token>(new Token(TOK_RW_END, "end")));
	global_symbol_table.insert("global",
		std::shared_ptr<Token>(new Token(TOK_RW_GLOB, "global")));
	global_symbol_table.insert("procedure",
		std::shared_ptr<Token>(new Token(TOK_RW_PROC, "procedure")));
	global_symbol_table.insert("variable",
		std::shared_ptr<Token>(new Token(TOK_RW_VAR, "variable")));
	global_symbol_table.insert("integer",
		std::shared_ptr<Token>(new Token(TOK_RW_INT, "integer")));
	global_symbol_table.insert("float",
		std::shared_ptr<Token>(new Token(TOK_RW_FLT, "float")));
	global_symbol_table.insert("string",
		std::shared_ptr<Token>(new Token(TOK_RW_STR, "string")));
	global_symbol_table.insert("bool",
		std::shared_ptr<Token>(new Token(TOK_RW_BOOL, "bool")));
	global_symbol_table.insert("if",
		std::shared_ptr<Token>(new Token(TOK_RW_IF, "if")));
	global_symbol_table.insert("then",
		std::shared_ptr<Token>(new Token(TOK_RW_THEN, "then")));
	global_symbol_table.insert("else",
		std::shared_ptr<Token>(new Token(TOK_RW_ELSE, "else")));
	global_symbol_table.insert("for",
		std::shared_ptr<Token>(new Token(TOK_RW_FOR, "for")));
	global_symbol_table.insert("return",
		std::shared_ptr<Token>(new Token(TOK_RW_RET, "return")));
	global_symbol_table.insert("not",
		std::shared_ptr<Token>(new Token(TOK_RW_NOT, "not")));
	global_symbol_table.insert("true",
		std::shared_ptr<Token>(new Token(TOK_RW_TRUE, "true")));
	global_symbol_table.insert("false",
		std::shared_ptr<Token>(new Token(TOK_RW_FALSE, "false")));
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

bool Environment::insert(const std::string& key,
		std::shared_ptr<Token> t, const bool& is_global) {
	bool success = false;
	if (!isReserved(key)) {
		if (is_global) {
			LOG(DEBUG) << "Adding global";
			success = global_symbol_table.insert(key, t);
		} else if (!local_symbol_table_stack.empty()) {
			LOG(DEBUG) << "Adding local";
			success = local_symbol_table_stack.top().insert(key, t);
		} else {
			LOG(WARN) << "Attempt to add local symbol with no local symbol table";
		}
	} else {
		LOG(WARN) << "Attempt to overwrite reserved word: " << key;
	}
	if (success) {
		LOG(DEBUG) << "Added " << t->getStr()
				<< " to symbol table with key " << key;
	} else {
		LOG(WARN) << "Failed to add " << t->getStr()
				<< " to symbol table with key " << key;
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
	LOG(DEBUG) << "Pushing symbol table stack";
	local_symbol_table_stack.push(SymbolTable());
}

void Environment::pop() {
	if (!local_symbol_table_stack.empty()) {
		LOG(DEBUG) << "Popping symbol table stack";
		local_symbol_table_stack.pop();
	} else {
		LOG(WARN) << "Attempt to pop empty symbol table stack";
	}
}

std::string Environment::getLocalStr() {
	if (!local_symbol_table_stack.empty()) {
		return local_symbol_table_stack.top().getStr();
	} else {
		LOG(WARN) << "Attempt to get local symbol table string with empty stack";
		return "\n";
	}
}

std::string Environment::getGlobalStr() {
	return global_symbol_table.getStr();
}
