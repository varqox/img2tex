#pragma once

#include <string_view>

inline bool has_prefix(const std::string_view& str,
                       const std::string_view& prefix) noexcept {
	return (str.compare(0, prefix.size(), prefix) == 0);
}

template <class... T>
inline bool has_one_of_prefixes(const std::string_view& str,
                                T&&... prefixes) noexcept {
	return (... or has_prefix(str, std::forward<T>(prefixes)));
}

inline bool has_suffix(const std::string_view& str,
                       const std::string_view& suffix) noexcept {
	return (str.size() >= suffix.size() and
	        str.compare(str.size() - suffix.size(), suffix.size(), suffix) ==
	           0);
}

template <class... T>
inline bool has_one_of_suffixes(const std::string_view& str,
                                T&&... suffixes) noexcept {
	return (... or has_suffix(str, std::forward<T>(suffixes)));
}
