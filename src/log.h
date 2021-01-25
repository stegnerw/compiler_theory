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

// Terminal color definitions
#define COL_RST  "\x1B[0m"
#define COL_CYN  "\x1B[36m"
#define COL_WHT  "\x1B[37m"
#define COL_YEL  "\x1B[33m"
#define COL_RED  "\x1B[31m"

using namespace std;

enum loglevel {DEBUG, INFO, WARN, ERROR};

class LOG {
public:
	static loglevel min_level;
	LOG(loglevel type);
	~LOG();
	template<class T>
	LOG& operator<<(const T &msg);
	static bool setMinLevel(int l);
private:
	bool written = false;
	loglevel msg_level = DEBUG;
	const std::string labels[4];
	const std::string colors[4] = {COL_CYN, COL_WHT, COL_YEL, COL_RED};
};

#endif // LOG_H

