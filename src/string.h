#pragma once

#include <string_view>

inline bool has_prefix(const std::string_view& str,
                       const std::string_view& prefix) noexcept {
	return (str.compare(0, prefix.size(), prefix) == 0);
}

inline bool has_suffix(const std::string_view& str,
                       const std::string_view& suffix) noexcept {
	return (str.size() >= suffix.size() and
	        str.compare(str.size() - suffix.size(), suffix.size(), suffix) ==
	           0);
}
