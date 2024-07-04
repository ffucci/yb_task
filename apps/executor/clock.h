#pragma once

#include <chrono>
#include <type_traits>
#include <thread>
#include <cstdint>
#include <stdexcept>

// based on David Gross implementation of a stable clock to measure performance

namespace yb::measure {
class Clock
{
   public:
    using TimePoint = uint64_t;

    static void init();
    static TimePoint now() noexcept;

    static std::chrono::nanoseconds from_cycles(uint64_t);

    template <class DurationT>
    static uint64_t to_cycles(DurationT);

    static double get_frequency_ghz()
    {
        return get_frequency_ghz_impl();
    }

   private:
    static double& get_frequency_ghz_impl()
    {
        static double tsc_freq_ghz = .0;
        return tsc_freq_ghz;
    }
};

namespace detail {

inline void cpuid()
{
    asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
}

inline uint64_t rdtscp()
{
    uint64_t rax, rdx;
    asm volatile("rdtsc" : "=a"(rax), "=d"(rdx));
    return (rdx << 32) + rax;
}

inline uint64_t rdtcsp2(int& chip, int& core)
{
    uint64_t rax, rcx, rdx;
    asm volatile("rdtscp" : "=a"(rax), "=d"(rdx), "=c"(rcx));
    chip = (rcx & 0xFFF000) >> 12;
    core = rcx & 0xFFF;
    return (rdx << 32) + rax;
}

inline double measure_tsc_frequency()
{
    using SteadyClock = std::conditional_t<
        std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;

    int chip, core, chip2, core2;

    const auto start_ts = SteadyClock::now();
    asm volatile("" : : "r,m"(start_ts) : "memory");

    const auto start_tsc = detail::rdtcsp2(chip, core);
    detail::cpuid();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto end_tsc = detail::rdtcsp2(chip2, core2);
    detail::cpuid();
    const auto end_ts = SteadyClock::now();

    if (core != core2 || chip != chip2) {
        throw std::runtime_error("Clock: process needs to be pinned to a specific core ");
    }

    const auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_ts - start_ts);
    return (end_tsc - start_tsc) / double(duration_ns.count());
}

}  // namespace detail

inline Clock::TimePoint Clock::now() noexcept
{
    return detail::rdtscp();
}

inline void Clock::init()
{
    double& tsc_freq = get_frequency_ghz_impl();
    if (tsc_freq != .0) {
        return;
    }

    double prev_freq = -1.0;
    for (int i = 0; i < 10; ++i) {
        tsc_freq = detail::measure_tsc_frequency();

        if (tsc_freq > prev_freq * 0.9999 && tsc_freq < prev_freq * 1.0001) {
            break;
        }

        prev_freq = tsc_freq;
    }
}

inline std::chrono::nanoseconds Clock::from_cycles(uint64_t cycles)
{
    const double nanoseconds{static_cast<double>(cycles) / get_frequency_ghz()};
    return std::chrono::nanoseconds(static_cast<uint64_t>(nanoseconds));
}

template <class DurationT>
inline uint64_t Clock::to_cycles(DurationT duration)
{
    const double nanoseconds{
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count())};
    return static_cast<uint64_t>(nanoseconds * get_frequency_ghz());
}

}  // namespace yb::measure
