/*
 * File:		Log.h
 * Author:		Alberto Lepe <dev@alepe.com>
 * Contributor:	Wayne Stegner <wayne.stegner@protonmail.com>
 *
 * This code was found and modified from this post on Stack Overflow:
 * https://stackoverflow.com/questions/5028302/small-logger-class/32262143#32262143
 * I restructured it and added COLORS
 */

#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// Terminal color definitions
const std::string COL_RST = "\x1B[0m";
const std::string COL_CYN = "\x1B[36m";
const std::string COL_WHT = "\x1B[37m";
const std::string COL_YEL = "\x1B[33m";
const std::string COL_RED = "\x1B[31m";

enum LOG_LEVEL {
	DEBUG = 0,
	INFO,
	WARN,
	ERROR,
};

class LOG {
public:
	LOG() = delete;
	LOG(LOG_LEVEL type) : msg_level(type) {
		// Open log file set console color, and print header
		log_fstream.open(log_file, std::ios::app);
		std::cout << COLORS[type];
		operator<<(LABELS[type]);
		operator<<(" Line ") << std::setfill(' ') << std::setw(3) << line_number
				<< ' ';
	}
	~LOG() {
		// Reset color, new line, and handle stream
		std::cout << COL_RST;
		operator<<('\n');
		std::cout << std::flush;
		if (log_fstream) {
			log_fstream.close();
		}
	}
	template <class T>
	LOG& operator<<(const T &msg) {
		if (msg_level >= min_level) {
			std::cout << msg;
		}
		if (log_fstream) {
			log_fstream << msg;
		}
		return *this;
	}

	static int line_number;
	static bool setLogFile(std::string f) {
		log_fstream.open(f, std::ios::out);
		if (log_fstream) {
			log_file = f;
			log_fstream.close();
			return true;
		} else {
			return false;
		}
	}
	static bool setMinLevel(int l) {
		switch (l) {
			case 0: min_level = DEBUG; break;
			case 1: min_level = INFO; break;
			case 2: min_level = WARN; break;
			case 3: min_level = ERROR; break;
			default: return false; break;
		}
		return true;
	}

private:
	LOG_LEVEL msg_level;

	static LOG_LEVEL min_level;
	static std::string log_file;
	static std::ofstream log_fstream;
	static const std::string LABELS[4];
	static const std::string COLORS[4];
};

#endif // LOG_H
