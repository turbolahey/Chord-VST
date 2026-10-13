// Chordwright: music theory core (no JUCE dependency, unit tested in tests/).
#pragma once

#include <string>
#include <vector>

namespace cw
{

enum class Triad { Major, Minor, Dim, Aug, Sus2, Sus4 };

// One chord written as a Roman numeral relative to the key's tonic.
// Because everything is stored relative to the tonic, moving to a new key
// keeps the exact same progression pattern and chord qualities.
struct Chord
{
    int degree = 1;          // 1..7
    int accidental = 0;      // -1 flat, +1 sharp (relative to the major scale)
    int rootOffset = 0;      // semitones above the tonic, 0..11
    Triad triad = Triad::Major;
    int seventh = 0;         // 0 none, 9 dim7, 10 minor 7th, 11 major 7th
    bool sixth = false;
    bool ninth = false;
    bool eleventh = false;
    bool thirteenth = false;
    int ninthAlt = 0;        // -1 b9, +1 #9
    int eleventhAlt = 0;     // +1 #11
    int thirteenthAlt = 0;   // -1 b13
    bool hasSlash = false;
    int bassOffset = 0;      // semitones above tonic for slash chords
    std::string text;        // the numeral as written, e.g. "IVmaj7"

    bool operator== (const Chord&) const = default;
};

enum class ColorMode { AsWritten = 0, Triads, Sevenths, Ninths, Lush };

// Parses numerals such as: I, vi, bVII, #iv°7, iiø7, V7, Imaj9, ii9, V13,
// IV6, I69, Iadd9, Vsus4, V7b9, I/V, IVmaj7#11. Returns false on bad input.
bool parseNumeral (const std::string& text, Chord& out);

// Parses a space-separated list of numerals.
bool parseProgression (const std::string& text, std::vector<Chord>& out);

bool scaleContains (int semitonesAboveTonic, bool minorKey);

// Adds or strips extensions to match the chosen color.
Chord applyColor (const Chord& c, ColorMode mode, bool minorKey);

// Semitones above the chord root, ascending, root included.
std::vector<int> chordIntervals (const Chord& c);

std::string noteName (int pitchClass, bool flats);
bool keyPrefersFlats (int keyRoot, bool minorKey);
std::string chordName (const Chord& c, int keyRoot, bool minorKey);

// Roman numeral for a chord after color changes, e.g. IVmaj7 with 9ths becomes IVmaj9.
std::string numeralName (const Chord& c);

// ---------------------------------------------------------------- voicing
enum class VoicingStyle { Close = 0, Open, Wide, Rootless };

struct VoicingSettings
{
    VoicingStyle style = VoicingStyle::Close;
    int center = 60;          // MIDI note the voicing gravitates toward
    bool voiceLeading = true;
    bool bassNote = true;
    int bassCenter = 40;      // where the bass note sits
    int maxVoices = 5;

    bool operator== (const VoicingSettings&) const = default;
};

struct Voicing
{
    std::vector<int> upper;   // ascending MIDI notes
    int bass = -1;            // -1 when there is no separate bass note

    std::vector<int> allNotes() const;
    bool empty() const { return upper.empty(); }
    bool operator== (const Voicing&) const = default;
};

Voicing voiceChord (const Chord& chord, int keyRoot, const VoicingSettings& vs, const Voicing* previous);

} // namespace cw
