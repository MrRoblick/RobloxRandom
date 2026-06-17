#include <iostream>
#include <cstdint>
#include <vector>
#include <random/random.h>


int main()
{
	Random rand{ 3344556767 };
	std::vector<int> data{ 4,8,9,1,0,100,300,500,200 };
	rand.shuffle(data);
	for (auto v : data) {
		std::cout << v << '\n';
	}
	return 0;
}
