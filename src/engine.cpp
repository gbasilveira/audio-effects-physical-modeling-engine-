#include "audiofx/engine.hpp"
#include "audiofx/dsp.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace audiofx {
using namespace dsp;
namespace {
constexpr std::array<Range, parameterCount> ranges{{{0, 1, "fraction"},
                                                    {.001, 2, "s"},
                                                    {-.95, .95, "amplitude"},
                                                    {0, 20, "Hz"},
                                                    {0, .05, "s"},
                                                    {1, 20000, "Hz"},
                                                    {.1, 20, "ratio"},
                                                    {-24, 24, "dB"},
                                                    {.01, 20, "ratio"},
                                                    {-80, 0, "dBFS"},
                                                    {1, 30, "ratio"},
                                                    {.0001, 2, "s"},
                                                    {.001, 10, "s"},
                                                    {.05, 20, "s"},
                                                    {-24, 24, "semitones"},
                                                    {2, 24, "bits"},
                                                    {0, 1, "boolean"}}};
constexpr std::array<std::string_view, 32> names{"delay",       "tape",        "bbd",
                                                 "flanger",     "chorus",      "phaser",
                                                 "tremolo",     "ring",        "frequency-shift",
                                                 "eq",          "wah",         "compressor",
                                                 "optical",     "vca",         "transient",
                                                 "diode",       "amplifier",   "speaker",
                                                 "fdn",         "convolution", "room",
                                                 "plate",       "spring",      "spatial",
                                                 "pitch",       "granular",    "vocoder",
                                                 "spectral",    "crusher",     "resonator",
                                                 "restoration", "multi-fx"};
constexpr std::array<std::string_view, 32> models{
    "integer/fractional feedback delay",
    "play-operator hysteresis and RC head reduction",
    "clocked charge-queue baseline",
    "causal through-zero comb",
    "three-voice modulated delay",
    "four ideal allpass stages",
    "bounded sinusoidal gain",
    "ideal balanced multiplication",
    "63-tap Hilbert single-sideband shift",
    "RBJ peaking EQ",
    "TPT state-variable bandpass",
    "feedforward RMS compressor",
    "two-state phenomenological optical recovery",
    "exponential VCA with explicitly delayed feedback",
    "fast/slow-envelope transient gain",
    "implicit RC antiparallel-diode circuit",
    "triode-law stage with RC supply sag",
    "linear electromechanical speaker states",
    "four-line orthogonal Householder FDN",
    "uniform partitioned FIR convolution",
    "shoebox first-order reflections and low modes",
    "simply supported physical modal plate",
    "reduced dispersive spring waveguide",
    "geometric stereo moving-source rendering",
    "16-partial phase-tracked spectral pitch shift",
    "bounded four-grain delay reader",
    "eight-band channel vocoder",
    "256-point spectral hold",
    "uniform quantisation and clocked hold",
    "physical stiff-string modal bank",
    "fixed-noise-power spectral subtraction",
    "delay/EQ/diode serial composition"};
void validate(const Design &d, const ProcessSpec &s) {
    if (!std::isfinite(s.sampleRate) || s.sampleRate < 8000 || s.sampleRate > 192000 ||
        s.channels < 1 || s.channels > 2 || s.maximumBlock < 1 || s.maximumBlock > 8192)
        throw std::invalid_argument("invalid processing specification");
    if (d.oversampling != 1 && d.oversampling != 2 && d.oversampling != 4 && d.oversampling != 8)
        throw std::invalid_argument("invalid oversampling factor");
    if (unsigned(d.family) < 1 || unsigned(d.family) > 32)
        throw std::invalid_argument("unknown family");
    if (!std::isfinite(d.smoothingSeconds) || d.smoothingSeconds < 0 || d.smoothingSeconds > 2)
        throw std::invalid_argument("invalid smoothing time");
    for (std::size_t i = 0; i < parameterCount; ++i)
        if (!std::isfinite(d.parameters.values[i]) || d.parameters.values[i] < ranges[i].minimum ||
            d.parameters.values[i] > ranges[i].maximum)
            throw std::invalid_argument("parameter outside declared range");
    const auto &p = d.physical;
    const std::array<double, 13> positive{p.plateX,        p.plateY,        p.thickness,
                                          p.density,       p.young,         p.roomX,
                                          p.roomY,         p.roomZ,         p.stringLength,
                                          p.stringTension, p.linearDensity, p.observationScale,
                                          double(p.modes)};
    for (double x : positive)
        if (!std::isfinite(x) || x <= 0)
            throw std::invalid_argument("physical design must be finite and positive");
    if (p.plateX < .05 || p.plateY < .05 || p.thickness < 1e-6 || p.density < 100 ||
        p.young < 1e6 || p.roomX < .5 || p.roomY < .5 || p.roomZ < .5 || p.stringLength < .05 ||
        p.stringTension < .01 || p.linearDensity < 1e-6 || p.observationScale < .001 ||
        p.plateX > 20 || p.plateY > 20 || p.thickness > .1 || p.density > 30000 || p.young > 1e13 ||
        p.modes > 256 || p.roomX > 100 || p.roomY > 100 || p.roomZ > 100 || p.stringLength > 20 ||
        p.stringTension > 1e6 || p.linearDensity > 10 || p.observationScale > 1e8)
        throw std::invalid_argument("physical design exceeds supported domain");
    if (!std::isfinite(p.poisson) || p.poisson <= -.9 || p.poisson >= .49 ||
        !std::isfinite(p.tension) || p.tension < 0 || p.tension > 1e7 ||
        !std::isfinite(p.bendingStiffness) || p.bendingStiffness < 0 || p.bendingStiffness > 1000 ||
        !std::isfinite(p.absorption) || p.absorption < .01 || p.absorption > 1)
        throw std::invalid_argument("invalid physical coefficient");
    for (double x : {p.inputX, p.inputY, p.outputX, p.outputY})
        if (!std::isfinite(x) || x < 0 || x > 1)
            throw std::invalid_argument("normalised position outside [0,1]");
    if (d.impulseResponse.empty() || d.impulseResponse.size() > 262144)
        throw std::invalid_argument("IR size outside [1,262144]");
    for (double x : d.impulseResponse)
        if (!std::isfinite(x) || std::abs(x) > 1e6)
            throw std::invalid_argument("invalid IR coefficient");
}
using Matrix = std::array<std::array<double, 4>, 4>;
Matrix multiply(const Matrix &a, const Matrix &b) {
    Matrix c{};
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < 4; ++j)
            for (unsigned k = 0; k < 4; ++k)
                c[i][j] += a[i][k] * b[k][j];
    return c;
}
Matrix exponential(Matrix a) {
    double norm = 0;
    for (auto row : a) {
        double n = 0;
        for (double x : row)
            n += std::abs(x);
        norm = std::max(norm, n);
    }
    unsigned scale = norm > .5 ? unsigned(std::ceil(std::log2(norm / .5))) : 0;
    for (auto &row : a)
        for (auto &x : row)
            x = std::ldexp(x, -int(scale));
    Matrix result{}, term{};
    for (unsigned i = 0; i < 4; ++i)
        result[i][i] = term[i][i] = 1;
    for (unsigned n = 1; n <= 32; ++n) {
        term = multiply(term, a);
        for (unsigned i = 0; i < 4; ++i)
            for (unsigned j = 0; j < 4; ++j) {
                term[i][j] /= n;
                result[i][j] += term[i][j];
            }
    }
    for (unsigned i = 0; i < scale; ++i)
        result = multiply(result, result);
    return result;
}
struct Channel {
    DelayLine delay, dry;
    Biquad filter, extra;
    Svf svf;
    DcBlock dc;
    DiodeRc diode;
    AdaaTanh adaa;
    Fdn fdn;
    ModalBank modes;
    Convolver convolution;
    Oversampler oversampler;
    Hilbert hilbert;
    SpectralPitch spectralPitch;
    Stft stft;
    std::array<Allpass, 12> allpasses{};
    std::array<Biquad, 8> bands{}, carriers{};
    std::array<Envelope, 8> bandEnvelopes{};
    Envelope detector, control, fast, slow;
    double feedback{}, play{}, head{}, supply{1}, opto1{}, opto2{}, hold{}, clockPhase{},
        pitchPhase{};
    std::array<double, 256> buckets{};
    std::size_t bucket{};
    std::array<double, 3> speaker{};
    std::array<unsigned, 4> grainAge{};
    std::array<double, 4> grainDelay{};
    void reset() noexcept {
        delay.reset();
        dry.reset();
        filter.reset();
        extra.reset();
        svf.reset();
        dc.reset();
        diode.reset();
        adaa.reset();
        fdn.reset();
        modes.reset();
        convolution.reset();
        oversampler.reset();
        hilbert.reset();
        spectralPitch.reset();
        stft.reset();
        for (auto &a : allpasses)
            a.reset();
        for (auto &a : bands)
            a.reset();
        for (auto &a : carriers)
            a.reset();
        for (auto &a : bandEnvelopes)
            a.reset();
        detector.reset();
        control.reset();
        fast.reset();
        slow.reset();
        feedback = play = head = opto1 = opto2 = hold = clockPhase = pitchPhase = 0;
        supply = 1;
        buckets.fill(0);
        bucket = 0;
        speaker.fill(0);
        grainAge.fill(512);
        grainDelay.fill(0);
    }
};
class Effect final : public Node {
    Design design_;
    ProcessSpec spec_;
    Parameters current_, target_;
    std::array<Channel, 2> channels_{};
    NodeInfo info_{};
    Diagnostics diagnostics_{};
    double phase_{}, smoothing_{}, lastFrequency_{-1}, lastQ_{-1}, lastGain_{1e9}, lastDecay_{-1};
    std::uint64_t time_{}, random_{};
    Matrix speakerStep_{};
    std::array<double, 7> roomDelays_{}, roomGains_{};
    std::array<std::unique_ptr<Node>, 3> children_{};
    double p(Parameter id) const noexcept {
        return current_[id];
    }
    double rand() noexcept {
        random_ ^= random_ >> 12;
        random_ ^= random_ << 25;
        random_ ^= random_ >> 27;
        return double((random_ * 2685821657736338717ULL) >> 11) * 0x1.0p-53;
    }
    void compileModes(Channel &c) {
        const auto &d = design_.physical;
        unsigned count = 0;
        if (design_.family == Family::Plate) {
            double D = d.young * std::pow(d.thickness, 3) / (12 * (1 - d.poisson * d.poisson));
            double mass = d.density * d.thickness * d.plateX * d.plateY / 4;
            struct Candidate {
                double f, input, output;
            };
            std::vector<Candidate> candidates;
            for (unsigned m = 1; m <= 32; ++m)
                for (unsigned n = 1; n <= 32; ++n) {
                    double k2 = std::pow(m * pi / d.plateX, 2) + std::pow(n * pi / d.plateY, 2);
                    double f =
                        std::sqrt((D * k2 * k2 + d.tension * k2) / (d.density * d.thickness)) /
                        (2 * pi);
                    candidates.push_back(
                        {f, std::sin(m * pi * d.inputX) * std::sin(n * pi * d.inputY) / mass,
                         std::sin(m * pi * d.outputX) * std::sin(n * pi * d.outputY) *
                             d.observationScale});
                }
            std::stable_sort(candidates.begin(), candidates.end(),
                             [](auto a, auto b) { return a.f < b.f; });
            for (auto a : candidates) {
                if (count++ >= d.modes)
                    break;
                c.modes.add(spec_.sampleRate, a.f, std::log(1000.) / p(Parameter::Decay), a.input,
                            a.output);
            }
        } else if (design_.family == Family::Resonator) {
            for (unsigned m = 1; m <= d.modes; ++m) {
                double k = m * pi / d.stringLength;
                double f = std::sqrt(d.stringTension / d.linearDensity * k * k +
                                     d.bendingStiffness / d.linearDensity * std::pow(k, 4)) /
                           (2 * pi);
                c.modes.add(spec_.sampleRate, f, std::log(1000.) / p(Parameter::Decay),
                            2 * std::sin(m * pi * d.inputX) / (d.linearDensity * d.stringLength),
                            std::sin(m * pi * d.outputX) * 100);
            }
        } else {
            for (unsigned nx = 0; nx < 4; ++nx)
                for (unsigned ny = 0; ny < 4; ++ny)
                    for (unsigned nz = 0; nz < 4; ++nz) {
                        if (nx + ny + nz == 0)
                            continue;
                        double f = 343 / 2. *
                                   std::sqrt(std::pow(nx / d.roomX, 2) + std::pow(ny / d.roomY, 2) +
                                             std::pow(nz / d.roomZ, 2));
                        double in = std::cos(nx * pi * .3) * std::cos(ny * pi * .4) *
                                    std::cos(nz * pi * .5);
                        double out = std::cos(nx * pi * .6) * std::cos(ny * pi * .5) *
                                     std::cos(nz * pi * .5);
                        c.modes.add(spec_.sampleRate, f, std::log(1000.) / p(Parameter::Decay), in,
                                    out * 100);
                    }
        }
    }

