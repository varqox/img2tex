#pragma once

#include <numeric>
#include <vector>

class DSU {
	std::vector<int> p;

public:
	explicit DSU(int size) : p(size) { iota(p.begin(), p.end(), 0); }

	int size() const noexcept { return p.size(); }

	int add() {
		int x = p.size();
		p.emplace_back(x);
		return x;
	}

	int find(int x) noexcept { return (p[x] == x ? x : p[x] = find(p[x])); }

	void join(int a, int b) noexcept { p[find(a)] = find(b); }
};
