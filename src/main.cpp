#include <iostream>
#include <cstdint>
#include <random/random.h>

int main()
{
	Random rand{ 3344556767 };

	for (int i = 0; i < 10; i++) {
		std::cout << rand.next_number(0.0, 1000.0) << '\n';
	}

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
