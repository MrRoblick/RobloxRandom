#include <print>
#include <fstream>
#include <vector>
#include <string_view>
#include <optional>
#include <algorithm>
#include <charconv>
#include <span>
#include <RobloxRandom/random.h>

template <typename T>
std::optional<T> parse_number(std::string_view str) {
    T value{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc{}) {
        return value;
    }
    return std::nullopt;
}

static void print_usage() {
    std::print(
        "Roblox PCG Random CLI Tool\n"
        "Usage:\n"
        "  roblox_random crack <filename> <min> <max>  - Crack seed using a sequence from a file\n"
        "  roblox_random single <target> <min> <max>   - Find a seed using a single target value\n"
        "  roblox_random gen <method> [options]        - Generate random numbers/vectors\n\n"
        "Methods:\n"
        "  int        Generate integers using next_integer(min, max)\n"
        "  num        Generate floating-point numbers using next_number(min, max)\n"
        "  vec        Generate unit vectors using next_unit_vector()\n\n"
        "Options for 'gen':\n"
        "  -s, --seed <value>   64-bit seed value (if omitted, uses entropy/Roblox default seed)\n"
        "  -n, --count <value>  Number of values to output (default: 1)\n"
        "  --min <value>        Minimum limit for int/num (default: 0 or 0.0)\n"
        "  --max <value>        Maximum limit for int/num (default: 100 or 1.0)\n"
    );
}

static int handle_crack(std::span<const std::string_view> args) {
    if (args.size() < 4) {
        std::println(stderr, "Error: Missing arguments for crack mode.");
        std::println(stderr, "Usage: roblox_random crack <filename> <min> <max>");
        return 1;
    }

    std::string filename{ args[1] };
    auto min_opt = parse_number<int64_t>(args[2]);
    auto max_opt = parse_number<int64_t>(args[3]);

    if (!min_opt || !max_opt) {
        std::println(stderr, "Error: Invalid min or max numerical values.");
        return 1;
    }

    int64_t min_val = *min_opt;
    int64_t max_val = *max_opt;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::println(stderr, "Error: Could not open file '{}'", filename);
        return 1;
    }

    std::vector<RobloxRandom::Structures::TargetInfo> targets;
    int64_t val;
    while (file >> val) {
        targets.push_back({ val, min_val, max_val });
    }
    file.close();

    if (targets.empty()) {
        std::println(stderr, "Error: No numbers found in file '{}'", filename);
        return 1;
    }

    std::println("Attempting to crack seed using a sequence of {} numbers from '{}'...", targets.size(), filename);
    std::println("Parameters configured: min={}, max={}", min_val, max_val);

    auto seed = RobloxRandom::Random::find_seed_from_sequence(targets);
    if (seed.has_value()) {
        std::println("SUCCESS! Seed found: {}", seed.value());
    }
    else {
        std::println("FAILED! Seed not found or search space exhausted.");
    }
    return 0;
}

static int handle_single(std::span<const std::string_view> args) {
    if (args.size() < 4) {
        std::println(stderr, "Error: Missing arguments for single mode.");
        std::println(stderr, "Usage: roblox_random single <target> <min> <max>");
        return 1;
    }

    auto target_opt = parse_number<int64_t>(args[1]);
    auto min_opt = parse_number<int64_t>(args[2]);
    auto max_opt = parse_number<int64_t>(args[3]);

    if (!target_opt || !min_opt || !max_opt) {
        std::println(stderr, "Error: Invalid numerical arguments.");
        return 1;
    }

    int64_t target = *target_opt;
    int64_t min_val = *min_opt;
    int64_t max_val = *max_opt;

    std::println("Attempting to find seed for target value {} in range [{}, {}]...", target, min_val, max_val);

    auto seed = RobloxRandom::Random::find_seed(target, min_val, max_val);
    if (seed.has_value()) {
        std::println("SUCCESS! Seed found: {}", seed.value());
        std::println("Note: Since only one number was provided, other seeds may produce the same output.");
    }
    else {
        std::println("FAILED! Seed not found. Ensure the target belongs to the given bounds.");
    }
    return 0;
}

static int handle_gen(std::span<const std::string_view> args) {
    if (args.size() < 2) {
        std::println(stderr, "Error: Missing generation method.");
        std::println(stderr, "Available methods: int, num, vec");
        return 1;
    }

    std::string_view method = args[1];

    std::optional<uint64_t> seed_opt;
    int count = 1;
    int64_t min_i = 0, max_i = 100;
    double min_d = 0.0, max_d = 1.0;

    for (size_t i = 2; i < args.size(); ++i) {
        if ((args[i] == "-s" || args[i] == "--seed") && i + 1 < args.size()) {
            seed_opt = parse_number<uint64_t>(args[++i]);
        }
        else if ((args[i] == "-n" || args[i] == "--count") && i + 1 < args.size()) {
            if (auto parsed = parse_number<int>(args[++i])) {
                count = *parsed;
            }
        }
        else if (args[i] == "--min" && i + 1 < args.size()) {
            std::string_view val = args[++i];
            if (auto parsed_i = parse_number<int64_t>(val)) min_i = *parsed_i;
            if (auto parsed_d = parse_number<double>(val)) min_d = *parsed_d;
        }
        else if (args[i] == "--max" && i + 1 < args.size()) {
            std::string_view val = args[++i];
            if (auto parsed_i = parse_number<int64_t>(val)) max_i = *parsed_i;
            if (auto parsed_d = parse_number<double>(val)) max_d = *parsed_d;
        }
    }

    std::optional<RobloxRandom::Random> rand_gen;
    if (seed_opt.has_value()) {
        std::println("Initializing generator with explicit seed: {}", *seed_opt);
        rand_gen.emplace(*seed_opt);
    }
    else {
        std::println("Initializing generator with entropy-based seed (Roblox default)...");
        rand_gen.emplace();
        std::println("Generated initial state: {}", rand_gen->state);
    }

    std::println("Generating {} outputs using method '{}':\n", count, method);

    if (method == "int") {
        for (int c = 0; c < count; ++c) {
            std::println("{}", rand_gen->next_integer(min_i, max_i));
        }
    }
    else if (method == "num") {
        for (int c = 0; c < count; ++c) {
            std::println("{}", rand_gen->next_number(min_d, max_d));
        }
    }
    else if (method == "vec") {
        for (int c = 0; c < count; ++c) {
            auto v = rand_gen->next_unit_vector();
            std::println("Vector3({}, {}, {})", v.x, v.y, v.z);
        }
    }
    else {
        std::println(stderr, "Error: Unknown generation method '{}'. Use int, num, or vec.", method);
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args(argv, argv + argc);

    if (args.size() < 2) {
        print_usage();
        return 1;
    }

    std::string_view mode = args[1];

    std::span<const std::string_view> sub_args(args.begin() + 1, args.end());

    if (mode == "crack") {
        return handle_crack(sub_args);
    }
    else if (mode == "single") {
        return handle_single(sub_args);
    }
    else if (mode == "gen") {
        return handle_gen(sub_args);
    }
    else {
        std::println(stderr, "Error: Unknown mode '{}'", mode);
        print_usage();
        return 1;
    }
}