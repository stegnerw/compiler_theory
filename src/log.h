/*
 * File:		Log.h
 * Author:		Alberto Lepe <dev@alepe.com>
 * Contributor:	Wayne Stegner <wayne.stegner@protonmail.com>
 *
 * This code was found and modified from this post on Stack Overflow:
 * https://stackoverflow.com/questions/5028302/small-logger-class/32262143#32262143
 * I (Wayne Stegner) restructured it and added COLORS
 */

#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Terminal color definitions
const std::string COL_RST = "\x1B[0m";
const std::string COL_CYN = "\x1B[36m";
const std::string COL_WHT = "\x1B[37m";
const std::string COL_YEL = "\x1B[33m";
const std::string COL_RED = "\x1B[31m";

enum LOG_LEVEL {DEBUG, INFO, WARN, ERROR};

class LOG {
public:
	static std::stringstream ss;
	static int line_number;
	LOG(LOG_LEVEL type);
	static bool setLogFile(std::string);
	static bool setMinLevel(int);

private:
	static LOG_LEVEL min_level;
	static std::string log_file;
	static std::ofstream log_fstream;
	static const std::string LABELS[4];
	static const std::string COLORS[4];
	LOG_LEVEL msg_level;
};

#endif // LOG_H

