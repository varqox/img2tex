#include "symbols_to_tex.h"

#include <regex>

using std::regex;
using std::regex_replace;
using std::reverse;
using std::string;
using std::vector;

static string space_digits_into_3digit_groups(string&& tex) {
	string res;
	int digits_count = 0;
	for (auto it = tex.rbegin(); it != tex.rend(); ++it) {
		char c = *it;
		if (not isdigit(c)) {
			digits_count = 0;
		} else if (++digits_count == 4) {
			digits_count = 1;
			res += ",\\";
		}

		res += c;
	}

	reverse(res.begin(), res.end());
	return res;
}

#include <iostream>

string symbols_to_tex(const vector<string>& symbols) {
	string res;
	for (const string& symbol : symbols)
		res += symbol, res += ' ';
	res.pop_back();

	std::cerr << res << '\n';
	// Remove spaces between letters and digits
	for (;;) {
		// Due to the lack of the lookbehind, we need to repeat this (at most
		// log res.size() times)
		auto len = res.size();
		res = regex_replace(
		   res, regex(R"#((^|\s)([A-Za-z0-9]+)\s(?=[A-Za-z0-9]))#"), "$1$2");
		if (res.size() == len)
			break;
	}
	std::cerr << res << '\n';

	// Remove space before ,
	res = regex_replace(res, regex(" ?,"), ",");
	std::cerr << res << '\n';
	// Remove space before )
	res = regex_replace(res, regex(" ?\\)"), ")");
	std::cerr << res << '\n';
	// Remove space after (
	res = regex_replace(res, regex("\\( ?"), "(");
	std::cerr << res << '\n';
	// Remove space before ( that is a function call
	res = regex_replace(res, regex(R"#((^|\s)(\w+) \()#"), "$1$2(");
	std::cerr << res << '\n';

	res = space_digits_into_3digit_groups(std::move(res));

	return res;
}
