#include "audiofx/engine.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>

using namespace audiofx;
namespace {
void u16(std::ostream &o, unsigned x) {
    o.put(char(x & 255));
    o.put(char((x >> 8) & 255));
}
void u32(std::ostream &o, std::uint32_t x) {
    for (unsigned n = 0; n < 4; ++n)
        o.put(char((x >> (8 * n)) & 255));
}
void wav(const std::string &path, const std::vector<Frame> &data, const ProcessSpec &spec) {
    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("cannot create output WAV");
    auto bytes = std::uint32_t(data.size() * spec.channels * 4);
    f.write("RIFF", 4);
    u32(f, 36 + bytes);
    f.write("WAVEfmt ", 8);
    u32(f, 16);
    u16(f, 3);
    u16(f, spec.channels);
    u32(f, unsigned(spec.sampleRate));
    u32(f, unsigned(spec.sampleRate) * spec.channels * 4);
    u16(f, spec.channels * 4);
    u16(f, 32);
    f.write("data", 4);
    u32(f, bytes);
    for (auto frame : data)
        for (unsigned ch = 0; ch < spec.channels; ++ch) {
            float v = float(frame[ch]);
            std::uint32_t bits;
            static_assert(sizeof(v) == sizeof(bits));
            std::memcpy(&bits, &v, 4);
            u32(f, bits);
        }
    if (!f)
        throw std::runtime_error("failed to write WAV");
}
Family parseFamily(const std::string &value) {
    for (unsigned n = 1; n <= 32; ++n)
        if (value == familyName(Family(n)) || value == std::to_string(n))
            return Family(n);
    throw std::invalid_argument("unknown effect family");
}
} // namespace
int main(int argc, char **argv) {
    try {
        Design design;
        ProcessSpec spec;
        spec.channels = 1;
        design.smoothingSeconds = 0;
        std::string output, input, stimulus = "impulse";
        bool raw = false, list = false;
        std::array<bool, parameterCount> specified{};
        std::size_t samples = 48000, tail = 0;
        double tone = 997, amplitude = .25, seconds = -1;
        const std::array<std::pair<std::string_view, Parameter>, parameterCount> parameters{
            {{"mix", Parameter::Mix},
             {"delay", Parameter::Delay},
             {"feedback", Parameter::Feedback},
             {"rate", Parameter::Rate},
             {"depth", Parameter::Depth},
             {"frequency", Parameter::Frequency},
             {"q", Parameter::Q},
             {"gain-db", Parameter::GainDb},
             {"drive", Parameter::Drive},
             {"threshold-db", Parameter::ThresholdDb},
             {"ratio", Parameter::Ratio},
             {"attack", Parameter::Attack},
             {"release", Parameter::Release},
             {"decay", Parameter::Decay},
             {"pitch", Parameter::Pitch},
             {"bits", Parameter::Bits},
             {"freeze", Parameter::Freeze}}};
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--raw") {
                raw = true;
                continue;
            }
            if (arg == "--list") {
                list = true;
                continue;
            }
            if (arg == "--help") {
                std::cout << "audiofx-render --family NAME --output FILE.wav [--signal "
                             "impulse|sine|noise|silence|music] [--seconds N] [--sample-rate N] "
                             "[--channels 1|2] [--block N] [--input FILE.csv] [--tail-samples N] "
                             "[--raw] [--mix N ...]\n";
                return 0;
            }
            if (i + 1 >= argc)
                throw std::invalid_argument("missing argument value");
            std::string value = argv[++i];
            if (arg == "--family")
                design.family = parseFamily(value);
            else if (arg == "--output")
                output = value;
            else if (arg == "--input")
                input = value;
            else if (arg == "--signal")
                stimulus = value;
            else if (arg == "--sample-rate")
                spec.sampleRate = std::stod(value);
            else if (arg == "--channels")
                spec.channels = unsigned(std::stoul(value));
            else if (arg == "--block")
                spec.maximumBlock = unsigned(std::stoul(value));
            else if (arg == "--oversampling")
                design.oversampling = unsigned(std::stoul(value));
            else if (arg == "--ir") {
                std::ifstream ir(value);
                if (!ir)
                    throw std::invalid_argument("cannot read IR text file");
                design.impulseResponse.clear();
                double coefficient;
                while (ir >> coefficient)
                    design.impulseResponse.push_back(coefficient);
                if (!ir.eof())
                    throw std::invalid_argument("invalid IR text");
            } else if (arg == "--samples")
                samples = std::stoull(value);
            else if (arg == "--tail-samples")
                tail = std::stoull(value);
            else if (arg == "--seconds") {
                seconds = std::stod(value);
                if (!std::isfinite(seconds) || seconds < 0 || seconds > 300)
                    throw std::invalid_argument("invalid duration");
            } else if (arg == "--tone")
                tone = std::stod(value);
            else if (arg == "--amplitude")
                amplitude = std::stod(value);
            else {
                bool found = false;
                for (auto [name, id] : parameters)
                    if (arg == "--" + std::string(name)) {
                        design.parameters[id] = std::stod(value);
                        specified[unsigned(id)] = true;
                        found = true;
                    }
                if (!found)
                    throw std::invalid_argument("unknown option: " + arg);
            }
        }
        if (list) {
            for (unsigned n = 1; n <= 32; ++n) {
                design.family = Family(n);
                auto node = prepare(design, spec);
                std::cout << n << '\t' << familyName(Family(n)) << '\t' << node->info().model
                          << '\n';
            }
            return 0;
        }
        if (!raw && output.empty())
            throw std::invalid_argument("specify --output or --raw");
        for (unsigned i = 0; i < parameterCount; ++i)
            if (specified[i] && !supportsParameter(design.family, Parameter(i)))
                throw std::invalid_argument("parameter not supported by this family");
        auto node = prepare(design, spec);
        (void)node;
        if (seconds >= 0) {
            if (!std::isfinite(seconds) || seconds > 300)
                throw std::invalid_argument("invalid duration");
            samples = std::size_t(seconds * spec.sampleRate);
        }
        if (!std::isfinite(amplitude) || std::abs(amplitude) > 100 || !std::isfinite(tone) ||
            tone < 0 || tone > spec.sampleRate / 2 || samples > 20000000 || tail > 20000000)
            throw std::invalid_argument("invalid stimulus/length");
        if (stimulus != "impulse" && stimulus != "sine" && stimulus != "noise" &&
            stimulus != "silence" && stimulus != "music")
            throw std::invalid_argument("unknown stimulus");
        std::vector<Frame> data;
        if (!input.empty()) {
            std::ifstream f(input);
            if (!f)
                throw std::runtime_error("cannot read input CSV");
            std::string line;
            while (std::getline(f, line)) {
                if (line.empty())
                    continue;
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream row(line);
                Frame v{};
                for (unsigned ch = 0; ch < spec.channels; ++ch)
                    if (!(row >> v[ch]))
                        throw std::runtime_error("CSV channel count mismatch");
                std::string extra;
                if (row >> extra)
                    throw std::runtime_error("CSV has extra columns");
                data.push_back(v);
                if (data.size() > 20000000)
                    throw std::runtime_error("input too long");
            }
        } else {
            data.resize(samples);
            std::uint64_t state = 74031;
            for (std::size_t n = 0; n < samples; ++n) {
                double t = double(n) / spec.sampleRate, v = 0;
                if (stimulus == "impulse")
                    v = n == 0 ? amplitude : 0;
                else if (stimulus == "sine")
                    v = amplitude * std::sin(2 * std::numbers::pi * tone * t);
                else if (stimulus == "noise") {
                    state = state * 6364136223846793005ULL + 1;
                    v = amplitude * (2 * double(state >> 11) * 0x1.0p-53 - 1);
                } else if (stimulus == "music") {
                    double envelope = std::exp(-8 * std::fmod(t, .5));
                    for (unsigned k = 1; k <= 8; ++k)
                        v +=
                            amplitude * envelope * std::sin(2 * std::numbers::pi * 110 * k * t) / k;
                }
                for (unsigned ch = 0; ch < spec.channels; ++ch)
                    data[n][ch] = v;
            }
        }
        if (data.size() + tail > 20000000)
            throw std::invalid_argument("combined render too long");
        data.resize(data.size() + tail);
        Graph graph;
        graph.prepare({{design}, {}, 0}, spec);
        std::array<std::vector<double>, 2> planes;
        for (unsigned ch = 0; ch < spec.channels; ++ch)
            planes[ch].resize(spec.maximumBlock);
        for (std::size_t pos = 0; pos < data.size(); pos += spec.maximumBlock) {
            auto count = std::uint32_t(std::min<std::size_t>(spec.maximumBlock, data.size() - pos));
            std::array<double *, 2> out{};
            std::array<const double *, 2> in{};
            for (unsigned ch = 0; ch < spec.channels; ++ch) {
                for (unsigned k = 0; k < count; ++k)
                    planes[ch][k] = data[pos + k][ch];
                in[ch] = out[ch] = planes[ch].data();
            }
            if (!graph.process(in.data(), out.data(), count))
                throw std::runtime_error("render process rejected");
            for (unsigned k = 0; k < count; ++k)
                for (unsigned ch = 0; ch < spec.channels; ++ch)
                    data[pos + k][ch] = planes[ch][k];
        }
        if (raw) {
            std::cout << std::setprecision(17);
            for (auto frame : data) {
                for (unsigned ch = 0; ch < spec.channels; ++ch) {
                    if (ch)
                        std::cout << ',';
                    std::cout << frame[ch];
                }
                std::cout << '\n';
            }
        } else {
            wav(output, data, spec);
            double peak = 0;
            for (auto v : data)
                for (unsigned ch = 0; ch < spec.channels; ++ch)
                    peak = std::max(peak, std::abs(v[ch]));
            auto stats = graph.diagnostics();
            std::cout << "{\"family\":\"" << familyName(design.family)
                      << "\",\"samples\":" << data.size() << ",\"sample_rate\":" << spec.sampleRate
                      << ",\"latency_samples\":" << graph.latencySamples() << ",\"peak\":" << peak
                      << ",\"solver_failures\":" << stats.solverFailures << "}\n";
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "audiofx-render: " << e.what() << '\n';
        return 1;
    }
}
