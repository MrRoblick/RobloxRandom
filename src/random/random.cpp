#include <random/random.h>
#include <algorithm>
#include <bit>
#include <chrono>

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

std::atomic<uint64_t> Random::global_entropy_seed{ 0 };
std::atomic<uint64_t> Random::global_entropy_seed_value{ 0 };
std::atomic<uint32_t> Random::entropy_counter{ 0 };
std::atomic<uint8_t>  Random::entropy_mutex_lock{ 0 };


uint64_t Random::get_nanoseconds_monotonic() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

uint64_t Random::get_microseconds_realtime() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

uint64_t Random::get_nanoseconds_float() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000LL;

    if constexpr (float_arith00 && (float_arith01 & 1) != 0) {
        return static_cast<uint64_t>(nanoseconds) + 1000000000ULL * seconds;
    }
    else {
        double sec_part = static_cast<double>(static_cast<int>(seconds)) * 1000000000.0;
        double nsec_part = static_cast<double>(static_cast<int>(nanoseconds));
        return static_cast<uint32_t>(static_cast<int>(nsec_part + sec_part));
    }
}

uint64_t Random::get_current_pid() {
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<uint64_t>(GetCurrentProcessId());
#else
    return static_cast<uint64_t>(getpid());
#endif
}

uint64_t Random::get_current_thread_id() {
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<uint64_t>(GetCurrentThreadId());
#else
    return reinterpret_cast<uint64_t>(pthread_self());
#endif
}

uint64_t Random::entropy_seed() {
    uint64_t v8 = global_entropy_seed.load(std::memory_order_relaxed);
    if (!v8) {
        uint64_t v1 = 7ULL * get_nanoseconds_monotonic();
        uint64_t v2 = 97ULL * get_microseconds_realtime();
        uint64_t v3 = 997ULL * get_current_pid();
        uint64_t v4 = 1997ULL * get_current_thread_id();

        uintptr_t stack_addr = reinterpret_cast<uintptr_t>(&v8);
        uintptr_t global_addr = reinterpret_cast<uintptr_t>(&global_entropy_seed);

        v8 = v4
            ^ (19997ULL * get_nanoseconds_float())
            ^ v3
            ^ v2
            ^ v1
            ^ static_cast<uint64_t>(stack_addr)
            ^ (static_cast<uint64_t>(global_addr) << 32);
    }

    uint32_t current_counter = entropy_counter.fetch_add(1, std::memory_order_relaxed);
    uint64_t v5 = v8 ^ static_cast<uint64_t>(2 * current_counter);

    uint64_t v6 = PCG_MULTIPLIER * v5 + PCG_INCREMENT;
    global_entropy_seed.store(PCG_MULTIPLIER * v6 + PCG_INCREMENT, std::memory_order_relaxed);

    uint32_t high = std::rotr<uint32_t>(static_cast<uint32_t>((v6 >> 27) ^ (v6 >> 45)), static_cast<int>(v6 >> 59));
    uint32_t low = std::rotr<uint32_t>(static_cast<uint32_t>((v5 >> 27) ^ (v8 >> 45)), static_cast<int>(v8 >> 59));

    return (static_cast<uint64_t>(high) << 32) | low;
}

void Random::safe_gen_entropy_seed() {
    uint8_t expected = 0;
    if (entropy_mutex_lock.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        global_entropy_seed_value.store(entropy_seed(), std::memory_order_release);
        entropy_mutex_lock.store(0, std::memory_order_release);
    }
}

