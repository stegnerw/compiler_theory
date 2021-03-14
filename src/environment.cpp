#include "environment.h"

#include "log.h"
#include "symbol_table.h"
#include "token.h"

Environment::Environment() {
	// Populate reserved words in global symbol table
	global_symbol_table.insert("program", TOK_RW_PROG);
	global_symbol_table.insert("is", TOK_RW_IS);
	global_symbol_table.insert("begin", TOK_RW_BEG);
	global_symbol_table.insert("end", TOK_RW_END);
	global_symbol_table.insert("global", TOK_RW_GLOB);
	global_symbol_table.insert("procedure", TOK_RW_PROC);
	global_symbol_table.insert("variable", TOK_RW_VAR);
	global_symbol_table.insert("integer", TOK_RW_INT);
	global_symbol_table.insert("float", TOK_RW_FLT);
	global_symbol_table.insert("string", TOK_RW_STR);
	global_symbol_table.insert("bool", TOK_RW_BOOL);
	global_symbol_table.insert("if", TOK_RW_IF);
	global_symbol_table.insert("then", TOK_RW_THEN);
	global_symbol_table.insert("else", TOK_RW_ELSE);
	global_symbol_table.insert("for", TOK_RW_FOR);
	global_symbol_table.insert("return", TOK_RW_RET);
	global_symbol_table.insert("not", TOK_RW_NOT);
	global_symbol_table.insert("true", TOK_RW_TRUE);
	global_symbol_table.insert("false", TOK_RW_FALSE);
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

// TODO: This is a bit messy... Maybe clean it up?
bool Environment::insert(const std::string& key, const TokenType& tt,
		const bool& is_global) {
	bool success = false;
	if (!isReserved(key)) {
		if (is_global) {
			success = global_symbol_table.insert(key, tt);
		} else if (!local_symbol_table_stack.empty()) {
			success = local_symbol_table_stack.top().insert(key, tt);
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