  public:
    Effect(const Design &d, const ProcessSpec &s)
        : design_(d), spec_(s), current_(d.parameters), target_(d.parameters),
          random_(d.seed ? d.seed : 1) {
        validate(d, s);
        smoothing_ =
            d.smoothingSeconds > 0 ? std::exp(-1 / (s.sampleRate * d.smoothingSeconds)) : 0;
        info_.model = models[unsigned(d.family) - 1];
        info_.physical = d.family == Family::Diode || d.family == Family::Speaker ||
                         d.family == Family::Plate || d.family == Family::Resonator ||
                         d.family == Family::Room;
        // Recursive models and freeze can have unbounded mathematical support.
        info_.tailSeconds = std::numeric_limits<double>::infinity();
        if (d.family == Family::Tremolo || d.family == Family::Ring)
            info_.tailSeconds = 0;
        if (d.family == Family::Chorus)
            info_.tailSeconds = .068 + 1 / s.sampleRate;
        switch (d.family) {
        case Family::Flanger:
            info_.latencySamples = std::uint32_t(std::ceil(s.sampleRate * .02));
            break;
        case Family::FrequencyShift:
            info_.latencySamples = 31;
            break;
        case Family::Convolution:
            info_.latencySamples = 128;
            info_.tailSeconds = double(d.impulseResponse.size() + 128) / s.sampleRate;
            break;
        case Family::Pitch:
            info_.latencySamples = 1024;
            break;
        case Family::Spectral:
        case Family::Restoration:
            info_.latencySamples = 256;
            break;
        default:
            break;
        }
        if ((d.family == Family::Diode || d.family == Family::Amplifier) && d.oversampling > 1)
            info_.latencySamples = 16;
        if (d.family == Family::MultiFx) {
            Design child = d;
            child.family = Family::Delay;
            children_[0] = audiofx::prepare(child, s);
            child.family = Family::Eq;
            children_[1] = audiofx::prepare(child, s);
            child.family = Family::Diode;
            children_[2] = audiofx::prepare(child, s);
            for (auto &c : children_)
                info_.latencySamples += c->info().latencySamples;
        }
        if (d.family == Family::Room) {
            std::array<double, 3> sizes{d.physical.roomX, d.physical.roomY, d.physical.roomZ},
                source{sizes[0] * .3, sizes[1] * .4, sizes[2] * .5},
                receiver{sizes[0] * .6, sizes[1] * .5, sizes[2] * .5};
            for (unsigned j = 0; j < 7; ++j) {
                auto image = source;
                if (j) {
                    unsigned axis = (j - 1) / 2;
                    image[axis] = (j % 2) ? -source[axis] : 2 * sizes[axis] - source[axis];
                }
                double distance = 0;
                for (unsigned k = 0; k < 3; ++k)
                    distance += std::pow(image[k] - receiver[k], 2);
                distance = std::sqrt(distance);
                roomDelays_[j] = distance * s.sampleRate / 343;
                roomGains_[j] = (j ? std::sqrt(1 - d.physical.absorption) : 1) / distance;
            }
        }
        if (d.family == Family::Speaker) {
            Matrix a{};
            double fs = s.sampleRate;
            // States [displacement m, velocity m/s, coil current A], held input V.
            a[0][1] = 1 / fs;
            a[1][0] = -1200 / .02 / fs;
            a[1][1] = -1.2 / .02 / fs;
            a[1][2] = 4 / .02 / fs;
            a[2][1] = -4 / .0006 / fs;
            a[2][2] = -6 / .0006 / fs;
            a[2][3] = 1 / .0006 / fs;
            speakerStep_ = exponential(a);
        }
        for (unsigned ch = 0; ch < s.channels; ++ch) {
            auto &c = channels_[ch];
            c.dry.prepare(info_.latencySamples + 2);
            // Delays are allocated only for families that need a transport/history buffer.
            switch (d.family) {
            case Family::Delay:
            case Family::Tape:
            case Family::Flanger:
            case Family::Chorus:
            case Family::Spring:
            case Family::Room:
            case Family::Spatial:
            case Family::Pitch:
            case Family::Granular:
                c.delay.prepare(std::size_t(s.sampleRate * 2.1));
                break;
            default:
                break;
            }
            c.hilbert.prepare();
            if (d.family == Family::Pitch)
                c.spectralPitch.prepare();
            if (d.family == Family::Diode || d.family == Family::Amplifier)
                c.oversampler.prepare(d.oversampling);
            if (d.family == Family::Fdn)
                c.fdn.prepare(s.sampleRate, p(Parameter::Decay));
            if (d.family == Family::Convolution)
                c.convolution.prepare(d.impulseResponse);
            if (d.family == Family::Spectral || d.family == Family::Restoration)
                c.stft.prepare();
            if (d.family == Family::Plate || d.family == Family::Resonator ||
                d.family == Family::Room)
                compileModes(c);
            if (d.family == Family::Vocoder)
                for (unsigned k = 0; k < 8; ++k) {
                    double f = std::min(s.sampleRate * .4, 200 * std::pow(30., double(k) / 7));
                    c.bands[k].bandpass(s.sampleRate, f, 2);
                    c.carriers[k].bandpass(s.sampleRate, f, 2);
                }
        }
        reset();
    }
    bool set(Parameter id, double value) noexcept override {
        if (unsigned(id) >= parameterCount || !std::isfinite(value) ||
            !supportsParameter(design_.family, id)) {
            ++diagnostics_.rejectedEvents;
            return false;
        }
        auto r = ranges[unsigned(id)];
        if (value < r.minimum || value > r.maximum) {
            ++diagnostics_.rejectedEvents;
            return false;
        }
        // Modal decay is a compiled design control; reject rather than silently ignore it.
        if (id == Parameter::Decay &&
            (design_.family == Family::Plate || design_.family == Family::Resonator ||
             design_.family == Family::Room)) {
            ++diagnostics_.rejectedEvents;
            return false;
        }
        target_[id] = value;
        if (id == Parameter::Freeze || id == Parameter::Bits)
            current_[id] = value;
        for (unsigned i = 0; i < 3; ++i)
            if (children_[i] &&
                supportsParameter(std::array{Family::Delay, Family::Eq, Family::Diode}[i], id))
                children_[i]->set(id, value);
        return true;
    }
    Frame tick(Frame input) noexcept override {
        for (std::size_t i = 0; i < parameterCount; ++i) {
            current_.values[i] =
                smoothing_ * current_.values[i] + (1 - smoothing_) * target_.values[i];
            if (std::abs(current_.values[i] - target_.values[i]) < 1e-12)
                current_.values[i] = target_.values[i];
        }
        for (unsigned ch = 0; ch < spec_.channels; ++ch)
            if (!std::isfinite(input[ch])) {
                input[ch] = 0;
                ++diagnostics_.invalidSamples;
            } else if (std::abs(input[ch]) > 32) {
                input[ch] = std::clamp(input[ch], -32., 32.);
                ++diagnostics_.invalidSamples;
            }
        if (design_.family == Family::MultiFx) {
            for (auto &child : children_)
                input = child->tick(input);
            ++time_;
            return input;
        }
        bool filterChange = p(Parameter::Frequency) != lastFrequency_ ||
                            p(Parameter::Q) != lastQ_ || p(Parameter::GainDb) != lastGain_;
        if (filterChange) {
            for (unsigned ch = 0; ch < spec_.channels; ++ch) {
                channels_[ch].filter.peak(spec_.sampleRate, p(Parameter::Frequency),
                                          p(Parameter::Q), p(Parameter::GainDb));
                channels_[ch].extra.lowpass(spec_.sampleRate * (design_.family == Family::Amplifier
                                                                    ? design_.oversampling
                                                                    : 1),
                                            std::min(6000., spec_.sampleRate * .4));
            }
            lastFrequency_ = p(Parameter::Frequency);
            lastQ_ = p(Parameter::Q);
            lastGain_ = p(Parameter::GainDb);
        }
        if (design_.family == Family::Fdn && p(Parameter::Decay) != lastDecay_) {
            for (unsigned ch = 0; ch < spec_.channels; ++ch)
                channels_[ch].fdn.damping(spec_.sampleRate, p(Parameter::Decay));
            lastDecay_ = p(Parameter::Decay);
        }
        Frame output{};
        double fs = spec_.sampleRate, lfo = std::sin(2 * pi * phase_),
               mono = (input[0] + (spec_.channels == 2 ? input[1] : input[0])) * .5;
        for (unsigned ch = 0; ch < spec_.channels; ++ch) {
            auto &c = channels_[ch];
            double x = input[ch], y = x;
            double dry = info_.latencySamples ? c.dry.read(info_.latencySamples) : x;
            c.dry.push(x);
            switch (design_.family) {
            case Family::Delay:
                y = c.delay.read(p(Parameter::Delay) * fs);
                c.delay.push(x + p(Parameter::Feedback) * y);
                break;
            case Family::Tape: {
                double u = p(Parameter::Drive) * x;
                c.play = std::clamp(c.play, u - .12, u + .12);
                double mag = .85 * std::tanh(u) + .15 * std::tanh(c.play);
                double a = std::exp(-2 * pi * std::min(6000., fs * .4) / fs);
                c.head = a * c.head + (1 - a) * mag;
                y = c.delay.read((p(Parameter::Delay) + p(Parameter::Depth) * .2 * lfo) * fs);
                c.delay.push(c.dc.tick(c.head, fs) + p(Parameter::Feedback) * y);
                break;
            }
            case Family::Bbd: {
                double clock = std::clamp(256 / (2 * p(Parameter::Delay)), 10., fs * 8),
                       a = std::exp(-2 * pi * std::min(clock * .4, fs * .4) / fs);
                c.head = a * c.head + (1 - a) * (x + p(Parameter::Feedback) * c.hold);
                c.clockPhase += 2 * clock / fs;
                while (c.clockPhase >= 1) {
                    c.clockPhase -= 1;
                    c.hold = c.buckets[c.bucket];
                    c.buckets[c.bucket] = c.head;
                    c.bucket = (c.bucket + 1) % 256;
                }
                c.opto1 = a * c.opto1 + (1 - a) * c.hold;
                y = c.opto1;
                break;
            }
            case Family::Flanger: {
                double depth = std::min(p(Parameter::Depth), .019) * fs;
                y = c.delay.read(info_.latencySamples + depth * lfo);
                c.delay.push(x + p(Parameter::Feedback) * y);
                break;
            }
            case Family::Chorus:
                y = 0;
                for (unsigned voice = 0; voice < 3; ++voice)
                    y += c.delay.read(fs * (.018 + p(Parameter::Depth) *
                                                       std::sin(2 * pi * phase_ +
                                                                voice * 2 * pi / 3 + ch * .6))) /
                         3;
                c.delay.push(x);
                break;
            case Family::Phaser: {
                y = x + p(Parameter::Feedback) * c.feedback;
                double f = cutoff(p(Parameter::Frequency) * (1 + .7 * lfo), fs),
                       g = std::tan(pi * f / fs), a = (g - 1) / (g + 1);
                for (unsigned k = 0; k < 4; ++k)
                    y = c.allpasses[k].tick(y, a);
                c.feedback = y;
                break;
            }
            case Family::Tremolo:
                y = x * (1 - p(Parameter::Depth) * 10 + p(Parameter::Depth) * 10 * lfo);
                break;
            case Family::Ring:
                y = x * std::cos(2 * pi * c.pitchPhase);
                c.pitchPhase = wrap(c.pitchPhase + p(Parameter::Frequency) / fs);
                break;
            case Family::FrequencyShift: {
                auto h = c.hilbert.tick(x);
                y = h[0] * std::cos(2 * pi * c.pitchPhase) - h[1] * std::sin(2 * pi * c.pitchPhase);
                c.pitchPhase = wrap(c.pitchPhase + p(Parameter::Frequency) / fs);
                break;
            }
            case Family::Eq:
                y = c.filter.tick(x);
                break;
            case Family::Wah:
                y = c.svf.tick(x, fs, p(Parameter::Frequency), p(Parameter::Q))[1];
                break;
            case Family::Compressor: {
                double power =
                    c.detector.tick(x * x, fs, p(Parameter::Attack), p(Parameter::Release));
                double level = db(std::sqrt(std::max(0., power)));
                double reduction = std::min(0., (p(Parameter::ThresholdDb) - level) *
                                                    (1 - 1 / p(Parameter::Ratio)));
                double control =
                    c.control.tick(-reduction, fs, p(Parameter::Attack), p(Parameter::Release));
                y = x * gain(-control);
                break;
            }
            case Family::Optical: {
                double light = std::max(0., db(std::abs(x)) - p(Parameter::ThresholdDb));
                double a = std::exp(
                    -1 / (fs * (light > c.opto1 ? p(Parameter::Attack) : p(Parameter::Release))));
                c.opto1 = a * c.opto1 + (1 - a) * light;
                double b = std::exp(-1 / (fs * (light > c.opto2 ? p(Parameter::Attack) * 2
                                                                : p(Parameter::Release) * 4)));
                c.opto2 = b * c.opto2 + (1 - b) * light;
                y = x * gain(-(.7 * c.opto1 + .3 * c.opto2) * (1 - 1 / p(Parameter::Ratio)));
                break;
            }
            case Family::Vca: {
                double level = db(c.feedback),
                       excess = std::max(0., level - p(Parameter::ThresholdDb));
                double control =
                    c.control.tick(excess, fs, p(Parameter::Attack), p(Parameter::Release));
                y = x * std::exp(-std::log(10.) / 20 * control * (p(Parameter::Ratio) - 1));
                c.feedback = std::abs(y);
                break;
            }
            case Family::Transient: {
                double f =
                    c.fast.tick(std::abs(x), fs, p(Parameter::Attack), p(Parameter::Release));
                double s = c.slow.tick(std::abs(x), fs, p(Parameter::Attack) * 4,
                                       p(Parameter::Release) * 4);
                double g = std::clamp(
                    p(Parameter::Drive) * (std::log(f + 1e-9) - std::log(s + 1e-9)), -2., 2.);
                y = x * std::exp(g);
                break;
            }
            case Family::Diode:
                y = c.oversampler.tick(x, [&](double u) noexcept {
                    return c.diode.tick(u * p(Parameter::Drive), fs * design_.oversampling);
                });
                break;
            case Family::Amplifier: {
                y = c.oversampler.tick(x, [&](double u) noexcept {
                    double internal = fs * design_.oversampling;
                    double target = 1 / (1 + .2 * std::abs(c.feedback));
                    c.supply += (target - c.supply) * (1 - std::exp(-1 / (internal * .1)));
                    double grid = .8 + p(Parameter::Drive) * u;
                    double current = std::pow(std::max(0., grid), 1.5) - std::pow(.8, 1.5);
                    double processed = c.adaa.tick(-current * c.supply);
                    processed = c.extra.tick(c.dc.tick(processed, internal));
                    c.feedback = processed;
                    return processed;
                });
                break;
            }
            case Family::Speaker: {
                std::array<double, 3> next{};
                for (unsigned i = 0; i < 3; ++i) {
                    next[i] = speakerStep_[i][3] * x;
                    for (unsigned j = 0; j < 3; ++j)
                        next[i] += speakerStep_[i][j] * c.speaker[j];
                }
                c.speaker = next;
                y = next[1];
                break;
            }
            case Family::Fdn:
                y = c.fdn.tick(x);
                break;
            case Family::Convolution:
                y = c.convolution.tick(x);
                break;
            case Family::Room:
                y = 0;
                for (unsigned k = 0; k < 7; ++k)
                    y += roomGains_[k] * c.delay.read(roomDelays_[k]);
                c.delay.push(x);
                y += c.modes.tick(x);
                break;
            case Family::Plate:
            case Family::Resonator:
                y = c.modes.tick(x);
                break;
            case Family::Spring: {
                double delayed = c.delay.read(p(Parameter::Delay) * fs);
                for (unsigned k = 0; k < 12; ++k)
                    delayed = c.allpasses[k].tick(delayed, .15 + .7 * double(k) / 11);
                double loss = std::pow(10., -3 * p(Parameter::Delay) / p(Parameter::Decay));
                y = c.extra.tick(delayed);
                c.delay.push(x + loss * y);
                break;
            }
            case Family::Spatial: {
                double theta = 2 * pi * phase_, ear = ch ? 1. : -1.;
                double distance = std::sqrt(1 + .09 * .09 - 2 * .09 * ear * std::sin(theta));
                y = c.delay.read(fs * distance / 343) * (1 + .3 * std::cos(theta + ear * .4)) /
                    distance;
                c.delay.push(mono);
                break;
            }
            case Family::Pitch:
                y = c.spectralPitch.tick(x, std::pow(2., p(Parameter::Pitch) / 12), fs);
                break;
            case Family::Granular: {
                if (time_ % 128 == 0) {
                    unsigned voice = unsigned((time_ / 128) % 4);
                    c.grainAge[voice] = 0;
                    c.grainDelay[voice] = 64 + rand() * 1024;
                }
                y = 0;
                double r = std::pow(2., p(Parameter::Pitch) / 12);
                for (unsigned k = 0; k < 4; ++k)
                    if (c.grainAge[k] < 512) {
                        double age = c.grainAge[k]++;
                        y += .5 * std::pow(std::sin(pi * age / 512), 2) *
                             c.delay.read(c.grainDelay[k] + 512 + (1 - r) * age);
                    }
                if (p(Parameter::Freeze) < .5)
                    c.delay.push(x);
                break;
            }
            case Family::Vocoder: {
                double carrier = 0;
                for (unsigned harmonic = 1; harmonic <= 8; ++harmonic)
                    carrier += std::sin(2 * pi * c.pitchPhase * harmonic) / harmonic;
                carrier *= .3;
                c.pitchPhase = wrap(c.pitchPhase + 120 / fs);
                y = 0;
                for (unsigned k = 0; k < 8; ++k) {
                    double env =
                        c.bandEnvelopes[k].tick(std::abs(c.bands[k].tick(x)), fs,
                                                p(Parameter::Attack), p(Parameter::Release));
                    y += env * c.carriers[k].tick(carrier) * 4;
                }
                break;
            }
            case Family::Spectral:
                y = c.stft.tick(x, p(Parameter::Freeze) >= .5);
                break;
            case Family::Crusher: {
                c.clockPhase += 1 / (1 + p(Parameter::Delay) * fs * .05);
                if (c.clockPhase >= 1) {
                    c.clockPhase -= 1;
                    double step = std::exp2(1 - std::round(p(Parameter::Bits)));
                    c.hold = std::clamp(step * std::round(x / step), -1., 1 - step);
                }
                y = c.hold;
                break;
            }
            case Family::Restoration:
                y = c.stft.tick(x, false, std::pow(gain(p(Parameter::ThresholdDb)), 2) * 256);
                break;
            case Family::MultiFx:
                break;
            }
            output[ch] = finite(dry * (1 - p(Parameter::Mix)) + y * p(Parameter::Mix));
        }
        phase_ = wrap(phase_ + p(Parameter::Rate) / fs);
        ++time_;
        return output;
    }
    void reset() noexcept override {
        for (unsigned ch = 0; ch < spec_.channels; ++ch)
            channels_[ch].reset();
        for (auto &c : children_)
            if (c)
                c->reset();
        current_ = target_;
        phase_ = 0;
        time_ = 0;
        random_ = design_.seed ? design_.seed : 1;
        diagnostics_ = {};
        lastFrequency_ = -1;
        lastQ_ = -1;
        lastGain_ = 1e9;
        lastDecay_ = -1;
    }
    NodeInfo info() const noexcept override {
        return info_;
    }
    Diagnostics diagnostics() const noexcept override {
        auto d = diagnostics_;
        for (unsigned ch = 0; ch < spec_.channels; ++ch)
            d.solverFailures += channels_[ch].diode.failures;
        for (auto &c : children_)
            if (c) {
                auto n = c->diagnostics();
                d.rejectedEvents += n.rejectedEvents;
                d.invalidSamples += n.invalidSamples;
                d.solverFailures += n.solverFailures;
            }
        return d;
    }
};
} // namespace
bool supportsParameter(Family f, Parameter p) noexcept {
    if (unsigned(f) < 1 || unsigned(f) > 32 || unsigned(p) >= parameterCount)
        return false;
    if (p == Parameter::Mix)
        return true;
    using P = Parameter;
    switch (f) {
    case Family::Delay:
        return p == P::Delay || p == P::Feedback;
    case Family::Tape:
        return p == P::Delay || p == P::Feedback || p == P::Drive || p == P::Rate || p == P::Depth;
    case Family::Bbd:
        return p == P::Delay || p == P::Feedback;
    case Family::Flanger:
        return p == P::Feedback || p == P::Rate || p == P::Depth;
    case Family::Chorus:
    case Family::Tremolo:
        return p == P::Rate || p == P::Depth;
    case Family::Phaser:
        return p == P::Frequency || p == P::Rate || p == P::Feedback;
    case Family::Ring:
    case Family::FrequencyShift:
        return p == P::Frequency;
    case Family::Eq:
        return p == P::Frequency || p == P::Q || p == P::GainDb;
    case Family::Wah:
        return p == P::Frequency || p == P::Q;
    case Family::Compressor:
    case Family::Optical:
    case Family::Vca:
        return p == P::ThresholdDb || p == P::Ratio || p == P::Attack || p == P::Release;
    case Family::Transient:
        return p == P::Drive || p == P::Attack || p == P::Release;
    case Family::Diode:
    case Family::Amplifier:
        return p == P::Drive;
    case Family::Fdn:
        return p == P::Decay;
    case Family::Spring:
        return p == P::Delay || p == P::Decay;
    case Family::Spatial:
        return p == P::Rate;
    case Family::Pitch:
        return p == P::Pitch;
    case Family::Granular:
        return p == P::Pitch || p == P::Freeze;
    case Family::Vocoder:
        return p == P::Attack || p == P::Release;
    case Family::Spectral:
        return p == P::Freeze;
    case Family::Crusher:
        return p == P::Bits || p == P::Delay;
    case Family::Restoration:
        return p == P::ThresholdDb;
    case Family::MultiFx:
        return p == P::Delay || p == P::Feedback || p == P::Frequency || p == P::Q ||
               p == P::GainDb || p == P::Drive;
    default:
        return false;
    }
}
Range parameterRange(Parameter p) noexcept {
    return unsigned(p) < parameterCount ? ranges[unsigned(p)] : Range{0, 0, "invalid"};
}
std::string_view familyName(Family f) noexcept {
    return unsigned(f) >= 1 && unsigned(f) <= 32 ? names[unsigned(f) - 1] : "invalid";
}
std::unique_ptr<Node> prepare(const Design &d, const ProcessSpec &s) {
    return std::make_unique<Effect>(d, s);
}

