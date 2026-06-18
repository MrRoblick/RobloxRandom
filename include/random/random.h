#pragma once
#include <atomic>
#include <cstdint>
#include <optional>
#include <vector>

struct Vector3 {
    float x;
    float y;
    float z;
};

struct TargetInfo {
    int64_t target;
    int64_t min_val;
    int64_t max_val;
};

class Random {
private:
    static constexpr uint64_t PCG_MULTIPLIER = 0x5851F42D4C957F2DULL;
    static constexpr uint64_t PCG_INCREMENT = 105ULL;

    static std::atomic<uint64_t> global_entropy_seed;
    static std::atomic<uint64_t> global_entropy_seed_value;
    static std::atomic<uint32_t> entropy_counter;
    static std::atomic<uint8_t>  entropy_mutex_lock;

    static constexpr bool USE_ALT_FLOAT_ARITHMETIC = false;
    static constexpr int  FLOAT_ARITHMETIC_MASK = 0;

    static uint64_t generate_roblox_default_seed();
    static uint64_t entropy_seed();
    static void safe_gen_entropy_seed();
    static uint64_t hash_seed(std::atomic<uint64_t>& entropy_seed_value_ptr);

    static uint64_t get_nanoseconds_monotonic();
    static uint64_t get_microseconds_realtime();
    static uint64_t get_nanoseconds_float();
    static uint64_t get_current_pid();
    static uint64_t get_current_thread_id();

    double next_double();
public:
    uint64_t state{ 0 };

    Random();
    Random(uint64_t seed);

    uint64_t next_integer(int64_t min_val, int64_t max_val);
    double next_number();
    double next_number(double min_val, double max_val);
    Vector3 next_unit_vector();

    template <typename Container>
    void shuffle(Container& container) {
        auto size = std::size(container);
        if (size <= 1) return;
        for (auto i = size; i >= 2; --i) {
            auto j = next_integer(1, i);
            using std::swap;
            swap(container[j - 1], container[i - 1]);
        }
    }

    static std::optional<uint64_t> find_seed(int64_t target, int64_t min_val, int64_t max_val);
    static std::optional<uint64_t> find_seed_from_sequence(const std::vector<int64_t>& sequence, int64_t min_val, int64_t max_val);
    static std::optional<uint64_t> find_seed_from_sequence(const std::vector<TargetInfo>& targets);
};