uint64_t Random::hash_seed(std::atomic<uint64_t>& entropy_seed_value_ptr) {
    uint64_t v1 = entropy_seed_value_ptr.load(std::memory_order_relaxed);
    uint64_t v2 = PCG_MULTIPLIER * v1 + PCG_INCREMENT;

    while (true) {
        uint64_t expected = v1;
        if (entropy_seed_value_ptr.compare_exchange_strong(expected, v2, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            break;
        }
        v1 = expected;
        v2 = PCG_MULTIPLIER * v1 + PCG_INCREMENT;
    }

    uint64_t v5 = entropy_seed_value_ptr.load(std::memory_order_relaxed);
    uint64_t v6 = PCG_MULTIPLIER * v5 + PCG_INCREMENT;

    while (true) {
        uint64_t expected = v5;
        if (entropy_seed_value_ptr.compare_exchange_strong(expected, v6, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            break;
        }
        v5 = expected;
        v6 = PCG_MULTIPLIER * v5 + PCG_INCREMENT;
    }

    uint32_t high = std::rotr<uint32_t>(static_cast<uint32_t>((v5 >> 27) ^ (v5 >> 45)), static_cast<int>(v5 >> 59));
    uint32_t low = std::rotr<uint32_t>(static_cast<uint32_t>((v1 >> 27) ^ (v1 >> 45)), static_cast<int>(v1 >> 59));

    return (static_cast<uint64_t>(high) << 32) | low;
}

uint64_t Random::generate_roblox_default_seed() {
    safe_gen_entropy_seed();
    uint64_t hashed = hash_seed(global_entropy_seed_value);
    return PCG_MULTIPLIER * hashed + 0x399D2694695129DELL;
}


Random::Random() {
    state = generate_roblox_default_seed();
}

Random::Random(uint64_t seed) {
    uint64_t temp_state = 0;
    temp_state = temp_state * PCG_MULTIPLIER + PCG_INCREMENT;
    temp_state += seed;
    temp_state = temp_state * PCG_MULTIPLIER + PCG_INCREMENT;
    state = temp_state;
}

uint64_t Random::next_integer(int64_t _min, int64_t _max) {
    uint64_t min_val = (std::min)(_min, _max);
    uint64_t max_val = (std::max)(_min, _max);
    uint64_t range = max_val - min_val;

    uint64_t next_state1 = PCG_MULTIPLIER * state + PCG_INCREMENT;
    uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((state >> 27) ^ (state >> 45)), static_cast<int>(state >> 59));

    if ((range >> 1) > 0x7FFFFFFE) {
        uint32_t rand_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state1 >> 27) ^ (next_state1 >> 45)), static_cast<int>(next_state1 >> 59));
        state = PCG_MULTIPLIER * next_state1 + PCG_INCREMENT;
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

            return term1 + term2 + min_val + term3;
        }
    }
    else {
        state = next_state1;
        return min_val + ((static_cast<uint64_t>(rand_low) * (range + 1)) >> 32);
    }
}

double Random::next_double() {
    uint64_t current_state = state;

    uint64_t next_state = current_state * PCG_MULTIPLIER + PCG_INCREMENT;
    state = next_state * PCG_MULTIPLIER + PCG_INCREMENT;

    uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((current_state >> 27) ^ (current_state >> 45)), static_cast<int>(current_state >> 59));
    uint32_t rand_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state >> 27) ^ (next_state >> 45)), static_cast<int>(next_state >> 59));

    uint64_t rand64 = (static_cast<uint64_t>(rand_high) << 32) | rand_low;

    return std::ldexp(static_cast<double>(rand64), -64);
}

double Random::next_number() {
    return next_double();
}

double Random::next_number(double arg1, double arg2) {
    double min_val = (std::min)(arg1, arg2);
    double max_val = (std::max)(arg1, arg2);

    double r = next_double();

    double result = r * max_val + (1.0 - r) * min_val;

    if (result < min_val) return min_val;
    if (result > max_val) return max_val;

    return result;
}

Vector3 Random::next_unit_vector() {
    uint64_t current_state = state;

    uint64_t next_state = current_state * PCG_MULTIPLIER + PCG_INCREMENT;
    state = next_state * PCG_MULTIPLIER + PCG_INCREMENT;

    uint32_t rand_low = std::rotr<uint32_t>(static_cast<uint32_t>((current_state >> 27) ^ (current_state >> 45)), static_cast<int>(current_state >> 59));
    uint32_t rand_high = std::rotr<uint32_t>(static_cast<uint32_t>((next_state >> 27) ^ (next_state >> 45)), static_cast<int>(next_state >> 59));

    double v2 = std::ldexp(static_cast<double>(rand_low), -32);
    double v3 = std::ldexp(static_cast<double>(rand_high), -32);

    float x_angle = static_cast<float>(v2 * 6.283185307179586);

    float z = static_cast<float>(v3 * 2.0 - 1.0);

    float r_xy = std::sqrt(1.0f - z * z);

    return Vector3{
        std::cos(x_angle) * r_xy,
        std::sin(x_angle) * r_xy,
        z
    };
}

std::optional<uint64_t> Random::find_seed(int64_t target, int64_t min_val, int64_t max_val) {
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

    uint32_t x = unxor_xsh_rr(static_cast<uint32_t>(target_rand_low));
    uint64_t state_base = static_cast<uint64_t>(x) << 27;

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