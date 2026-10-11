#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace audiofx {
enum class Family : unsigned {
    Delay = 1,
    Tape,
    Bbd,
    Flanger,
    Chorus,
    Phaser,
    Tremolo,
    Ring,
    FrequencyShift,
    Eq,
    Wah,
    Compressor,
    Optical,
    Vca,
    Transient,
    Diode,
    Amplifier,
    Speaker,
    Fdn,
    Convolution,
    Room,
    Plate,
    Spring,
    Spatial,
    Pitch,
    Granular,
    Vocoder,
    Spectral,
    Crusher,
    Resonator,
    Restoration,
    MultiFx
};
enum class Parameter : unsigned {
    Mix,
    Delay,
    Feedback,
    Rate,
    Depth,
    Frequency,
    Q,
    GainDb,
    Drive,
    ThresholdDb,
    Ratio,
    Attack,
    Release,
    Decay,
    Pitch,
    Bits,
    Freeze,
    Count
};
inline constexpr std::size_t parameterCount = unsigned(Parameter::Count);
struct Range {
    double minimum, maximum;
    std::string_view unit;
};
Range parameterRange(Parameter) noexcept;
bool supportsParameter(Family, Parameter) noexcept;
struct Parameters {
    std::array<double, parameterCount> values{
        .5, .1, .35, .5, .003, 1000, .7071067811865476, 0, 1, -18, 4, .01, .1, 1.5, 0, 8, 0};
    double &operator[](Parameter p) noexcept {
        return values[unsigned(p)];
    }
    double operator[](Parameter p) const noexcept {
        return values[unsigned(p)];
    }
};
struct PhysicalDesign {
    double plateX{2}, plateY{1}, thickness{.0005}, density{7800}, young{2e11}, poisson{.3},
        tension{0};
    double inputX{.23}, inputY{.37}, outputX{.61}, outputY{.41}; // normalised positions
    double observationScale{10000}; // displacement metres -> normalised audio
    double roomX{6}, roomY{4}, roomZ{3}, absorption{.3};
    double stringLength{1}, stringTension{80}, linearDensity{.005}, bendingStiffness{.001};
    unsigned modes{64};
};
struct Design {
    Family family{Family::Delay};
    Parameters parameters{};
    PhysicalDesign physical{};
    std::vector<double> impulseResponse{1.};
    double smoothingSeconds{.01};
    std::uint64_t seed{74031};
    unsigned oversampling{1};
};
struct ProcessSpec {
    double sampleRate{48000};
    std::uint32_t maximumBlock{512}, channels{2};
};
struct Event {
    std::uint32_t offset{}, node{};
    Parameter parameter{};
    double value{};
};
struct NodeInfo {
    std::uint32_t latencySamples{};
    double tailSeconds{};
    std::string_view model;
    bool physical{};
};
struct Diagnostics {
    std::uint64_t rejectedEvents{}, invalidSamples{}, solverFailures{};
};
using Frame = std::array<double, 2>;
class Node {
  public:
    virtual ~Node() = default;
    virtual Frame tick(Frame) noexcept = 0;
    virtual bool set(Parameter, double) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual NodeInfo info() const noexcept = 0;
    virtual Diagnostics diagnostics() const noexcept = 0;
};
std::unique_ptr<Node> prepare(const Design &, const ProcessSpec &);
std::string_view familyName(Family) noexcept;

struct Edge {
    std::uint32_t from{}, to{};
    double gain{1};
    bool delayed{};
};
// A delayed edge explicitly inserts one sample. It is not an implicit analog loop.
struct GraphDesign {
    std::vector<Design> nodes;
    std::vector<Edge> edges;
    std::uint32_t output{};
};
class Graph {
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    Graph();
    ~Graph();
    Graph(Graph &&) noexcept;
    Graph &operator=(Graph &&) noexcept;
    Graph(const Graph &) = delete;
    Graph &operator=(const Graph &) = delete;
    void prepare(const GraphDesign &, const ProcessSpec &); // non-RT; strong exception guarantee
    void reset() noexcept;
    // Planar audio. in/out may alias. Null channel pointers are rejected without dereference.
    // Valid, sorted events at identical offsets execute in caller order. Unsorted events are
    // rejected.
    bool process(const double *const *in, double *const *out, std::uint32_t samples,
                 std::span<const Event> events = {}) noexcept;
    std::uint32_t latencySamples() const noexcept;
    Diagnostics diagnostics() const noexcept;
};
} // namespace audiofx
