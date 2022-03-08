#include "evaluate.h"
#include "nn.h"


value evaluate(const board& b)
{
#if use_nn
	return nn::network.evaluate_board(b) * 1000 * ((b.get_side_to_move() == WHITE)?1:-1);
#endif
	return 0;
}