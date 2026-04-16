#pragma once

#include <fstream>
#include <string>

void ensure_runtime_log_dir();
std::string default_runtime_log_path(int rank);
void log_line(std::ofstream& f, const std::string& s);
