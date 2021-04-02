#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "log.h"
#include "token.h"
#include "parser.h"

bool parse_args(int argc, char* argv[], std::string &src_file,
		std::string &log_file, bool &show_welcome);
void show_usage(std::string prog_name);
void welcome_msg();

int main(int argc, char* argv[]) {
	// Set up, parse args, etc
	std::string src_file, log_file;
	bool show_welcome = true;
	if (!parse_args(argc, argv, src_file, log_file, show_welcome)) {
		exit(EXIT_FAILURE);
	}
	if (show_welcome) welcome_msg();
	LOG(INFO) << "Begin compiling file: " << src_file;

	// Set up parser
	Parser parser;
	if (!parser.init(src_file)) {
		exit(EXIT_FAILURE);
	}

	// Parse the file
	if (parser.parse()) {
		exit(EXIT_SUCCESS);
	} else {
		exit(EXIT_FAILURE);
	}
}

bool parse_args(int argc, char* argv[], std::string &src_file,
		std::string &log_file, bool &show_welcome) {
	int opt;
	bool error = false;
	while ((opt = getopt(argc, argv, "hv:i:l:w")) != -1) {
		switch (opt) {
			case 'h':
				error = true;
				break;
			case 'v':
				// setMinLevel handles the bound checking
				if (!LOG::setMinLevel(std::atoi(optarg))) {
					LOG(ERROR) << "Could not set verbosity level";
					LOG(ERROR) << "Pass an integer from 0-3";
					error = true;
				}
				break;
			case 'i':
				src_file = optarg;
				break;
			case 'l':
				if (!LOG::setLogFile(optarg)) {
					LOG(ERROR) << "Cannot open file for write: " << optarg;
					error = true;
				}
				log_file = optarg;
				break;
			case 'w':
				show_welcome = false;
				break;
			case '?':
				LOG(ERROR) << "Invalid flag: " << static_cast<char>(optopt);
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
				<< "\t-w\t\tDo not show welcome Tux.\n"
				<< "\t\t\tThis will make Tux sad. :(\n"
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
