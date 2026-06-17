#include <iostream>
#include <cstdint>
#include <vector>
#include <random/random.h>


int main()
{
    int64_t my_min = 10;
    int64_t my_max = 50;
    uint64_t original_seed = 3344556767;

    Random rand{ original_seed };
    std::vector<int64_t> generated_sequence;

    std::cout << "Generated sequence: ";
    for (int i = 0; i < 100; ++i) {
        int64_t val = rand.next_integer(my_min, my_max);
        generated_sequence.push_back(val);
        std::cout << val << " ";
    }
    std::cout << "\n---------------------\n";

    std::cout << "Cracking seed... Please wait...\n";
    auto guessed_seed = Random::find_seed_from_sequence(generated_sequence, my_min, my_max);

    if (guessed_seed.has_value()) {
        std::cout << "Success! Found seed: " << guessed_seed.value() << "\n";
        if (guessed_seed.value() == original_seed) {
            std::cout << "The guessed seed matches the original one perfectly!\n";
        }
    }
    else {
        std::cout << "Failed to find seed (perhaps it's a full 64-bit seed).\n";
    }

    return 0;
}