struct Graph::Impl {
    ProcessSpec spec;
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::uint32_t> order, arrivals;
    std::vector<Frame> frames, previous;
    struct Route {
        Edge edge;
        std::array<DelayLine, 2> compensation;
        std::uint32_t delay{};
    };
    std::vector<Route> routes;
    std::uint32_t output{}, latency{};
    Diagnostics stats{};
};
Graph::Graph() = default;
Graph::~Graph() = default;
Graph::Graph(Graph &&) noexcept = default;
Graph &Graph::operator=(Graph &&) noexcept = default;
void Graph::prepare(const GraphDesign &d, const ProcessSpec &s) {
    if (d.nodes.empty() || d.nodes.size() > 128 || d.output >= d.nodes.size() ||
        d.edges.size() > 4096)
        throw std::invalid_argument("invalid graph size/output");
    auto next = std::make_unique<Impl>();
    next->spec = s;
    next->output = d.output;
    for (auto &design : d.nodes)
        next->nodes.push_back(audiofx::prepare(design, s));
    auto n = d.nodes.size();
    std::vector<unsigned> degree(n, 0);
    for (auto e : d.edges) {
        if (e.from >= n || e.to >= n || !std::isfinite(e.gain) || std::abs(e.gain) > 16)
            throw std::invalid_argument("invalid edge");
        if (!e.delayed)
            ++degree[e.to];
        next->routes.push_back({e, {}, 0});
    }
    for (std::size_t i = 0; i < n; ++i)
        if (degree[i] == 0)
            next->order.push_back(std::uint32_t(i));
    for (std::size_t head = 0; head < next->order.size(); ++head)
        for (auto e : d.edges)
            if (!e.delayed && e.from == next->order[head] && --degree[e.to] == 0)
                next->order.push_back(e.to);
    if (next->order.size() != n)
        throw std::invalid_argument("cycle requires an explicit delayed edge");
    next->arrivals.assign(n, 0);
    next->frames.resize(n);
    next->previous.resize(n);
    for (auto id : next->order) {
        std::uint32_t arrival = 0;
        for (auto e : d.edges)
            if (!e.delayed && e.to == id)
                arrival = std::max(arrival, next->arrivals[e.from]);
        next->arrivals[id] = arrival + next->nodes[id]->info().latencySamples;
        for (auto &r : next->routes)
            if (!r.edge.delayed && r.edge.to == id) {
                r.delay = arrival - next->arrivals[r.edge.from];
                for (unsigned ch = 0; ch < s.channels; ++ch)
                    r.compensation[ch].prepare(r.delay + 2);
            }
    }
    next->latency = next->arrivals[d.output];
    impl_ = std::move(next);
}
void Graph::reset() noexcept {
    if (!impl_)
        return;
    for (auto &n : impl_->nodes)
        n->reset();
    for (auto &r : impl_->routes)
        for (auto &l : r.compensation)
            l.reset();
    std::fill(impl_->frames.begin(), impl_->frames.end(), Frame{});
    std::fill(impl_->previous.begin(), impl_->previous.end(), Frame{});
    impl_->stats = {};
}
bool Graph::process(const double *const *input, double *const *output, std::uint32_t samples,
                    std::span<const Event> events) noexcept {
    if (!impl_ || samples > impl_->spec.maximumBlock)
        return false;
    auto &g = *impl_;
    if (samples) {
        if (!input || !output)
            return false;
        for (unsigned ch = 0; ch < g.spec.channels; ++ch)
            if (!input[ch] || !output[ch])
                return false;
    }
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events[i].offset >= samples || events[i].node >= g.nodes.size() ||
            (i && events[i].offset < events[i - 1].offset)) {
            ++g.stats.rejectedEvents;
            return false;
        }
    std::size_t event = 0;
    for (std::uint32_t t = 0; t < samples; ++t) {
        while (event < events.size() && events[event].offset == t) {
            const auto &e = events[event++];
            g.nodes[e.node]->set(e.parameter, e.value);
        }
        Frame original{};
        for (unsigned ch = 0; ch < g.spec.channels; ++ch)
            original[ch] = input[ch][t];
        for (auto id : g.order) {
            Frame x{};
            bool incoming = false;
            for (auto &r : g.routes)
                if (r.edge.to == id) {
                    if (!r.edge.delayed)
                        incoming = true;
                    for (unsigned ch = 0; ch < g.spec.channels; ++ch) {
                        double value = r.edge.delayed ? g.previous[r.edge.from][ch]
                                                      : g.frames[r.edge.from][ch];
                        if (r.delay) {
                            double delayed = r.compensation[ch].read(r.delay);
                            r.compensation[ch].push(value);
                            value = delayed;
                        }
                        x[ch] += r.edge.gain * value;
                    }
                }
            if (!incoming)
                for (unsigned ch = 0; ch < g.spec.channels; ++ch)
                    x[ch] += original[ch];
            g.frames[id] = g.nodes[id]->tick(x);
        }
        for (unsigned ch = 0; ch < g.spec.channels; ++ch)
            output[ch][t] = g.frames[g.output][ch];
        std::copy(g.frames.begin(), g.frames.end(), g.previous.begin());
    }
    return true;
}
std::uint32_t Graph::latencySamples() const noexcept {
    return impl_ ? impl_->latency : 0;
}
Diagnostics Graph::diagnostics() const noexcept {
    Diagnostics d{};
    if (!impl_)
        return d;
    d = impl_->stats;
    for (auto &n : impl_->nodes) {
        auto a = n->diagnostics();
        d.rejectedEvents += a.rejectedEvents;
        d.invalidSamples += a.invalidSamples;
        d.solverFailures += a.solverFailures;
    }
    return d;
}
} // namespace audiofx
