#include "tt.h"
#include <cstring>
#include <cstdint>


void tt::clear()
{
	std::memset(_tt, 0, sizeof(_tt));
}

void tt::store(const tt_entry& entry, bool always_replace)
{
	// https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
	const int index = ((uint32_t)entry.posKey * (uint64_t)size) >> 32;

	// look if we can replace any element in the current index

	// replace the same position, or replace based on depth
	// higher depth elements are more important, replace based on that
	// the same position replacement is mostly to guarantee we have the rootPos stored at the end of the serach, no matter what
	// why is it the first condition when it's the least likely? don't ask
	if((_tt[index].posKey == entry.posKey) || (_tt[index].depth <= entry.depth) || always_replace)
	{
		_tt[index] = entry;
		return;
	}
}

const tt_entry& tt::get(const tt_entry& entry, bool& found) const
{
	return this->get(entry.posKey, found);
} 

const tt_entry& tt::get(unsigned long long posKey, bool& found) const
{
	// https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
	const int index = ((uint32_t)posKey * (uint64_t)size) >> 32;
	// return some element, mark whether it is what we were looking for
	found = _tt[index].posKey == posKey;
	return _tt[index];
}