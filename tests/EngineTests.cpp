// Unit tests for the theory and engine code. Run: cmake -B build-tests tests && cmake --build build-tests && ./build-tests/engine_tests
#include "../Source/Engine.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace cw;

static int failures = 0, checks = 0;

#define CHECK(cond) do { ++checks; if (! (cond)) { ++failures; std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { ++checks; const auto _a = (a); const auto _b = (b); if (! (_a == _b)) { ++failures; \
    std::printf ("FAIL %s:%d  %s == %s  (got \"%s\" vs \"%s\")\n", __FILE__, __LINE__, #a, #b, toStr (_a).c_str(), toStr (_b).c_str()); } } while (0)

static std::string toStr (const std::string& s) { return s; }
static std::string toStr (const char* s) { return s; }
template <typename T> static std::string toStr (const T& v) { return std::to_string (v); }

static int findProgression (const std::string& name)
{
    const auto& lib = progressionLibrary();
    for (size_t i = 0; i < lib.size(); ++i)
        if (lib[i].name == name) return (int) i;
    std::printf ("missing progression %s\n", name.c_str());
    return 0;
}

static std::string names (const std::string& prog, int key, ColorMode color = ColorMode::AsWritten)
{
    const auto& p = progressionLibrary()[(size_t) findProgression (prog)];
    std::string out;
    for (const auto& c : p.chords)
    {
        if (! out.empty()) out += " ";
        out += chordName (applyColor (c, color, p.minor), key, p.minor);
    }
    return out;
}

static std::set<int> pcsOf (const std::vector<int>& notes)
{
    std::set<int> s;
    for (int n : notes) s.insert (n % 12);
    return s;
}

int main()
{
    // Library parses (throws otherwise) and has every genre.
    const auto& lib = progressionLibrary();
    CHECK (lib.size() >= 35);
    for (const auto& g : genreNames())
    {
        int count = 0;
        for (const auto& p : lib) if (p.genre == g) ++count;
        CHECK (count >= 6);
    }
    for (const auto& p : lib)
        CHECK (p.chords.size() >= 2 && p.chords.size() <= 8);

    // Transposing keeps the progression pattern.
    CHECK_EQ (names ("Axis", 0), "C G Am F");
    CHECK_EQ (names ("Axis", 4), "E B C#m A");
    CHECK_EQ (names ("Axis", 3), "Eb Bb Cm Ab");
    CHECK_EQ (names ("Axis", 6), "Gb Db Ebm B");
    CHECK_EQ (names ("Andalusian", 9), "Am G F E");
    CHECK_EQ (names ("Andalusian", 0), "Cm Bb Ab G");
    CHECK_EQ (names ("Andalusian", 4), "Em D C B");
    CHECK_EQ (names ("Just the two of us", 1), "Gbmaj7 F7 Bbm7 Abm7 Db7");
    CHECK_EQ (names ("Gospel walk-up", 0), "C C7 F F#dim7 C/G Am7 Dm7 G7");
    CHECK_EQ (names ("Creep", 7), "G B C Cm");
    CHECK_EQ (names ("Mixolydian rock", 2), "D C G D");
    CHECK_EQ (names ("Boom bap jazz", 9), "Am7 Fmaj7 Bm7b5 E7");
    CHECK_EQ (names ("Dorian groove", 9), "Am7 D9");
    CHECK_EQ (names ("Tritone sub", 0), "Dm9 Db7 Cmaj9 Cmaj9");

    // Color modes.
    CHECK_EQ (names ("Axis", 0, ColorMode::Sevenths), "Cmaj7 G7 Am7 Fmaj7");
    CHECK_EQ (names ("Axis", 0, ColorMode::Ninths), "Cmaj9 G9 Am9 Fmaj9");
    CHECK_EQ (names ("Axis", 0, ColorMode::Lush), "Cmaj13 G13 Am11 Fmaj13");
    CHECK_EQ (names ("Andalusian", 9, ColorMode::Sevenths), "Am7 G7 Fmaj7 E7");
    CHECK_EQ (names ("Andalusian", 9, ColorMode::Ninths), "Am9 G9 Fmaj9 E7b9");
    CHECK_EQ (names ("Two-five-one", 0, ColorMode::Triads), "Dm G C C");
    CHECK_EQ (names ("Doo-wop", 7, ColorMode::Sevenths), "Gmaj7 Em7 Cmaj7 D7");

    // Parser edge cases.
    Chord c;
    CHECK (! parseNumeral ("X", c));
    CHECK (! parseNumeral ("Vq", c));
    CHECK (! parseNumeral ("Vi", c)); // mixed case is not a numeral
    CHECK (parseNumeral ("vii°", c) && c.triad == Triad::Dim && c.rootOffset == 11);
    CHECK (parseNumeral ("iiø7", c) && c.triad == Triad::Dim && c.seventh == 10);
    CHECK (parseNumeral ("V7b9", c) && c.ninthAlt == -1 && c.seventh == 10);
    CHECK (parseNumeral ("IVmaj7#11", c) && c.eleventhAlt == 1 && chordName (c, 0, false) == "Fmaj7#11");
    CHECK (parseNumeral ("Vsus4", c) && chordName (c, 0, false) == "Gsus4");
    CHECK (parseNumeral ("I69", c) && chordName (c, 0, false) == "C6/9");
    CHECK (parseNumeral ("iadd9", c) && chordName (c, 9, true) == "Amadd9");

    // Numerals follow the color setting and match the written numerals when untouched.
    for (const auto& p : lib)
        for (const auto& ch : p.chords)
            CHECK_EQ (numeralName (ch), ch.text);
    {
        const auto& p = progressionLibrary()[(size_t) findProgression ("Just the two of us")];
        CHECK_EQ (numeralName (applyColor (p.chords[0], ColorMode::Ninths, false)), "IVmaj9");
        CHECK_EQ (numeralName (applyColor (p.chords[2], ColorMode::Ninths, false)), "vi9");
        CHECK_EQ (numeralName (applyColor (p.chords[2], ColorMode::Triads, false)), "vi");
        Chord d;
        parseNumeral ("vii°", d);
        CHECK_EQ (numeralName (applyColor (d, ColorMode::Sevenths, false)), "vii\xC3\xB8" "7");
    }

    // Diatonic sets.
    CHECK_EQ ((int) diatonicChords (false).size(), 12);
    CHECK_EQ ((int) diatonicChords (true).size(), 12);

    // Voicings contain the right notes, stay in range and voice lead smoothly.
    for (int style = 0; style < 4; ++style)
    {
        for (int color = 0; color < 5; ++color)
        {
            for (const auto& p : lib)
            {
                for (int key = 0; key < 12; ++key)
                {
                    VoicingSettings vs;
                    vs.style = (VoicingStyle) style;
                    Voicing prev;
                    for (const auto& ch : p.chords)
                    {
                        const Chord colored = applyColor (ch, (ColorMode) color, p.minor);
                        const Voicing v = voiceChord (colored, key, vs, prev.empty() ? nullptr : &prev);
                        CHECK (! v.upper.empty());
                        if (v.upper.empty()) continue;
                        CHECK (v.upper.front() >= 33 && v.upper.back() <= 98);
                        CHECK (v.bass >= 24 && v.bass < v.upper.front());
                        CHECK ((int) v.upper.size() <= 5);
                        // The third (or sus tone) is always present: it defines the chord.
                        const auto ints = chordIntervals (colored);
                        const int rootPc = (key + colored.rootOffset) % 12;
                        CHECK (pcsOf (v.upper).count ((rootPc + ints[1]) % 12) == 1);
                        // Every upper note belongs to the chord.
                        std::set<int> allowed;
                        for (int i : ints) allowed.insert ((rootPc + i) % 12);
                        for (int n : v.upper) CHECK (allowed.count (n % 12) == 1);
                        prev = v;
                    }
                }
            }
        }
    }

    // Voice leading moves less than jumping between root positions.
    {
        const auto& p = progressionLibrary()[(size_t) findProgression ("Axis")];
        VoicingSettings led, unled;
        unled.voiceLeading = false;
        Voicing a, b;
        int moveLed = 0, moveUnled = 0;
        for (int round = 0; round < 2; ++round)
        {
            for (const auto& ch : p.chords)
            {
                Voicing na = voiceChord (ch, 0, led, a.empty() ? nullptr : &a);
                Voicing nb = voiceChord (ch, 0, unled, b.empty() ? nullptr : &b);
                if (! a.empty()) moveLed += std::abs (na.upper.back() - a.upper.back()) + std::abs (na.upper.front() - a.upper.front());
                if (! b.empty()) moveUnled += std::abs (nb.upper.back() - b.upper.back()) + std::abs (nb.upper.front() - b.upper.front());
                a = na; b = nb;
            }
        }
        CHECK (moveLed < moveUnled);
        std::printf ("voice leading: outer-voice movement %d (led) vs %d (root position)\n", moveLed, moveUnled);
    }

    // Engine timing.
    {
        Engine e;
        EngineSettings s;
        s.progression = findProgression ("Axis");
        s.pattern = 0; // sustain
        s.beatsPerChord = 4;
        e.setSettings (s);
        CHECK (std::abs (e.cycleBeats() - 16.0) < 1e-9);

        std::vector<Hit> hits;
        e.hitsInRange (0, 16, hits);
        CHECK_EQ ((int) hits.size(), 4);
        for (size_t i = 0; i < hits.size(); ++i)
        {
            CHECK (std::abs (hits[i].start - 4.0 * (double) i) < 1e-9);
            CHECK_EQ (hits[i].slot, (int) i);
        }

        // Split windows produce the same hits as one window (no gaps, no doubles).
        for (int pattern = 0; pattern < (int) patternLibrary().size(); ++pattern)
        {
            s.pattern = pattern;
            s.swing = 0.6;
            e.setSettings (s);
            std::vector<Hit> whole, parts;
            e.hitsInRange (-3.0, 40.0, whole);
            double t = -3.0;
            const double steps[] = { 0.37, 1.1, 0.013, 2.5, 0.25 };
            int k = 0;
            while (t < 40.0)
            {
                const double next = std::min (40.0, t + steps[k++ % 5]);
                e.hitsInRange (t, next, parts);
                t = next;
            }
            CHECK_EQ (whole.size(), parts.size());
            for (size_t i = 0; i < std::min (whole.size(), parts.size()); ++i)
                CHECK (std::abs (whole[i].start - parts[i].start) < 1e-9);
        }

        // Swing pushes the off-beat 8th toward the triplet position.
        s.pattern = 3; // eighth pulse
        s.swing = 1.0;
        e.setSettings (s);
        hits.clear();
        e.hitsInRange (0, 1, hits);
        CHECK_EQ ((int) hits.size(), 2);
        if (hits.size() == 2)
            CHECK (std::abs (hits[1].start - (0.5 + 1.0 / 6.0)) < 1e-9);

        // Starting playback mid-chord picks up the sustained chord.
        s.pattern = 0;
        s.swing = 0;
        e.setSettings (s);
        hits.clear();
        e.ongoingHits (6.0, hits);
        CHECK_EQ ((int) hits.size(), 1);
        if (hits.size() == 1)
        {
            CHECK_EQ (hits[0].slot, 1);
            CHECK (std::abs (hits[0].length - 2.0) < 1e-9);
        }

        CHECK_EQ (e.slotAt (13.0), 3);
        CHECK_EQ (e.slotAt (16.5), 0);
        CHECK_EQ (e.slotAt (-0.5), 3);

        // Canon uses half-bar chords: 8 chords x 0.5 x 4 beats = 16.
        s.progression = findProgression ("Canon");
        e.setSettings (s);
        CHECK (std::abs (e.cycleBeats() - 16.0) < 1e-9);
    }

    // Humanizer.
    {
        Humanizer h;
        HumanizeSettings hs;
        hs.timingMs = 12;
        hs.strumMs = 30;
        std::vector<NoteEvent> ev;
        for (int i = 0; i < 2000; ++i)
            h.chord ({ 48, 60, 64, 67, 71 }, true, 1.0, 96, i % 2 == 1, 1.0, false, hs, ev);
        bool anyNegative = false, velOk = true;
        for (const auto& e : ev)
        {
            if (e.onMs < 0) anyNegative = true;
            if (e.velocity < 1 || e.velocity > 127) velOk = false;
        }
        CHECK (! anyNegative);
        CHECK (velOk);

        // Down strum: low notes first. Up strum: high notes first.
        HumanizeSettings strum;
        strum.timingMs = 0; strum.strumMs = 40; strum.velocityVariation = 0; strum.lengthVariation = 0;
        strum.strumDir = StrumDirection::Down;
        ev.clear();
        h.chord ({ 48, 60, 64, 67 }, true, 1.0, 96, false, 1.0, true, strum, ev);
        CHECK (ev[0].onMs < ev[3].onMs);
        strum.strumDir = StrumDirection::Up;
        ev.clear();
        h.chord ({ 48, 60, 64, 67 }, true, 1.0, 96, false, 1.0, true, strum, ev);
        CHECK (ev[0].onMs > ev[3].onMs);

        // Everything off means the grid, untouched.
        HumanizeSettings off;
        off.timingMs = 0; off.strumMs = 0; off.velocityVariation = 0; off.topVoice = 0; off.lengthVariation = 0;
        ev.clear();
        h.chord ({ 60, 64, 67 }, false, 1.0, 100, false, 1.0, true, off, ev);
        for (const auto& e : ev)
        {
            CHECK (std::abs (e.onMs) < 1e-9);
            CHECK_EQ (e.velocity, 100);
            CHECK (std::abs (e.lengthBeats - 1.0) < 1e-9);
        }

        // Top voice sings over the inner voices.
        HumanizeSettings top = off;
        top.topVoice = 1.0;
        ev.clear();
        h.chord ({ 48, 60, 64, 67 }, true, 1.0, 90, false, 1.0, true, top, ev);
        CHECK (ev[3].velocity > ev[1].velocity && ev[3].velocity > ev[2].velocity);
    }

    // Print a few voicings so a human can eyeball them.
    {
        auto show = [] (const std::string& prog, int key, ColorMode color, VoicingStyle style) {
            const auto& p = progressionLibrary()[(size_t) findProgression (prog)];
            VoicingSettings vs;
            vs.style = style;
            Voicing prev;
            std::printf ("%-20s key %-2s:", prog.c_str(), noteName (key, false).c_str());
            for (const auto& ch : p.chords)
            {
                const Chord cc = applyColor (ch, color, p.minor);
                const Voicing v = voiceChord (cc, key, vs, prev.empty() ? nullptr : &prev);
                std::printf ("  %s[", chordName (cc, key, p.minor).c_str());
                for (int n : v.allNotes()) std::printf (" %s%d", noteName (n % 12, keyPrefersFlats (key, p.minor)).c_str(), n / 12 - 1);
                std::printf (" ]");
                prev = v;
            }
            std::printf ("\n");
        };
        show ("Axis", 0, ColorMode::AsWritten, VoicingStyle::Close);
        show ("Axis", 0, ColorMode::Ninths, VoicingStyle::Open);
        show ("Two-five-one", 5, ColorMode::AsWritten, VoicingStyle::Rootless);
        show ("Just the two of us", 1, ColorMode::AsWritten, VoicingStyle::Close);
        show ("Gospel walk-up", 0, ColorMode::AsWritten, VoicingStyle::Close);
        show ("Anthem minor", 9, ColorMode::Lush, VoicingStyle::Wide);
    }

    std::printf ("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
