#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include "log.h"

bool parse_args(int argc, char* argv[], std::string &src_file);
void show_usage(std::string prog_name);
void welcome_msg();

int main(int argc, char* argv[]) {
	welcome_msg();
	std::string src_file;
	if (!parse_args(argc, argv, src_file)) return 1;
	LOG(DEBUG) << "Begin scanning file: " << src_file;

	//// Test the levels
	//LOG(DEBUG) << "Test debug";
	//LOG(INFO) << "Test info";
	//LOG(WARN) << "Test warning";
	//LOG(ERROR) << "Test error";
	return 0;
}

bool parse_args(int argc, char* argv[], std::string &src_file) {
	int opt;
	bool error = false;
	while ((opt = getopt(argc, argv, "hi:v:")) != -1) {
		switch (opt) {
			case 'h':
				error = true;
				break;
			case 'i': {
					src_file = optarg;
					std::ifstream src_fstream(src_file);
					if (!src_fstream.good()) {
						LOG(ERROR) << "Invalid file: " << src_file;
						LOG(ERROR) << "Make sure it exists and you have read"
							<< " permissions.";
						error = true;
					}
					break;
				}
			case 'v':
				// setMinLevel handles the bound checking
				if (!LOG::setMinLevel(std::atoi(optarg))) {
					LOG(ERROR) << "Could not set verbosity level.";
					LOG(ERROR) << "Pass an integer from 0-3.";
					error = true;
				}
				break;
			case '?':
				LOG(ERROR) << "Invalid flag: " << (char)optopt;
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

