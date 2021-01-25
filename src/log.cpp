#include "log.h"
#include <string>
#include <iostream>

loglevel LOG::min_level = INFO;
 const std::string labels[4] = {"DEBUG", "INFO ", "WARN ", "ERROR"};
LOG::LOG(loglevel type) {
	msg_level = type;
	std::cout << colors[type];
	operator << ("["+labels[type]+"]\t");
}

LOG::~LOG() {
	if(written) {
		cout << endl << COL_RST;
	}
	written = false;
}

template<class T>
LOG &LOG::operator<<(const T &msg) {
	if(msg_level >= min_level) {
		cout << msg;
		written = true;
	}
	return *this;
}

/*
 * Set min_level from an int
 * Also does range checking
 */
bool LOG::setMinLevel(int l) {
	switch (l) {
		case 0:		min_level = DEBUG;	break;
		case 1:		min_level = INFO;	break;
		case 2:		min_level = WARN;	break;
		case 3:		min_level = ERROR;	break;
		default:	return false;		break;
	}
	return true;
}

