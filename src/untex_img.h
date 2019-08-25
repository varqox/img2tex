#pragma once

#include "symbol_database.h"

#include <string>
#include <variant>
#include <vector>

struct UntexFailure {
	std::vector<SplitSymbol> unmatched_symbol_candidates;
};

std::variant<std::string, UntexFailure>
untex_img(const Matrix<int>& img, const SymbolDatabase& symbol_database,
          bool be_verbose);
