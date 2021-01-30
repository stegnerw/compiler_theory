/*
 * File:		Log.h
 * Author:		Alberto Lepe <dev@alepe.com>
 * Contributor: Wayne Stegner <wayne.stegner@protonmail.com>
 *
 * This code was found and modified from this post on Stack Overflow:
 * https://stackoverflow.com/questions/5028302/small-logger-class/32262143#32262143
 * I (Wayne Stegner) restructured it and added colors
 * I plan to make it output to a log file as well as printing
 */

#ifndef LOG_H
#define LOG_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

// Terminal color definitions
#define COL_RST  "\x1B[0m"
#define COL_CYN  "\x1B[36m"
#define COL_WHT  "\x1B[37m"
#define COL_YEL  "\x1B[33m"
#define COL_RED  "\x1B[31m"

enum LOG_LEVEL {DEBUG, INFO, WARN, ERROR};

class LOG {
public:
	static std::stringstream ss;
	LOG(LOG_LEVEL type);
	static bool setLogFile(std::string f);
	static bool setMinLevel(int l);
private:
	static LOG_LEVEL min_level;
	static std::string log_file;
	static std::ofstream log_fstream;
	static const std::string labels[4];
	static const std::string colors[4];
	LOG_LEVEL msg_level;
};

#endif // LOG_H

