#include "audiofx/dsp.hpp"
#include "audiofx/engine.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__GNUC__)
#define AUDIOFX_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define AUDIOFX_NOINLINE __declspec(noinline)
#else
#define AUDIOFX_NOINLINE
#endif
static std::atomic<bool> watch{false};
static std::atomic<std::size_t> allocations{0};
void *operator new(std::size_t n) {
    if (watch)
        ++allocations;
    if (void *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) {
    return ::operator new(n);
}
void *operator new(std::size_t n, const std::nothrow_t &) noexcept {
    try {
        return ::operator new(n);
    } catch (...) {
        return nullptr;
    }
}
void *operator new[](std::size_t n, const std::nothrow_t &) noexcept {
    return ::operator new(n, std::nothrow);
}
AUDIOFX_NOINLINE void operator delete(void *p, const std::nothrow_t &) noexcept {
    std::free(p);
}
AUDIOFX_NOINLINE void operator delete[](void *p, const std::nothrow_t &) noexcept {
    std::free(p);
}
AUDIOFX_NOINLINE void operator delete(void *p) noexcept {
    std::free(p);
}
AUDIOFX_NOINLINE void operator delete[](void *p) noexcept {
    std::free(p);
}
AUDIOFX_NOINLINE void operator delete(void *p, std::size_t) noexcept {
    std::free(p);
}
AUDIOFX_NOINLINE void operator delete[](void *p, std::size_t) noexcept {
    std::free(p);
}
using namespace audiofx;
using namespace audiofx::dsp;
static unsigned checks = 0;
void require(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void close(double a, double b, double tolerance, const char *message) {
    require(std::abs(a - b) <= tolerance, message);
}
std::vector<Frame> render(const Design &d, double fs, unsigned channels, unsigned block,
                          const std::vector<Frame> &x) {
    Graph g;
    g.prepare({{d}, {}, 0}, {fs, block, channels});
    std::vector<Frame> y(x.size());
    std::array<std::vector<double>, 2> input, output;
    for (unsigned c = 0; c < channels; ++c) {
        input[c].resize(block);
        output[c].resize(block);
    }
    for (std::size_t pos = 0; pos < x.size(); pos += block) {
        unsigned n = unsigned(std::min<std::size_t>(block, x.size() - pos));
        std::array<const double *, 2> in{};
        std::array<double *, 2> out{};
        for (unsigned c = 0; c < channels; ++c) {
            for (unsigned k = 0; k < n; ++k)
                input[c][k] = x[pos + k][c];
            in[c] = input[c].data();
            out[c] = output[c].data();
        }
        watch = true;
        bool ok = g.process(in.data(), out.data(), n);
        watch = false;
        require(ok, "valid processing rejected");
        for (unsigned k = 0; k < n; ++k)
            for (unsigned c = 0; c < channels; ++c)
                y[pos + k][c] = output[c][k];
    }
    require(g.diagnostics().solverFailures == 0, "nonlinear solver failed in normal domain");
    return y;
}
int main() {
    try {
        DelayLine line;
        line.prepare(16);
        line.push(1);
        close(line.read(1), 1, 0, "delay alignment");
        line.push(0);
        close(line.read(1.5), .5, 0, "fractional interpolation");
        WdfRc rc;
        Biquad b;
        double fs = 48000, R = rc.R, C = rc.C, alpha = 2 * fs * R * C;
        b.assign(1, 1, 0, 1 + alpha, 1 - alpha, 0);
        for (unsigned i = 0; i < 500; ++i) {
            double x = std::sin(i * .2);
            close(rc.tick(x, fs), b.tick(x), 1e-12, "WDF capacitor sign");
        }
        Mode mode;
        mode.prepare(fs, 2 * pi * 700, 5, 1, 1);
        mode.q = 1;
        double energy = mode.energy();
        for (unsigned i = 0; i < 4096; ++i) {
            mode.tick(0);
            require(mode.energy() <= energy * (1 + 1e-12), "modal energy grew");
            energy = mode.energy();
        }
        DiodeRc diode;
        for (unsigned i = 0; i < 1000; ++i) {
            double input = 2 * std::sin(i * .3), old = diode.voltage, v = diode.tick(input, fs);
            double residual = diode.C * fs * (v - old) + (v - input) / diode.R +
                              2 * diode.Is * std::sinh(v / diode.thermal);
            require(std::abs(residual) < 1e-11, "diode residual");
        }
        require(diode.failures == 0, "diode solver failure");
        Fft fft;
        fft.prepare(256);
        std::array<std::complex<double>, 256> spectrum{};
        for (unsigned i = 0; i < 256; ++i)
            spectrum[i] = std::sin(.2 * i);
        auto original = spectrum;
        fft.transform(spectrum.data());
        fft.transform(spectrum.data(), true);
        for (unsigned i = 0; i < 256; ++i)
            close(std::abs(spectrum[i] - original[i]), 0, 1e-12, "FFT roundtrip");
        Fdn fdn;
        fdn.prepare(fs, 1e100);
        fdn.tick(1);
        energy = fdn.energy();
        for (unsigned i = 0; i < 4096; ++i) {
            fdn.tick(0);
            close(fdn.energy(), energy, 1e-12, "FDN lossless energy");
        }
        std::vector<Frame> stimulus(2053);
        std::mt19937_64 rng(74031);
        for (auto &x : stimulus)
            x = {double(rng() % 100000) / 500000 - .1, double(rng() % 100000) / 500000 - .1};
        for (unsigned family = 1; family <= 32; ++family) {
            Design d;
            d.family = Family(family);
            d.smoothingSeconds = .001;
            auto a = render(d, fs, 2, 32, stimulus), c = render(d, fs, 2, 257, stimulus);
            for (std::size_t n = 0; n < a.size(); ++n)
                for (unsigned ch = 0; ch < 2; ++ch) {
                    require(std::isfinite(a[n][ch]), "nonfinite family output");
                    close(a[n][ch], c[n][ch], 0, "block partition changed output");
                }
            ProcessSpec spec{fs, 512, 2};
            auto node = prepare(d, spec);
            std::vector<Frame> first;
            for (unsigned n = 0; n < 300; ++n)
                first.push_back(node->tick(stimulus[n]));
            node->reset();
            for (unsigned n = 0; n < 300; ++n) {
                auto v = node->tick(stimulus[n]);
                close(v[0], first[n][0], 0, "reset nondeterministic");
                close(v[1], first[n][1], 0, "stereo reset nondeterministic");
            }
            // Parameter changes, overload recovery, NaN sanitisation and supported sample-rate
            // limits.
            for (auto rate : {8000., 44100., 96000., 192000.}) {
                auto effect = prepare(d, {rate, 64, 1});
                for (unsigned n = 0; n < 512; ++n) {
                    if (n == 150)
                        effect->set(Parameter::Drive, 20);
                    auto v = effect->tick({n == 151  ? std::numeric_limits<double>::quiet_NaN()
                                           : n < 256 ? 2 * std::sin(n * .1)
                                                     : 0.,
                                           0});
                    require(std::isfinite(v[0]), "rate/overload nonfinite");
                }
                require(effect->diagnostics().invalidSamples == 1, "NaN diagnostic missing");
            }
        }
        require(allocations == 0, "allocation inside process callback");
        Design eq;
        eq.family = Family::Eq;
        eq.parameters[Parameter::Mix] = 1;
        eq.smoothingSeconds = 0;
        Graph graph;
        graph.prepare({{eq}, {}, 0}, {fs, 128, 1});
        std::array<double, 128> input{}, output{};
        input.fill(.1);
        const double *in[]{input.data()};
        double *out[]{output.data()};
        std::array<Event, 2> events{
            {{32, 0, Parameter::GainDb, 12}, {64, 0, Parameter::GainDb, 0}}};
        require(graph.process(in, out, 128, events), "event processing");
        for (unsigned i = 0; i < 32; ++i)
            close(input[i], output[i], 1e-14, "event applied early");
        std::array<Event, 2> invalid{
            {{64, 0, Parameter::GainDb, 0}, {32, 0, Parameter::GainDb, 12}}};
        require(!graph.process(in, out, 128, invalid), "unsorted events accepted");
        require(!graph.process(in, out, 129), "oversized block accepted");
        require(graph.process(nullptr, nullptr, 0), "zero block rejected");
        require(!graph.process(nullptr, out, 128), "null input accepted");
        bool rejected = false;
        try {
            Graph g;
            g.prepare({{eq, eq}, {{0, 1, 1, false}, {1, 0, 1, false}}, 1}, {fs, 128, 1});
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require(rejected, "algebraic cycle accepted");
        Design convolution;
        convolution.family = Family::Convolution;
        convolution.parameters[Parameter::Mix] = 1;
        Graph alignment;
        alignment.prepare({{eq, convolution, eq}, {{0, 2, 1, false}, {1, 2, 1, false}}, 2},
                          {fs, 128, 1});
        require(alignment.latencySamples() == 128, "parallel latency not propagated");
        input.fill(0);
        input[0] = 1;
        require(alignment.process(in, out, 128), "parallel process");
        for (double x : output)
            close(x, 0, 1e-12, "short path not aligned");
        input.fill(0);
        alignment.process(in, out, 128);
        close(output[0], 2, 1e-12, "aligned impulse sum");
        Graph feedback;
        eq.parameters[Parameter::GainDb] = 0;
        feedback.prepare({{eq}, {{0, 0, .5, true}}, 0}, {fs, 128, 1});
        input.fill(0);
        input[0] = 1;
        feedback.process(in, out, 128);
        for (unsigned n = 0; n < 32; ++n)
            close(output[n], std::pow(.5, n), 1e-12, "explicit feedback sample/injection");
        // Invalid reprepare must preserve the existing operational graph.
        rejected = false;
        Design invalidDesign = eq;
        invalidDesign.parameters[Parameter::Q] = 0;
        try {
            feedback.prepare({{invalidDesign}, {}, 0}, {fs, 128, 1});
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require(rejected, "invalid design accepted");
        feedback.reset();
        feedback.process(in, out, 128);
        close(output[1], .5, 1e-12, "failed prepare destroyed graph");
        // Invalid performance parameters are diagnosed and leave valid targets unchanged.
        auto standalone = prepare(eq, {fs, 128, 1});
        require(!standalone->set(Parameter::Mix, 2), "invalid event range accepted");
        require(!standalone->set(Parameter(1000), 1), "invalid parameter ID accepted");
        for (auto family : {Family::Diode, Family::Amplifier})
            for (unsigned factor : {2u, 4u, 8u}) {
                Design d;
                d.family = family;
                d.oversampling = factor;
                std::vector<Frame> stimulus(2048);
                stimulus[0] = {.25, -.25};
                auto a = render(d, fs, 2, 32, stimulus);
                auto b = render(d, fs, 2, 257, stimulus);
                for (unsigned n = 0; n < a.size(); ++n)
                    for (unsigned c = 0; c < 2; ++c)
                        close(a[n][c], b[n][c], 1e-13, "oversampling block dependence");
                require(prepare(d, {fs, 128, 2})->info().latencySamples == 16,
                        "oversampling latency");
            }
        require(allocations == 0, "callback allocation detected");
        std::cout << "{\"checks\":" << checks << ",\"callback_allocations\":" << allocations
                  << ",\"families\":32,\"status\":\"passed\"}\n";
        return 0;
    } catch (const std::exception &e) {
        watch = false;
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
