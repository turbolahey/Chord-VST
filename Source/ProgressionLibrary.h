// Chordwright: progression library. Chord progressions are stored as Roman
// numerals so any of them can be played in any of the 12 keys.
#pragma once

#include "Theory.h"

#include <string>
#include <vector>

namespace cw
{

struct Progression
{
    std::string name;            // short nickname
    std::string genre;           // one of genreNames()
    bool minor = false;          // tonic chord is minor
    std::string numerals;        // space separated Roman numerals
    std::vector<double> weights; // length of each chord relative to "chord length" (empty = all 1)
    std::string heardIn;         // well-known songs built on the same changes
    std::vector<Chord> chords;   // parsed numerals

    double weight (size_t i) const { return i < weights.size() ? weights[i] : 1.0; }
    std::string displayName() const;
};

const std::vector<std::string>& genreNames();
const std::vector<Progression>& progressionLibrary();

// 12 chords for "chord keys" mode: 7 white keys then 5 black keys (C#, D#, F#, G#, A#).
const std::vector<Chord>& diatonicChords (bool minorKey);
const std::vector<std::string>& diatonicNumerals (bool minorKey);

} // namespace cw
