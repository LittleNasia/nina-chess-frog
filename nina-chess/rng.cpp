#include "rng.h"

#include <intrin.h>
#include <limits>


rng::result_type rng::max()
{
	return std::numeric_limits<result_type>::max();
}

rng::result_type rng::min()
{
	return std::numeric_limits<result_type>::min();
}

rng::rng() : rng_state{ 0xabcdabcd ^ __rdtsc() }
{
	_rdseed64_step(&rng_state);
	rng_state ^= __rdtsc();
}

rng::result_type rng::operator()()
{
	uint64_t x = rng_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return rng_state = x;
}

color rng::random_color()
{
	// get an rng value, return 0 (white) or 1 (black)
	return this->operator()() & 1;
}

bool rng::random_roll(int chance)
{
	return (this->operator()() & 255) <= chance;
}
