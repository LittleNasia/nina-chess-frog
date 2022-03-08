#pragma once
#include <cstdint>

#include "utils.h"

struct tt_entry
{
	static constexpr int flag_exact = 0;
	static constexpr int flag_alpha = 1;
	static constexpr int flag_beta = 2;
	unsigned long long posKey;
	move move;
	int score;
	int depth;
	int flag;	
};



class tt
{
public:
	constexpr static unsigned int size = 2<<21;
	void clear();
	void store(const tt_entry& entry, bool always_replace = false);
	const tt_entry& get(const tt_entry& entry, bool& found) const;
	const tt_entry& get(unsigned long long posKey, bool& found) const;
private:
	tt_entry _tt[size];
};
