#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace audiofx::dsp {
inline constexpr double pi = std::numbers::pi;
inline double finite(double x) noexcept {
    return std::isfinite(x) && std::abs(x) > 1e-300 ? x : 0.;
}
inline double db(double x) noexcept {
    return 20 * std::log10(std::max(std::abs(x), 1e-15));
}
inline double gain(double x) noexcept {
    return std::pow(10., x / 20);
}
inline double cutoff(double f, double fs) noexcept {
    return std::clamp(f, 1., fs * .45);
}
inline double wrap(double p) noexcept {
    return p - std::floor(p);
}

class DelayLine {
    std::vector<double> data_;
    std::size_t pos_{};

  public:
    void prepare(std::size_t maximum) {
        data_.assign(std::max<std::size_t>(maximum + 2, 4), 0.);
        pos_ = 0;
    }
    void reset() noexcept {
        std::fill(data_.begin(), data_.end(), 0.);
        pos_ = 0;
    }
    void push(double x) noexcept {
        data_[pos_] = finite(x);
        pos_ = (pos_ + 1) % data_.size();
    }
    // Read before pushing the current sample: d=1 means the previous sample.
    double read(double d) const noexcept {
        d = std::clamp(d, 1., double(data_.size() - 2));
        auto n = std::size_t(d);
        double f = d - double(n);
        auto a = (pos_ + data_.size() - n) % data_.size();
        auto b = (a + data_.size() - 1) % data_.size();
        return data_[a] * (1 - f) + data_[b] * f;
    }
    double energy(std::size_t length) const noexcept {
        double e = 0;
        for (std::size_t n = 1; n <= length; ++n) {
            double x = data_[(pos_ + data_.size() - n) % data_.size()];
            e += x * x;
        }
        return e;
    }
};

