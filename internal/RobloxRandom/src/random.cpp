#include <RobloxRandom/random.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

std::atomic<uint64_t> RobloxRandom::Random::global_entropy_seed{ 0 };
std::atomic<uint64_t> RobloxRandom::Random::global_entropy_seed_value{ 0 };
std::atomic<uint32_t> RobloxRandom::Random::entropy_counter{ 0 };
std::atomic<uint8_t>  RobloxRandom::Random::entropy_mutex_lock{ 0 };


uint64_t RobloxRandom::Random::get_nanoseconds_monotonic() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

uint64_t RobloxRandom::Random::get_microseconds_realtime() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

uint64_t RobloxRandom::Random::get_nanoseconds_float() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000LL;

    if constexpr (USE_ALT_FLOAT_ARITHMETIC && (FLOAT_ARITHMETIC_MASK & 1) != 0) {
        return static_cast<uint64_t>(nanoseconds) + 1000000000ULL * seconds;
    }
    else {
        double sec_part = static_cast<double>(static_cast<int>(seconds)) * 1000000000.0;
        double nsec_part = static_cast<double>(static_cast<int>(nanoseconds));
        return static_cast<uint32_t>(static_cast<int>(nsec_part + sec_part));
    }
}

uint64_t RobloxRandom::Random::get_current_pid() {
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<uint64_t>(GetCurrentProcessId());
#else
    return static_cast<uint64_t>(getpid());
#endif
}

uint64_t RobloxRandom::Random::get_current_thread_id() {
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<uint64_t>(GetCurrentThreadId());
#else
    return reinterpret_cast<uint64_t>(pthread_self());
#endif
}

uint64_t RobloxRandom::Random::entropy_seed() {
    uint64_t base_seed = global_entropy_seed.load(std::memory_order_relaxed);
    if (!base_seed) {
        uint64_t mono_ns_contrib = 7ULL * get_nanoseconds_monotonic();
        uint64_t real_us_contrib = 97ULL * get_microseconds_realtime();
        uint64_t pid_contrib = 997ULL * get_current_pid();
        uint64_t tid_contrib = 1997ULL * get_current_thread_id();
        uint64_t float_ns_contrib = 19997ULL * get_nanoseconds_float();

        uintptr_t stack_addr = reinterpret_cast<uintptr_t>(&base_seed);
        uintptr_t global_addr = reinterpret_cast<uintptr_t>(&global_entropy_seed);

        base_seed = tid_contrib
            ^ float_ns_contrib
            ^ pid_contrib
            ^ real_us_contrib
            ^ mono_ns_contrib
            ^ static_cast<uint64_t>(stack_addr)
            ^ (static_cast<uint64_t>(global_addr) << 32);
    }

    uint32_t current_counter = entropy_counter.fetch_add(1, std::memory_order_relaxed);
    uint64_t mixed_entropy = base_seed ^ static_cast<uint64_t>(2 * current_counter);

    uint64_t pcg_state = PCG_MULTIPLIER * mixed_entropy + PCG_INCREMENT;
    global_entropy_seed.store(PCG_MULTIPLIER * pcg_state + PCG_INCREMENT, std::memory_order_relaxed);

    uint32_t pcg_high = std::rotr<uint32_t>(static_cast<uint32_t>((pcg_state >> 27) ^ (pcg_state >> 45)), static_cast<int>(pcg_state >> 59));
    uint32_t pcg_low = std::rotr<uint32_t>(static_cast<uint32_t>((mixed_entropy >> 27) ^ (base_seed >> 45)), static_cast<int>(base_seed >> 59));

    return (static_cast<uint64_t>(pcg_high) << 32) | pcg_low;
}

void RobloxRandom::Random::safe_gen_entropy_seed() {
    uint8_t expected = 0;
    if (entropy_mutex_lock.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        global_entropy_seed_value.store(entropy_seed(), std::memory_order_release);
        entropy_mutex_lock.store(0, std::memory_order_release);
    }
}

