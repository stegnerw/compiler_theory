/*
 * This source file really only initializes static members since that cannot be
 * done inline
 */
#include "log.h"

#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// Public
////////////////////////////////////////////////////////////////////////////////

int LOG::line_number = 0;

////////////////////////////////////////////////////////////////////////////////
// Private
////////////////////////////////////////////////////////////////////////////////

bool LOG::has_errored = false;
LOG_LEVEL LOG::min_level = INFO;
std::string LOG::log_file;
std::ofstream LOG::log_fstream;
const std::string LOG::LABELS[4] = {"[DEBUG]\t", "[INFO ]\t", "[WARN ]\t",
    "[ERROR]\t"};
const std::string LOG::COLORS[4] = {COL_CYN, COL_WHT, COL_YEL, COL_RED};
