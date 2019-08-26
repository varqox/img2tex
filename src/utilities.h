#pragma once

#include <algorithm>
#include <iterator>

template <class A, class... Option>
constexpr bool is_one_of(A&& val, Option&&... option) {
	return ((val == option) or ...);
}

template <class T>
constexpr bool is_sorted(T&& collection) {
	auto it = std::begin(collection);
	auto end = std::end(collection);
	if (it == end)
		return true;

	auto last = it;
	while (++it != end) {
		if (*last > *it)
			return false;

		last = it;
	}

	return true;
}

template <class T, class V>
constexpr bool contains(T&& collection, V&& val) {
	for (auto const& elem : collection) {
		if (elem == val)
			return true;
	}

	return false;
}

template <class T, class V>
constexpr bool binary_search(T&& collection, V&& val) {
	return std::binary_search(
	   std::begin(collection), std::end(collection), std::forward<V>(val));
}

template <class... T>
struct overloaded : T... {
	using T::operator()...;
};

template <class... T>
overloaded(T...)->overloaded<T...>;