uint64_t RobloxRandom::Random::hash_seed(std::atomic<uint64_t>& entropy_seed_value_ptr) {
    uint64_t initial_seed = entropy_seed_value_ptr.load(std::memory_order_relaxed);
    uint64_t next_seed_step = PCG_MULTIPLIER * initial_seed + PCG_INCREMENT;

    while (true) {
        uint64_t expected = initial_seed;
        if (entropy_seed_value_ptr.compare_exchange_strong(expected, next_seed_step, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            break;
        }
        initial_seed = expected;
        next_seed_step = PCG_MULTIPLIER * initial_seed + PCG_INCREMENT;
    }

    uint64_t final_seed = entropy_seed_value_ptr.load(std::memory_order_relaxed);
    uint64_t final_next_step = PCG_MULTIPLIER * final_seed + PCG_INCREMENT;

    while (true) {
        uint64_t expected = final_seed;
        if (entropy_seed_value_ptr.compare_exchange_strong(expected, final_next_step, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            break;
        }
        final_seed = expected;
        final_next_step = PCG_MULTIPLIER * final_seed + PCG_INCREMENT;
    }

    uint32_t pcg_high = std::rotr<uint32_t>(static_cast<uint32_t>((final_seed >> 27) ^ (final_seed >> 45)), static_cast<int>(final_seed >> 59));
    uint32_t pcg_low = std::rotr<uint32_t>(static_cast<uint32_t>((initial_seed >> 27) ^ (initial_seed >> 45)), static_cast<int>(initial_seed >> 59));

    return (static_cast<uint64_t>(pcg_high) << 32) | pcg_low;
}

uint64_t RobloxRandom::Random::generate_roblox_default_seed() {
    safe_gen_entropy_seed();
    uint64_t hashed_entropy = hash_seed(global_entropy_seed_value);
    return PCG_MULTIPLIER * hashed_entropy + 0x399D2694695129DELL;
}


RobloxRandom::Random::Random() {
    state = generate_roblox_default_seed();
}

RobloxRandom::Random::Random(uint64_t seed) {
    uint64_t temp_state = 0;
    temp_state = temp_state * PCG_MULTIPLIER + PCG_INCREMENT;
    temp_state += seed;
    temp_state = temp_state * PCG_MULTIPLIER + PCG_INCREMENT;
    state = temp_state;
}

uint64_t RobloxRandom::Random::next_integer(int64_t min_val, int64_t max_val) {
    uint64_t clean_min = (std::min)(static_cast<uint64_t>(min_val), static_cast<uint64_t>(max_val));
    uint64_t clean_max = (std::max)(static_cast<uint64_t>(min_val), static_cast<uint64_t>(max_val));
    uint64_t range = clean_max - clean_min;

    uint64_t next_state_step = PCG_MULTIPLIER * state + PCG_INCREMENT;
    uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((state >> 27) ^ (state >> 45)), static_cast<int>(state >> 59));

    if ((range >> 1) > 0x7FFFFFFE) {
        uint32_t rand_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state_step >> 27) ^ (next_state_step >> 45)), static_cast<int>(next_state_step >> 59));
        state = PCG_MULTIPLIER * next_state_step + PCG_INCREMENT;
        uint64_t total_values = range + 1;

        if (total_values == 0) {
            return rand_low | (static_cast<uint64_t>(rand_high) << 32);
        }
        else {
            uint32_t total_values_lo = static_cast<uint32_t>(total_values);
            uint32_t total_values_hi = static_cast<uint32_t>(total_values >> 32);

            uint64_t term1 = (static_cast<uint64_t>(rand_high) * total_values_lo) >> 32;
            uint64_t term2 = static_cast<uint64_t>(rand_high) * total_values_hi;
            uint64_t term3 = (static_cast<uint64_t>(rand_low) * total_values_hi) >> 32;

            return term1 + term2 + clean_min + term3;
        }
    }
    else {
        state = next_state_step;
        return clean_min + ((static_cast<uint64_t>(rand_low) * (range + 1)) >> 32);
    }
}