struct Biquad {
    double b0{1}, b1{}, b2{}, a1{}, a2{}, s1{}, s2{};
    void reset() noexcept {
        s1 = s2 = 0;
    }
    double tick(double x) noexcept {
        double y = b0 * x + s1;
        s1 = finite(b1 * x - a1 * y + s2);
        s2 = finite(b2 * x - a2 * y);
        return finite(y);
    }
    void assign(double B0, double B1, double B2, double A0, double A1, double A2) noexcept {
        b0 = B0 / A0;
        b1 = B1 / A0;
        b2 = B2 / A0;
        a1 = A1 / A0;
        a2 = A2 / A0;
    }
    void peak(double fs, double f, double q, double g) noexcept {
        double w = 2 * pi * cutoff(f, fs) / fs, A = std::pow(10., g / 40),
               al = std::sin(w) / (2 * q), c = -2 * std::cos(w);
        assign(1 + al * A, c, 1 - al * A, 1 + al / A, c, 1 - al / A);
    }
    void lowpass(double fs, double f, double q = .7071067811865476) noexcept {
        double w = 2 * pi * cutoff(f, fs) / fs, c = std::cos(w), al = std::sin(w) / (2 * q);
        assign((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandpass(double fs, double f, double q = 2) noexcept {
        double w = 2 * pi * cutoff(f, fs) / fs, al = std::sin(w) / (2 * q);
        assign(al, 0, -al, 1 + al, -2 * std::cos(w), 1 - al);
    }
};
struct Svf {
    double s1{}, s2{};
    void reset() noexcept {
        s1 = s2 = 0;
    }
    std::array<double, 3> tick(double x, double fs, double f, double q) noexcept {
        double g = std::tan(pi * cutoff(f, fs) / fs), k = 1 / q;
        double v1 = (s1 + g * (x - s2)) / (1 + g * k + g * g), v2 = s2 + g * v1;
        s1 = finite(2 * v1 - s1);
        s2 = finite(2 * v2 - s2);
        return {finite(v2), finite(v1), finite(x - k * v1 - v2)};
    }
};
struct Envelope {
    double value{};
    void reset() noexcept {
        value = 0;
    }
    double tick(double target, double fs, double attack, double release) noexcept {
        double a = std::exp(-1 / (fs * (target > value ? attack : release)));
        value = finite(a * value + (1 - a) * target);
        return value;
    }
};
struct DcBlock {
    double x{}, y{};
    void reset() noexcept {
        x = y = 0;
    }
    double tick(double u, double fs) noexcept {
        double v = u - x + std::exp(-2 * pi * 10 / fs) * y;
        x = u;
        y = finite(v);
        return y;
    }
};
struct Allpass {
    double x{}, y{};
    void reset() noexcept {
        x = y = 0;
    }
    double tick(double u, double a) noexcept {
        double v = a * u + x - a * y;
        x = u;
        y = finite(v);
        return y;
    }
};
struct DiodeRc {
    double voltage{};
    unsigned failures{}, maximumIterations{};
    double R{2200}, C{10e-9}, Is{1e-9}, thermal{.026};
    void reset() noexcept {
        voltage = 0;
        failures = maximumIterations = 0;
    }
    double tick(double u, double fs) noexcept {
        // Backward Euler. Residual is strictly increasing for positive components.
        double lo = std::min({voltage, u, 0.}) - 1e-9, hi = std::max({voltage, u, 0.}) + 1e-9;
        double v = std::clamp(voltage, lo, hi);
        bool converged = false;
        unsigned it = 0;
        for (; it < 32; ++it) {
            double argument = std::clamp(v / thermal, -80., 80.);
            double r = C * fs * (v - voltage) + (v - u) / R + 2 * Is * std::sinh(argument);
            if (std::abs(r) < 1e-12) {
                converged = true;
                break;
            }
            if (r > 0)
                hi = v;
            else
                lo = v;
            double derivative = C * fs + 1 / R + 2 * Is / thermal * std::cosh(argument);
            double next = v - r / derivative;
            v = next > lo && next < hi ? next : (lo + hi) / 2;
        }
        maximumIterations = std::max(maximumIterations, it);
        if (!converged) {
            ++failures;
            v = (lo + hi) / 2;
        }
        voltage = finite(v);
        return voltage;
    }
};
struct AdaaTanh {
    double previous{};
    static double primitive(double x) noexcept {
        return std::abs(x) + std::log1p(std::exp(-2 * std::abs(x))) - std::log(2.);
    }
    void reset() noexcept {
        previous = 0;
    }
    double tick(double x) noexcept {
        double d = x - previous;
        double y = std::abs(d) > 1e-7 ? (primitive(x) - primitive(previous)) / d
                                      : std::tanh((x + previous) / 2);
        previous = x;
        return y;
    }
};
struct WdfRc {
    double reflected{}, R{2200}, C{100e-9};
    void reset() noexcept {
        reflected = 0;
    }
    double tick(double u, double fs) noexcept {
        double rp = 1 / (2 * fs * C);
        double a = (2 * rp * u + (R - rp) * reflected) / (R + rp);
        double v = (a + reflected) / 2;
        reflected = a;
        return finite(v);
    }
};

struct Mode {
    double q{}, v{}, omega{}, sigma{}, input{}, output{}, a00{}, a01{}, a10{}, a11{}, b0{}, b1{};
    void prepare(double fs, double w, double loss, double in, double out) {
        if (!(w > 0 && loss >= 0))
            throw std::invalid_argument("positive frequency and nonnegative loss required");
        omega = w;
        sigma = loss;
        input = in;
        output = out;
        double discriminant = w * w - loss * loss, e = std::exp(-loss / fs), s = 1 / fs, c = 1;
        if (discriminant > 1e-12) {
            double wd = std::sqrt(discriminant);
            s = std::sin(wd / fs) / wd;
            c = std::cos(wd / fs);
        } else if (discriminant < -1e-12) {
            double wd = std::sqrt(-discriminant);
            s = std::sinh(wd / fs) / wd;
            c = std::cosh(wd / fs);
        }
        a00 = e * (c + loss * s);
        a01 = e * s;
        a10 = -w * w * a01;
        a11 = e * (c - loss * s);
        b0 = in * (1 - a00) / (w * w);
        b1 = -in * a10 / (w * w);
        reset();
    }
    void reset() noexcept {
        q = v = 0;
    }
    double tick(double u) noexcept {
        double nq = a00 * q + a01 * v + b0 * u;
        v = finite(a10 * q + a11 * v + b1 * u);
        q = finite(nq);
        return output * q;
    }
    double energy() const noexcept {
        return (v * v + omega * omega * q * q) / 2;
    }
};
class ModalBank {
    std::vector<Mode> modes_;

  public:
    void add(double fs, double frequency, double damping, double input, double output) {
        if (frequency >= fs * .45)
            return;
        Mode m;
        m.prepare(fs, 2 * pi * frequency, damping, input, output);
        modes_.push_back(m);
    }
    void clear() {
        modes_.clear();
    }
    void reset() noexcept {
        for (auto &m : modes_)
            m.reset();
    }
    double tick(double u) noexcept {
        double y = 0;
        for (auto &m : modes_)
            y += m.tick(u);
        return finite(y);
    }
    std::size_t size() const noexcept {
        return modes_.size();
    }
};
class Fdn {
    std::array<DelayLine, 4> lines_;
    std::array<double, 4> lengths_{}, gains_{};

  public:
    void prepare(double fs, double decay) {
        constexpr std::array<double, 4> times{.031, .043, .059, .071};
        for (std::size_t i = 0; i < 4; ++i) {
            lengths_[i] = std::round(fs * times[i]);
            lines_[i].prepare(std::size_t(lengths_[i]));
        }
        damping(fs, decay);
    }
    void damping(double fs, double decay) noexcept {
        for (std::size_t i = 0; i < 4; ++i)
            gains_[i] = std::pow(10., -3 * lengths_[i] / (fs * decay));
    }
    void reset() noexcept {
        for (auto &l : lines_)
            l.reset();
    }
    double tick(double x) noexcept {
        std::array<double, 4> r{};
        double sum = 0, y = 0;
        for (std::size_t i = 0; i < 4; ++i) {
            r[i] = lines_[i].read(lengths_[i]) * gains_[i];
            sum += r[i];
            y += (i % 2 ? -1 : 1) * r[i] * .5;
        }
        for (std::size_t i = 0; i < 4; ++i)
            lines_[i].push(x * .5 + r[i] - sum * .5);
        return y;
    }
    double energy() const noexcept {
        double e = 0;
        for (std::size_t i = 0; i < 4; ++i)
            e += lines_[i].energy(std::size_t(lengths_[i]));
        return e;
    }
};

class Fft {
    std::size_t n_{};
    std::vector<std::size_t> reversed_;
    std::vector<std::complex<double>> roots_;

  public:
    void prepare(std::size_t n) {
        if (n < 2 || (n & (n - 1)))
            throw std::invalid_argument("FFT size must be a power of two");
        n_ = n;
        reversed_.resize(n);
        roots_.resize(n / 2);
        unsigned bits = 0;
        for (auto v = n; v > 1; v >>= 1)
            ++bits;
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t r = 0, v = i;
            for (unsigned b = 0; b < bits; ++b) {
                r = (r << 1) | (v & 1);
                v >>= 1;
            }
            reversed_[i] = r;
        }
        for (std::size_t i = 0; i < n / 2; ++i)
            roots_[i] = std::polar(1., -2 * pi * double(i) / double(n));
    }
    void transform(std::complex<double> *x, bool inverse = false) const noexcept {
        for (std::size_t i = 0; i < n_; ++i)
            if (i < reversed_[i])
                std::swap(x[i], x[reversed_[i]]);
        for (std::size_t size = 2; size <= n_; size *= 2)
            for (std::size_t start = 0; start < n_; start += size)
                for (std::size_t j = 0; j < size / 2; ++j) {
                    auto w = roots_[j * n_ / size];
                    if (inverse)
                        w = std::conj(w);
                    auto u = x[start + j], v = x[start + j + size / 2] * w;
                    x[start + j] = u + v;
                    x[start + j + size / 2] = u - v;
                }
        if (inverse)
            for (std::size_t i = 0; i < n_; ++i)
                x[i] /= double(n_);
    }
};
class Convolver {
    Fft fft_;
    std::size_t p_{}, q_{}, fill_{}, out_{}, head_{};
    std::vector<std::complex<double>> kernels_, history_, work_;
    std::vector<double> input_, output_, overlap_;

  public:
    void prepare(const std::vector<double> &ir, std::size_t partition = 128) {
        if (ir.empty())
            throw std::invalid_argument("empty IR");
        p_ = partition;
        q_ = (ir.size() + p_ - 1) / p_;
        fft_.prepare(2 * p_);
        kernels_.assign(q_ * 2 * p_, {});
        history_ = kernels_;
        work_.assign(2 * p_, {});
        input_.assign(p_, 0);
        output_ = input_;
        overlap_ = input_;
        for (std::size_t j = 0; j < q_; ++j) {
            for (std::size_t k = 0; k < p_ && j * p_ + k < ir.size(); ++k)
                kernels_[j * 2 * p_ + k] = ir[j * p_ + k];
            fft_.transform(kernels_.data() + j * 2 * p_);
        }
        reset();
    }
    void reset() noexcept {
        fill_ = out_ = head_ = 0;
        std::fill(history_.begin(), history_.end(), std::complex<double>{});
        std::fill(input_.begin(), input_.end(), 0);
        std::fill(output_.begin(), output_.end(), 0);
        std::fill(overlap_.begin(), overlap_.end(), 0);
    }
    double tick(double x) noexcept {
        double y = output_[out_];
        out_ = (out_ + 1) % p_;
        input_[fill_++] = x;
        if (fill_ == p_) {
            auto *h = history_.data() + head_ * 2 * p_;
            for (std::size_t k = 0; k < 2 * p_; ++k)
                h[k] = k < p_ ? input_[k] : 0.;
            fft_.transform(h);
            std::fill(work_.begin(), work_.end(), std::complex<double>{});
            for (std::size_t j = 0; j < q_; ++j) {
                auto index = (head_ + q_ - j) % q_;
                for (std::size_t k = 0; k < 2 * p_; ++k)
                    work_[k] += history_[index * 2 * p_ + k] * kernels_[j * 2 * p_ + k];
            }
            fft_.transform(work_.data(), true);
            for (std::size_t k = 0; k < p_; ++k) {
                output_[k] = finite(work_[k].real() + overlap_[k]);
                overlap_[k] = finite(work_[k + p_].real());
            }
            head_ = (head_ + 1) % q_;
            fill_ = 0;
        }
        return y;
    }
    std::size_t latency() const noexcept {
        return p_;
    }
};

// Windowed-sinc interpolation and antialias decimation. Fixed work, prepared storage.
class Oversampler {
    unsigned factor_{1};
    std::size_t upPos_{}, downPos_{};
    std::vector<double> kernel_, up_, down_;
    double filter(std::vector<double> &history, std::size_t &pos, double x) noexcept {
        history[pos] = x;
        double y = 0;
        for (std::size_t i = 0; i < kernel_.size(); ++i)
            y += kernel_[i] * history[(pos + history.size() - i) % history.size()];
        pos = (pos + 1) % history.size();
        return y;
    }

  public:
    void prepare(unsigned factor) {
        if (factor != 1 && factor != 2 && factor != 4 && factor != 8)
            throw std::invalid_argument("oversampling factor must be 1,2,4,8");
        factor_ = factor;
        std::size_t length = 16 * factor + 1;
        kernel_.resize(length);
        up_.assign(length, 0);
        down_ = up_;
        double sum = 0, fc = .45 / factor;
        for (std::size_t i = 0; i < length; ++i) {
            double m = double(i) - double(length - 1) / 2;
            double value = m == 0 ? 2 * fc : std::sin(2 * pi * fc * m) / (pi * m);
            value *= .42 - .5 * std::cos(2 * pi * double(i) / double(length - 1)) +
                     .08 * std::cos(4 * pi * double(i) / double(length - 1));
            kernel_[i] = value;
            sum += value;
        }
        for (auto &x : kernel_)
            x /= sum;
        reset();
    }
    void reset() noexcept {
        std::fill(up_.begin(), up_.end(), 0);
        std::fill(down_.begin(), down_.end(), 0);
        upPos_ = downPos_ = 0;
    }
    template <class F> double tick(double x, F nonlinear) noexcept {
        if (factor_ == 1)
            return nonlinear(x);
        double output = 0;
        for (unsigned phase = 0; phase < factor_; ++phase) {
            double interpolated = filter(up_, upPos_, phase == 0 ? x : 0) * factor_;
            double y = filter(down_, downPos_, nonlinear(interpolated));
            if (phase == 0)
                output = y;
        }
        return finite(output);
    }
    unsigned latency() const noexcept {
        return factor_ > 1 ? 16 : 0;
    }
};

class Hilbert {
    static constexpr std::size_t size = 63;
    std::array<double, size> coefficients_{}, history_{};
    std::size_t pos_{};

  public:
    void prepare() noexcept {
        for (std::size_t k = 0; k < size; ++k) {
            int m = int(k) - 31;
            coefficients_[k] = (m % 2 ? 2 / (pi * m) : 0.) *
                               (.5 - .5 * std::cos(2 * pi * double(k) / double(size - 1)));
        }
        reset();
    }
    void reset() noexcept {
        history_.fill(0);
        pos_ = 0;
    }
    std::array<double, 2> tick(double x) noexcept {
        history_[pos_] = x;
        double quadrature = 0;
        for (std::size_t k = 0; k < size; ++k)
            quadrature += coefficients_[k] * history_[(pos_ + size - k) % size];
        double real = history_[(pos_ + size - 31) % size];
        pos_ = (pos_ + 1) % size;
        return {real, quadrature};
    }
};

// Peak/phase analysis with bounded additive resynthesis. A reduced pitch model,
// not a transient-locked full-spectrum phase vocoder or arbitrary time stretcher.
class SpectralPitch {
    static constexpr std::size_t N = 1024, H = 256, V = 16;
    struct Voice {
        double frequency{}, amplitude{}, target{}, phase{};
    };
    struct Peak {
        double frequency{}, amplitude{}, phase{}, strength{};
    };
    Fft fft_;
    std::array<double, N> input_{}, window_{};
    std::array<double, N / 2 + 1> previous_{};
    std::array<std::complex<double>, N> work_{};
    std::array<Voice, V> voices_{};
    std::size_t clock_{};

  public:
    void prepare() {
        fft_.prepare(N);
        for (std::size_t k = 0; k < N; ++k)
            window_[k] = .5 - .5 * std::cos(2 * pi * double(k) / N);
        reset();
    }
    void reset() noexcept {
        input_.fill(0);
        previous_.fill(0);
        voices_.fill({});
        clock_ = 0;
    }
    double tick(double x, double ratio, double fs) noexcept {
        double identity = input_[clock_ % N];
        input_[clock_ % N] = x;
        double output = 0;
        for (auto &voice : voices_) {
            voice.amplitude += (voice.target - voice.amplitude) / 32;
            output += voice.amplitude * std::cos(voice.phase);
            voice.phase =
                std::remainder(voice.phase + 2 * pi * voice.frequency * ratio / fs, 2 * pi);
        }
        if ((clock_ + 1) % H == 0 && clock_ + 1 >= N) {
            for (std::size_t k = 0; k < N; ++k)
                work_[k] = input_[(clock_ + 1 + k) % N] * window_[k];
            fft_.transform(work_.data());
            std::array<Peak, V> peaks{};
            for (std::size_t k = 2; k < N / 2 - 1; ++k) {
                double mag = std::abs(work_[k]);
                if (mag < std::abs(work_[k - 1]) || mag <= std::abs(work_[k + 1]) || mag < 1e-8)
                    continue;
                double left = std::log(std::max(std::abs(work_[k - 1]), 1e-20)),
                       centre = std::log(mag),
                       right = std::log(std::max(std::abs(work_[k + 1]), 1e-20));
                double delta =
                           std::clamp(.5 * (left - right) / (left - 2 * centre + right), -.5, .5),
                       phase = std::arg(work_[k]);
                double frequency = (double(k) + delta) * fs / N;
                if (clock_ + 1 > N) {
                    double expected = 2 * pi * double(k) * H / N;
                    double increment =
                        expected + std::remainder(phase - previous_[k] - expected, 2 * pi);
                    frequency = increment * fs / (2 * pi * H);
                }
                if (frequency <= 0 || frequency * ratio >= fs * .45)
                    continue;
                double response = std::abs(delta) < 1e-8
                                      ? 1
                                      : std::sin(pi * delta) / (pi * delta * (1 - delta * delta));
                Peak peak{frequency, 4 * mag / (N * response), phase - 2 * pi * delta * .5, mag};
                for (std::size_t j = 0; j < V; ++j)
                    if (peak.strength > peaks[j].strength) {
                        for (std::size_t m = V - 1; m > j; --m)
                            peaks[m] = peaks[m - 1];
                        peaks[j] = peak;
                        break;
                    }
            }
            for (std::size_t k = 0; k <= N / 2; ++k)
                previous_[k] = std::arg(work_[k]);
            std::array<bool, V> assigned{};
            for (auto &voice : voices_)
                voice.target = 0;
            for (const auto &peak : peaks) {
                if (peak.strength == 0)
                    continue;
                std::size_t best = V;
                double distance = fs;
                for (std::size_t j = 0; j < V; ++j)
                    if (!assigned[j]) {
                        double d = std::abs(voices_[j].frequency - peak.frequency);
                        if (d < distance) {
                            distance = d;
                            best = j;
                        }
                    }
                if (best == V)
                    break;
                if (distance > fs / N * 2) {
                    for (std::size_t j = 0; j < V; ++j)
                        if (!assigned[j] && voices_[j].amplitude < 1e-8) {
                            best = j;
                            break;
                        }
                    voices_[best].phase = peak.phase;
                }
                assigned[best] = true;
                voices_[best].frequency = peak.frequency;
                voices_[best].target = peak.amplitude;
            }
        }
        ++clock_;
        return std::abs(ratio - 1) < 1e-12 ? identity : finite(output);
    }
    static constexpr unsigned latency() noexcept {
        return N;
    }
};

class Stft {
    static constexpr std::size_t N = 256, H = 64;
    Fft fft_;
    std::array<double, N> input_{}, window_{}, frozenMagnitude_{}, phase_{}, previousPhase_{},
        phaseIncrement_{};
    std::array<double, 2 * N> output_{};
    std::array<std::complex<double>, N> work_{};
    std::size_t clock_{};
    bool captured_{};

  public:
    void prepare() {
        fft_.prepare(N);
        for (std::size_t i = 0; i < N; ++i)
            window_[i] = .5 - .5 * std::cos(2 * pi * double(i) / N);
        reset();
    }
    void reset() noexcept {
        input_.fill(0);
        output_.fill(0);
        frozenMagnitude_.fill(0);
        phase_.fill(0);
        previousPhase_.fill(0);
        phaseIncrement_.fill(0);
        clock_ = 0;
        captured_ = false;
    }
    double tick(double x, bool freeze = false, double noisePower = 0.) noexcept {
        auto index = clock_ % (2 * N);
        double y = output_[index];
        output_[index] = 0;
        input_[clock_ % N] = x;
        if ((clock_ + 1) % H == 0) {
            for (std::size_t k = 0; k < N; ++k)
                work_[k] = input_[(clock_ + 1 + k) % N] * window_[k];
            fft_.transform(work_.data());
            if (!freeze)
                captured_ = false;
            bool capture = freeze && !captured_;
            for (std::size_t k = 0; k <= N / 2; ++k) {
                double angle = std::arg(work_[k]), mag = std::abs(work_[k]);
                if (capture) {
                    frozenMagnitude_[k] = mag;
                    phase_[k] = angle;
                    double expected = 2 * pi * double(k) * H / N;
                    phaseIncrement_[k] =
                        expected + std::remainder(angle - previousPhase_[k] - expected, 2 * pi);
                } else if (freeze) {
                    phase_[k] = std::remainder(phase_[k] + phaseIncrement_[k], 2 * pi);
                }
                previousPhase_[k] = angle;
                if (freeze)
                    work_[k] = std::polar(frozenMagnitude_[k], phase_[k]);
                if (noisePower > 0) {
                    double power = std::norm(work_[k]);
                    double g = std::max(.05, 1 - noisePower / (power + 1e-20));
                    work_[k] *= g;
                }
                if (k > 0 && k < N / 2)
                    work_[N - k] = std::conj(work_[k]);
            }
            if (freeze)
                captured_ = true;
            work_[0] = {work_[0].real(), 0};
            work_[N / 2] = {work_[N / 2].real(), 0};
            fft_.transform(work_.data(), true);
            for (std::size_t k = 0; k < N; ++k)
                output_[(clock_ + 1 + k) % (2 * N)] += work_[k].real() * window_[k] / 1.5;
        }
        ++clock_;
        return finite(y);
    }
    static constexpr std::size_t latency() noexcept {
        return N;
    }
};
} // namespace audiofx::dsp
