#pragma once

#include <stdexcept>
#include <string>

inline void fail_if(bool cond, const std::string& msg) {
    if (cond) throw std::runtime_error(msg);
}
