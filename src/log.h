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

// Terminal color definitions
#define COL_RST  "\x1B[0m"
#define COL_CYN  "\x1B[36m"
#define COL_WHT  "\x1B[37m"
#define COL_YEL  "\x1B[33m"
#define COL_RED  "\x1B[31m"

enum loglevel {DEBUG, INFO, WARN, ERROR};

class LOG {
public:
	static loglevel min_level;

	LOG(loglevel type) {
		msg_level = type;
		std::cout << colors[type];
		operator << ("["+labels[type]+"]\t");
	}

	~LOG() {
		if(written) {
			std::cout << std::endl << COL_RST;
		}
		written = false;
	}

	template<class T>
	LOG& operator<<(const T &msg) {
		if(msg_level >= min_level) {
			std::cout << msg;
			written = true;
		}
		return *this;
	}

	/*
	 * Set min_level from an int
	 * Also does range checking
	 */
	static bool setMinLevel(int l) {
		switch (l) {
			case 0:		min_level = DEBUG;	break;
			case 1:		min_level = INFO;	break;
			case 2:		min_level = WARN;	break;
			case 3:		min_level = ERROR;	break;
			default:	return false;		break;
		}
		return true;
	}
private:
	bool written = false;
	loglevel msg_level = DEBUG;
	const std::string labels[4] = {"DEBUG", "INFO ", "WARN ", "ERROR"};
	const std::string colors[4] = {COL_CYN, COL_WHT, COL_YEL, COL_RED};
};

#endif // LOG_H

