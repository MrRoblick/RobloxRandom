#include <iostream>
#include <cstdint>
#include <random/random.h>

int main()
{
	auto seed = Random::find_seed(6767, 0, 14888);
	if (seed.has_value()) {
		std::cout << "Seed found: " << *seed << '\n';
		return 0;
	}
	else {
		std::cout << "Seed not found\n";
		return 1;
	}
	return 0;
}