double RobloxRandom::Random::next_double() {
    uint64_t current_state = state;

    uint64_t next_state = current_state * PCG_MULTIPLIER + PCG_INCREMENT;
    state = next_state * PCG_MULTIPLIER + PCG_INCREMENT;

    uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((current_state >> 27) ^ (current_state >> 45)), static_cast<int>(current_state >> 59));
    uint32_t rand_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state >> 27) ^ (next_state >> 45)), static_cast<int>(next_state >> 59));

    uint64_t rand64 = (static_cast<uint64_t>(rand_high) << 32) | rand_low;

    return std::ldexp(static_cast<double>(rand64), -64);
}

double RobloxRandom::Random::next_number() {
    return next_double();
}

double RobloxRandom::Random::next_number(double min_val, double max_val) {
    double clean_min = (std::min)(min_val, max_val);
    double clean_max = (std::max)(min_val, max_val);

    double progress = next_double();
    double result = progress * clean_max + (1.0 - progress) * clean_min;

    if (result < clean_min) return clean_min;
    if (result > clean_max) return clean_max;

    return result;
}

RobloxRandom::Structures::Vector3 RobloxRandom::Random::next_unit_vector() {
    uint64_t current_state = state;

    uint64_t next_state = current_state * PCG_MULTIPLIER + PCG_INCREMENT;
    state = next_state * PCG_MULTIPLIER + PCG_INCREMENT;

    uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((current_state >> 27) ^ (current_state >> 45)), static_cast<int>(current_state >> 59));
    uint32_t rand_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state >> 27) ^ (next_state >> 45)), static_cast<int>(next_state >> 59));

    double angle_fraction = std::ldexp(static_cast<double>(rand_low), -32);
    double height_fraction = std::ldexp(static_cast<double>(rand_high), -32);

    float azimuth_angle = static_cast<float>(angle_fraction * 6.283185307179586);
    float z_coord = static_cast<float>(height_fraction * 2.0 - 1.0);
    float radius_xy = std::sqrt(1.0f - z_coord * z_coord);

    return RobloxRandom::Structures::Vector3{
        std::cos(azimuth_angle) * radius_xy,
        std::sin(azimuth_angle) * radius_xy,
        z_coord
    };
}


#ifdef ROBLOX_RANDOM_ENABLE_SEED_FINDER
std::optional<uint64_t> RobloxRandom::Random::find_seed(int64_t target, int64_t min_val, int64_t max_val) {
    uint64_t range = max_val - min_val;

    if ((range >> 1) > 0x7FFFFFFE) {
        return std::nullopt;
    }

    uint64_t total_values = range + 1;
    uint64_t target_offset = target - min_val;

    uint64_t target_rand_low = (target_offset * (1ULL << 32) + total_values - 1) / total_values;

    auto unxor_xsh_rr = [](uint32_t val) -> uint32_t {
        uint32_t x_high = val & 0xFFFFC000;
        uint32_t x_low = (val ^ (x_high >> 18)) & 0x3FFF;
        return x_high | x_low;
        };

    uint32_t unxored_val = unxor_xsh_rr(static_cast<uint32_t>(target_rand_low));
    uint64_t state_base = static_cast<uint64_t>(unxored_val) << 27;

    constexpr uint64_t M_INV = 0xC097EF87329E28A5ULL;

    for (uint64_t low_bits = 0; low_bits < (1ULL << 27); ++low_bits) {
        uint64_t candidate_state = state_base | low_bits;
        uint64_t seed = ((candidate_state - PCG_INCREMENT) * M_INV - PCG_INCREMENT);

        if (seed < 9007199254740992ULL) {
            return seed;
        }
    }

    return std::nullopt;
}

