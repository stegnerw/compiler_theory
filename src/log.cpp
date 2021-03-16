#include "log.h"

#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// Public
////////////////////////////////////////////////////////////////////////////////

std::stringstream LOG::ss;
int LOG::line_number = 0;

LOG::LOG(LOG_LEVEL type) : msg_level(type) {
	if (!ss.str().empty()) {
		if (msg_level >= min_level) {
			std::cout << COLORS[type] << LABELS[type] << "Line " << line_number
				<< std::endl << "\t" << ss.str() << COL_RST << std::endl;
		}
		log_fstream.open(log_file, std::ios::app);
		if (log_fstream) {
			log_fstream << LABELS[type] << "Line " << line_number << std::endl
				<< "\t" << ss.str() << std::endl;
			log_fstream.close();
		}
		ss.str(std::string());
	}
}

bool LOG::setLogFile(std::string f) {
	// Try opening the file
	log_fstream.open(f, std::ios::out);
	if (log_fstream) {
		log_file = f;
		log_fstream.close();
		return true;
	} else {
		return false;
	}
}

bool LOG::setMinLevel(int l) {
	switch (l) {
		case 0: min_level = DEBUG; break;
		case 1: min_level = INFO; break;
		case 2: min_level = WARN; break;
		case 3: min_level = ERROR; break;
		default: return false; break;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Private
////////////////////////////////////////////////////////////////////////////////

LOG_LEVEL LOG::min_level = INFO;
std::string LOG::log_file;
std::ofstream LOG::log_fstream;
const std::string LOG::LABELS[4] = {"[DEBUG]\t", "[INFO ]\t", "[WARN ]\t",
		"[ERROR]\t"};
const std::string LOG::COLORS[4] = {COL_CYN, COL_WHT, COL_YEL, COL_RED};

