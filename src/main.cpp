#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include "log.h"
#include "token.h"
#include "scanner.h"

bool parse_args(int argc, char* argv[], std::string &src_file,
		std::string &log_file);
void show_usage(std::string prog_name);
void welcome_msg();

int main(int argc, char* argv[]) {
	welcome_msg();
	std::string src_file, log_file;
	if (!parse_args(argc, argv, src_file, log_file)) return 1;
	LOG::ss << "Begin scanning file: " << src_file;
	LOG(LOG_LEVEL::DEBUG);
	Scanner scanner;
	if (!scanner.init(src_file)) return 1;
	Token* t(NULL);
	bool scanning = true;
	while (scanning) {
		t = scanner.getToken();
		LOG::ss << "New token:\t" << t->getStr();
		LOG(LOG_LEVEL::DEBUG);
		if (t->getType() == TOK_EOF) scanning = false;
		delete t;
		t = NULL;
	}
	return 0;
}

bool parse_args(int argc, char* argv[], std::string &src_file,
		std::string &log_file) {
	int opt;
	bool error = false;
	while ((opt = getopt(argc, argv, "hv:i:l:")) != -1) {
		switch (opt) {
			case 'h':
				error = true;
				break;
			case 'v':
				// setMinLevel handles the bound checking
				if (!LOG::setMinLevel(std::atoi(optarg))) {
					LOG::ss << "Could not set verbosity level.";
					LOG(LOG_LEVEL::ERROR);
					LOG::ss << "Pass an integer from 0-3.";
					LOG(LOG_LEVEL::ERROR);
					error = true;
				}
				break;
			case 'i':
				src_file = optarg;
				break;
			case 'l':
				if (!LOG::setLogFile(optarg)) {
					LOG::ss << "Cannot open file for write: " << optarg;
					LOG(LOG_LEVEL::ERROR);
					error = true;
				}
				log_file = optarg;
				break;
			case '?':
				LOG::ss << "Invalid flag: " << static_cast<char>(optopt);
				LOG(LOG_LEVEL::ERROR);
				error = true;
				break;
		}
		if (error) {
			show_usage(argv[0]);
			break;
		}
	}
	return !error;
}

void show_usage(std::string prog_name) {
	std::cerr	<< "Usage: " << prog_name << " [options]\n"
				<< "Please be gentle; I did not rigorously test arg parsing.\n"
				<< "Options:\n"
				<< "\t-h\t\tShow this help message\n"
				<< "\t-i INFILE\tSpecify input file to compile\n"
				<< "\t-v LEVEL\tSpecify verbosity level (default 2):\n"
				<< "\t\t\t0 - DEBUG\n"
				<< "\t\t\t1 - INFO\n"
				<< "\t\t\t2 - WARNING\n"
				<< "\t\t\t3 - ERROR\n"
				<< std::endl;
}

void welcome_msg(){
	std::cout
	<< " ______________________________\n"
	<< "< Compiler? I hardly know her! >\n"
	<< " ------------------------------\n"
	<< "  \\\n"
	<< "   \\\n"
	<< "       .--.\n"
	<< "      |o_o |\n"
	<< "      |:_/ |\n"
	<< "     //   \\ \\\n"
	<< "    (|     | )\n"
	<< "   /'\\_   _/`\\\n"
	<< "   \\___)=(___/"
	<< std::endl;
}

