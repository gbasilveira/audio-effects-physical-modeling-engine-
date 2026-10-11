#include "audiofx/engine.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
int main() {
    using namespace audiofx;
    constexpr unsigned block = 128, repeats = 1024;
    std::array<double, block> left{}, right{}, outLeft{}, outRight{};
    for (unsigned n = 0; n < block; ++n)
        left[n] = right[n] = .1 * std::sin(n * .13);
    const double *input[]{left.data(), right.data()};
    double *output[]{outLeft.data(), outRight.data()};
    std::cout << "{\"sample_rate\":48000,\"channels\":2,\"block\":128,\"blocks_per_family\":1024,"
                 "\"measurements\":[";
    for (unsigned f = 1; f <= 32; ++f) {
        Design d;
        d.family = Family(f);
        Graph g;
        g.prepare({{d}, {}, 0}, {48000, block, 2});
        for (unsigned n = 0; n < 64; ++n)
            if (!g.process(input, output, block))
                return 1;
        std::vector<double> elapsed;
        elapsed.reserve(repeats);
        for (unsigned n = 0; n < repeats; ++n) {
            const auto start = std::chrono::steady_clock::now();
            if (!g.process(input, output, block))
                return 1;
            const auto stop = std::chrono::steady_clock::now();
            elapsed.push_back(std::chrono::duration<double, std::micro>(stop - start).count());
        }
        std::sort(elapsed.begin(), elapsed.end());
        if (f > 1)
            std::cout << ',';
        std::cout << "{\"family\":\"" << familyName(Family(f))
                  << "\",\"median_us\":" << elapsed[repeats / 2]
                  << ",\"p99_us\":" << elapsed[repeats * 99 / 100]
                  << ",\"maximum_us\":" << elapsed.back() << ",\"median_realtime_fraction\":"
                  << elapsed[repeats / 2] / (1e6 * block / 48000.) << '}';
    }
    std::cout << "]}\n";
}
