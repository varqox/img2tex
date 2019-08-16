#pragma once

#include <string>

bool has_prefix(const std::string& str, const std::string& prefix) noexcept {
	return (str.compare(0, prefix.size(), prefix) == 0);
}
