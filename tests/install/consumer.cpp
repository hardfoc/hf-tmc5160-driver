// Compiles the driver, including its template implementation and the feature headers, from an install tree.
#include "tmc51x0.hpp"
#include "tmc51x0_daisy_chain.hpp"
#include "tmc51x0_multi_node.hpp"

int main() { return tmc51x0::ChipVersion::TMC5160 == 0x30 ? 0 : 1; }
