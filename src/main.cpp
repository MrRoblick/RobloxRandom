#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cstring>
#include <bit>
#include <random/random.h>

void print_usage() {
    std::cout << "Roblox PCG Random CLI Tool\n";
    std::cout << "Usage:\n";
    std::cout << "  roblox_random crack <filename> <min> <max>\n";
    std::cout << "  roblox_random gen <method> [options]\n\n";
    std::cout << "Methods:\n";
    std::cout << "  int        Generate integers using next_integer(min, max)\n";
    std::cout << "  num        Generate floating-point numbers using next_number(min, max)\n";
    std::cout << "  vec        Generate unit vectors using next_unit_vector()\n\n";
    std::cout << "Options for 'gen':\n";
    std::cout << "  -s, --seed <value>   64-bit seed value (if omitted, uses entropy/Roblox default seed)\n";
    std::cout << "  -n, --count <value>  Number of values to output (default: 1)\n";
    std::cout << "  --min <value>        Minimum limit for int/num (default: 0 or 0.0)\n";
    std::cout << "  --max <value>        Maximum limit for int/num (default: 100 or 1.0)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "crack") {
        if (argc < 5) {
            std::cout << "Error: Missing arguments for crack mode.\n";
            std::cout << "Usage: roblox_random crack <filename> <min> <max>\n";
            return 1;
        }
        std::string filename = argv[2];
        int64_t min_val = std::stoll(argv[3]);
        int64_t max_val = std::stoll(argv[4]);

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Could not open file '" << filename << "'\n";
            return 1;
        }

        std::vector<TargetInfo> targets;
        int64_t val;
        while (file >> val) {
            targets.push_back({ val, min_val, max_val });
        }
        file.close();

        if (targets.empty()) {
            std::cout << "Error: No numbers found in file '" << filename << "'\n";
            return 1;
        }

        std::cout << "Attempting to crack seed using a sequence of " << targets.size() << " numbers from '" << filename << "'...\n";
        std::cout << "Parameters configured: min=" << min_val << ", max=" << max_val << "\n";

        auto seed = Random::find_seed_from_sequence(targets);
        if (seed.has_value()) {
            std::cout << "SUCCESS! Seed found: " << seed.value() << "\n";
        }
        else {
            std::cout << "FAILED! Seed not found or search space exhausted.\n";
        }
        return 0;
    }
    else if (mode == "gen") {
        if (argc < 3) {
            std::cout << "Error: Missing generation method.\n";
            std::cout << "Available methods: int, num, vec\n";
            return 1;
        }
        std::string method = argv[2];

        std::optional<uint64_t> seed_opt;
        int count = 1;
        int64_t min_i = 0, max_i = 100;
        double min_d = 0.0, max_d = 1.0;

        for (int i = 3; i < argc; ++i) {
            if ((std::strcmp(argv[i], "-s") == 0 || std::strcmp(argv[i], "--seed") == 0) && i + 1 < argc) {
                seed_opt = std::stoull(argv[++i]);
            }
            else if ((std::strcmp(argv[i], "-n") == 0 || std::strcmp(argv[i], "--count") == 0) && i + 1 < argc) {
                count = std::stoi(argv[++i]);
            }
            else if (std::strcmp(argv[i], "--min") == 0 && i + 1 < argc) {
                std::string val = argv[++i];
                min_i = std::stoll(val);
                min_d = std::stod(val);
            }
            else if (std::strcmp(argv[i], "--max") == 0 && i + 1 < argc) {
                std::string val = argv[++i];
                max_i = std::stoll(val);
                max_d = std::stod(val);
            }
        }

        Random* rand_gen = nullptr;
        if (seed_opt.has_value()) {
            std::cout << "Initializing generator with explicit seed: " << seed_opt.value() << "\n";
            rand_gen = new Random(seed_opt.value());
        }
        else {
            std::cout << "Initializing generator with entropy-based seed (Roblox default)...\n";
            rand_gen = new Random();
            std::cout << "Generated initial state: " << rand_gen->state << "\n";
        }

        std::cout << "Generating " << count << " outputs using method '" << method << "':\n\n";

        if (method == "int") {
            for (int c = 0; c < count; ++c) {
                std::cout << rand_gen->next_integer(min_i, max_i) << "\n";
            }
        }
        else if (method == "num") {
            for (int c = 0; c < count; ++c) {
                std::cout << rand_gen->next_number(min_d, max_d) << "\n";
            }
        }
        else if (method == "vec") {
            for (int c = 0; c < count; ++c) {
                Vector3 v = rand_gen->next_unit_vector();
                std::cout << "Vector3(" << v.x << ", " << v.y << ", " << v.z << ")\n";
            }
        }
        else {
            std::cout << "Error: Unknown generation method '" << method << "'. Use int, num, or vec.\n";
            delete rand_gen;
            return 1;
        }

        delete rand_gen;
        return 0;
    }
    else {
        std::cout << "Error: Unknown mode '" << mode << "'\n";
        print_usage();
        return 1;
    }
}