#include "logger.hpp"

#include <cstdlib>
#include <sstream>
#include <string>

void ensure_runtime_log_dir() {
    std::system("mkdir -p outputs/logs");
}

std::string default_runtime_log_path(int rank) {
    std::ostringstream o;
    o << "outputs/logs/runtime_rank" << rank << ".log";
    return o.str();
}

void log_line(std::ofstream& f, const std::string& s) {
    f << s << '\n';
    f.flush();
}
