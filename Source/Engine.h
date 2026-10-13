// Chordwright: rhythm patterns, humanizer and the progression player.
// No JUCE dependency, so the plugin and the unit tests share the same code.
#pragma once

#include "ProgressionLibrary.h"
#include "Theory.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace cw
{

// ---------------------------------------------------------------- patterns
struct PatternHit
{
    double pos;     // beats from the start of the bar
    double len;     // beats
    double accent;  // 0..1
    bool up;        // up-stroke for "alternate" strumming
};

struct PatternDef
{
    std::string name;
    std::vector<PatternHit> hits; // one 4-beat bar, repeated across long chords
    bool sustain = false;         // one hit lasting the whole chord
    int arp = 0;                  // 0 none, 1 up, 2 up and down
    bool guitarStrum = false;     // up-strokes skip the bass note
};

const std::vector<PatternDef>& patternLibrary();

// ---------------------------------------------------------------- humanizer
enum class StrumDirection { Down = 0, Up, Alternate, Random };

struct HumanizeSettings
{
    double timingMs = 8.0;          // size of the timing drift
    double laidBackMs = 0.0;        // positive plays behind the beat, negative ahead
    double strumMs = 18.0;          // time from first to last note of a chord
    StrumDirection strumDir = StrumDirection::Alternate;
    double velocityVariation = 0.3; // 0..1
    double topVoice = 0.4;          // 0..1, how much the top note sings over the rest
    double lengthVariation = 0.25;  // 0..1

    bool operator== (const HumanizeSettings&) const = default;
};

// A note relative to a nominal time. Offsets are in milliseconds so the result
// works at any tempo; lengths are in beats.
struct NoteEvent
{
    int note = 60;
    int velocity = 100;
    double onMs = 0.0;
    double lengthBeats = 1.0;
    double offMs = 0.0;
};

class Humanizer
{
public:
    explicit Humanizer (uint32_t seed = 0x5eedu);

    void reset();

    // notes: ascending, bass included. allowEarly = false for live playing,
    // where nothing can be played before the key is pressed.
    void chord (const std::vector<int>& notes, bool hasBass, double accent, int baseVelocity,
                bool upHint, double lengthBeats, bool allowEarly,
                const HumanizeSettings& hs, std::vector<NoteEvent>& out);

    void single (int note, double accent, int baseVelocity, double lengthBeats, bool allowEarly,
                 const HumanizeSettings& hs, std::vector<NoteEvent>& out);

    // Live chord keys: alternates strum direction between presses.
    bool nextLiveUpStroke (const HumanizeSettings& hs);

    static double maxEarlyMs (const HumanizeSettings& hs);

private:
    double gauss();
    double uniform01();
    double hitOffset (const HumanizeSettings& hs, bool allowEarly);
    int velocity (double base, const HumanizeSettings& hs);

    std::mt19937 rng;
    std::normal_distribution<double> normal { 0.0, 1.0 };
    std::uniform_real_distribution<double> uni { 0.0, 1.0 };
    double timingWalk = 0.0, velWalk = 0.0;
    bool liveUp = true;
    uint32_t seed;
};

// ---------------------------------------------------------------- engine
struct EngineSettings
{
    int keyRoot = 0;
    int progression = 0;
    ColorMode color = ColorMode::AsWritten;
    VoicingSettings voicing;
    int pattern = 0;
    double beatsPerChord = 4.0;
    double swing = 0.0;           // 0..1
    double gate = 0.96;           // 0.1..1
    int baseVelocity = 92;
    HumanizeSettings human;

    bool operator== (const EngineSettings&) const = default;
};

struct Hit
{
    double start = 0;     // absolute beats
    double length = 1;    // beats
    int slot = 0;         // chord index in the progression
    int index = 0;        // hit number inside the chord (drives arpeggios)
    double accent = 1.0;
    bool up = false;
};

class Engine
{
public:
    Engine();

    void setSettings (const EngineSettings& newSettings);
    const EngineSettings& settings() const { return s; }

    const Progression& progression() const;
    bool minorKey() const { return progression().minor; }
    int numSlots() const { return (int) progression().chords.size(); }
    double cycleBeats() const { return cycle; }
    double slotStart (int slot) const { return starts[(size_t) slot]; }
    double slotLength (int slot) const { return durations[(size_t) slot]; }
    int slotAt (double beat) const;

    // Hits whose start falls inside [from, to).
    void hitsInRange (double from, double to, std::vector<Hit>& out) const;
    // Hits already sounding at time t, trimmed to start at t (used when playback starts mid-bar).
    void ongoingHits (double t, std::vector<Hit>& out) const;

    // Turns a hit into notes using the voice-led progression voicings.
    void realizeHit (const Hit& hit, Humanizer& h, bool allowEarly, std::vector<NoteEvent>& out) const;

    Chord coloredSlotChord (int slot) const;
    Chord coloredDiatonicChord (int index) const;
    const Voicing& progressionVoicing (int slot) const { return autoVoicings[(size_t) slot]; }

    // Live playing keeps its own voice-leading memory.
    Voicing liveVoicing (const Chord& chord);
    void resetLiveVoicing() { lastLive = {}; }

private:
    void rebuild();

    EngineSettings s;
    std::vector<Voicing> autoVoicings;
    std::vector<double> starts, durations;
    double cycle = 4.0, maxDuration = 4.0;
    Voicing lastLive;
};

} // namespace cw