std::optional<uint64_t> RobloxRandom::Random::find_seed_from_sequence(const std::vector<int64_t>& sequence, int64_t min_val, int64_t max_val) {
    if (sequence.empty()) return std::nullopt;

    uint64_t clean_min = (std::min)(static_cast<uint64_t>(min_val), static_cast<uint64_t>(max_val));
    uint64_t clean_max = (std::max)(static_cast<uint64_t>(min_val), static_cast<uint64_t>(max_val));
    uint64_t range = clean_max - clean_min;

    if (range == 0) return std::nullopt;

    bool is_large_range = ((range >> 1) > 0x7FFFFFFE);
    int64_t first_target = sequence[0];

    for (uint64_t seed = 0; seed <= 0xFFFFFFFFULL; ++seed) {

        uint64_t current_state = (105ULL + seed) * PCG_MULTIPLIER + PCG_INCREMENT;
        int64_t generated;

        if (is_large_range) {
            RobloxRandom::Random test_rand(seed);
            generated = test_rand.next_integer(min_val, max_val);
            if (generated == first_target) {
                bool match = true;
                for (size_t i = 1; i < sequence.size(); ++i) {
                    if (test_rand.next_integer(min_val, max_val) != sequence[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) return seed;
            }
        }
        else {
            uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((current_state >> 27) ^ (current_state >> 45)), static_cast<int>(current_state >> 59));
            generated = clean_min + ((static_cast<uint64_t>(rand_low) * (range + 1)) >> 32);

            if (generated == first_target) {
                RobloxRandom::Random test_rand(seed);
                test_rand.next_integer(min_val, max_val);

                bool match = true;
                for (size_t i = 1; i < sequence.size(); ++i) {
                    if (test_rand.next_integer(min_val, max_val) != sequence[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) return seed;
            }
        }
    }
    return std::nullopt;
}

std::optional<uint64_t> RobloxRandom::Random::find_seed_from_sequence(const std::vector<RobloxRandom::Structures::TargetInfo>& targets) {
    if (targets.empty()) {
        return std::nullopt;
    }

    struct PreparedTarget {
        uint64_t min_val;
        uint64_t range;
        uint64_t total_vals;
        int64_t target;
        bool is_large;
    };

    std::vector<PreparedTarget> prepared_targets;
    prepared_targets.reserve(targets.size());
    for (const auto& t : targets) {
        uint64_t t_min = (std::min)(static_cast<uint64_t>(t.min_val), static_cast<uint64_t>(t.max_val));
        uint64_t t_max = (std::max)(static_cast<uint64_t>(t.min_val), static_cast<uint64_t>(t.max_val));
        uint64_t t_range = t_max - t_min;
        prepared_targets.push_back({
            t_min,
            t_range,
            t_range + 1,
            t.target,
            (t_range >> 1) > 0x7FFFFFFE
            });
    }

    auto check_seed = [&](uint64_t seed) -> bool {
        uint64_t active_state = (105ULL + seed) * PCG_MULTIPLIER + PCG_INCREMENT;
        for (size_t i = 0; i < prepared_targets.size(); ++i) {
            uint64_t next_state_step = PCG_MULTIPLIER * active_state + PCG_INCREMENT;
            uint32_t check_low = std::rotr<uint32_t>(static_cast<uint32_t>((active_state >> 27) ^ (active_state >> 45)), static_cast<int>(active_state >> 59));
            uint64_t generated = 0;
            if (prepared_targets[i].is_large) {
                uint32_t check_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state_step >> 27) ^ (next_state_step >> 45)), static_cast<int>(next_state_step >> 59));
                active_state = PCG_MULTIPLIER * next_state_step + PCG_INCREMENT;
                if (prepared_targets[i].total_vals == 0) {
                    generated = check_low | (static_cast<uint64_t>(check_high) << 32);
                }
                else {
                    uint32_t total_vals_lo = static_cast<uint32_t>(prepared_targets[i].total_vals);
                    uint32_t total_vals_hi = static_cast<uint32_t>(prepared_targets[i].total_vals >> 32);
                    uint64_t term1 = (static_cast<uint64_t>(check_high) * total_vals_lo) >> 32;
                    uint64_t term2 = static_cast<uint64_t>(check_high) * total_vals_hi;
                    uint64_t term3 = (static_cast<uint64_t>(check_low) * total_vals_hi) >> 32;
                    generated = term1 + term2 + prepared_targets[i].min_val + term3;
                }
            }
            else {
                active_state = next_state_step;
                generated = prepared_targets[i].min_val + ((static_cast<uint64_t>(check_low) * prepared_targets[i].total_vals) >> 32);
            }
            if (generated != static_cast<uint64_t>(prepared_targets[i].target)) {
                return false;
            }
        }
        return true;
        };

    std::atomic<bool> found{ false };
    std::atomic<uint64_t> result_seed{ 0 };

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    std::vector<std::thread> threads;
    uint64_t chunk_size = (1ULL << 32) / num_threads;

    for (unsigned int t = 0; t < num_threads; ++t) {
        uint64_t start_seed = t * chunk_size;
        uint64_t end_seed = (t == num_threads - 1) ? 0xFFFFFFFFULL : (start_seed + chunk_size - 1);
        threads.emplace_back([&, start_seed, end_seed]() {
            for (uint64_t seed = start_seed; seed <= end_seed; ) {
                if (found.load(std::memory_order_relaxed)) return;
                uint64_t limit = (std::min)(static_cast<uint64_t>(seed + 15000ULL), end_seed);
                for (; seed <= limit; ++seed) {
                    if (check_seed(seed)) {
                        result_seed.store(seed);
                        found.store(true);
                        return;
                    }
                }
            }
            });
    }

    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    if (found.load()) {
        return result_seed.load();
    }

    size_t best_idx = 0;
    uint64_t max_range = 0;
    bool found_small_range = false;

    for (size_t i = 0; i < targets.size(); ++i) {
        uint64_t t_min = (std::min)(static_cast<uint64_t>(targets[i].min_val), static_cast<uint64_t>(targets[i].max_val));
        uint64_t t_max = (std::max)(static_cast<uint64_t>(targets[i].min_val), static_cast<uint64_t>(targets[i].max_val));
        uint64_t t_range = t_max - t_min;
        if ((t_range >> 1) <= 0x7FFFFFFE) {
            if (t_range > max_range) {
                max_range = t_range;
                best_idx = i;
                found_small_range = true;
            }
        }
    }

    if (!found_small_range) {
        return std::nullopt;
    }

    const auto& best_target = targets[best_idx];
    uint64_t min_val = (std::min)(static_cast<uint64_t>(best_target.min_val), static_cast<uint64_t>(best_target.max_val));
    uint64_t max_val = (std::max)(static_cast<uint64_t>(best_target.min_val), static_cast<uint64_t>(best_target.max_val));
    uint64_t range = max_val - min_val;

    uint64_t total_values = range + 1;
    uint64_t target_offset = static_cast<uint64_t>(best_target.target) - min_val;

    uint64_t rand_low_min = (target_offset * (1ULL << 32) + total_values - 1) / total_values;
    uint64_t rand_low_max = (((target_offset + 1) * (1ULL << 32)) - 1) / total_values;

    constexpr uint64_t M_INV = 0xC097EF87329E28A5ULL;

    for (uint64_t r_low = rand_low_min; r_low <= rand_low_max; ++r_low) {
        uint32_t rand_low_aligned = static_cast<uint32_t>(r_low);

        for (uint32_t rotation = 0; rotation < 32; ++rotation) {
            uint32_t unrotated = std::rotl<uint32_t>(rand_low_aligned, rotation);

            uint32_t part1 = unrotated & 0xFFF80000;
            uint32_t part2 = (unrotated & 0x0007C000) ^ (rotation << 14);
            uint32_t high_parts = part1 | part2;
            uint32_t low_parts = (unrotated ^ (high_parts >> 18)) & 0x00003FFF;

            uint32_t unxored_val = high_parts | low_parts;
            uint64_t state_base = (static_cast<uint64_t>(rotation) << 59) | (static_cast<uint64_t>(unxored_val) << 27);

            for (uint64_t low_bits = 0; low_bits < (1ULL << 27); ++low_bits) {
                uint64_t candidate_state_at_best_idx = state_base | low_bits;

                uint64_t current_state = candidate_state_at_best_idx;
                for (int i = static_cast<int>(best_idx) - 1; i >= 0; --i) {
                    uint64_t t_min = (std::min)(static_cast<uint64_t>(targets[i].min_val), static_cast<uint64_t>(targets[i].max_val));
                    uint64_t t_max = (std::max)(static_cast<uint64_t>(targets[i].min_val), static_cast<uint64_t>(targets[i].max_val));
                    uint64_t t_range = t_max - t_min;
                    if ((t_range >> 1) > 0x7FFFFFFE) {
                        current_state = (current_state - PCG_INCREMENT) * M_INV;
                        current_state = (current_state - PCG_INCREMENT) * M_INV;
                    }
                    else {
                        current_state = (current_state - PCG_INCREMENT) * M_INV;
                    }
                }

                bool is_valid_seed = true;
                uint64_t test_state = current_state;

                for (size_t i = 0; i < targets.size(); ++i) {
                    uint64_t t_min = (std::min)(static_cast<uint64_t>(targets[i].min_val), static_cast<uint64_t>(targets[i].max_val));
                    uint64_t t_max = (std::max)(static_cast<uint64_t>(targets[i].min_val), static_cast<uint64_t>(targets[i].max_val));
                    uint64_t t_range = t_max - t_min;

                    uint64_t next_state1 = PCG_MULTIPLIER * test_state + PCG_INCREMENT;
                    uint32_t check_low = std::rotr<uint32_t>(static_cast<uint32_t>((test_state >> 27) ^ (test_state >> 45)), static_cast<int>(test_state >> 59));

                    uint64_t generated = 0;
                    if ((t_range >> 1) > 0x7FFFFFFE) {
                        uint32_t check_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state1 >> 27) ^ (next_state1 >> 45)), static_cast<int>(next_state1 >> 59));
                        test_state = PCG_MULTIPLIER * next_state1 + PCG_INCREMENT;
                        uint64_t total_vals = t_range + 1;
                        if (total_vals == 0) {
                            generated = check_low | (static_cast<uint64_t>(check_high) << 32);
                        }
                        else {
                            uint32_t total_vals_lo = static_cast<uint32_t>(total_vals);
                            uint32_t total_vals_hi = static_cast<uint32_t>(total_vals >> 32);
                            uint64_t term1 = (static_cast<uint64_t>(check_high) * total_vals_lo) >> 32;
                            uint64_t term2 = static_cast<uint64_t>(check_high) * total_vals_hi;
                            uint64_t term3 = (static_cast<uint64_t>(check_low) * total_vals_hi) >> 32;
                            generated = term1 + term2 + t_min + term3;
                        }
                    }
                    else {
                        test_state = next_state1;
                        generated = t_min + ((static_cast<uint64_t>(check_low) * (t_range + 1)) >> 32);
                    }

                    if (generated != static_cast<uint64_t>(targets[i].target)) {
                        is_valid_seed = false;
                        break;
                    }
                }

                if (is_valid_seed) {
                    uint64_t seed = ((current_state - PCG_INCREMENT) * M_INV - PCG_INCREMENT);
                    return seed;
                }
            }
        }
    }

    return std::nullopt;
}
#endif // ROBLOX_RANDOM_ENABLE_SEED_FINDER