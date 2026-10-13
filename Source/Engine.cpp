#include "Engine.h"

#include <algorithm>
#include <cmath>

namespace cw
{

// ---------------------------------------------------------------- patterns

const std::vector<PatternDef>& patternLibrary()
{
    static const std::vector<PatternDef> patterns = [] {
        std::vector<PatternDef> p;

        p.push_back ({ "Sustain", {}, true });
        p.push_back ({ "Half notes", { { 0, 2, 1.0, false }, { 2, 2, 0.85, false } } });
        p.push_back ({ "Quarter pulse", { { 0, 1, 1.0, false }, { 1, 1, 0.82, false }, { 2, 1, 0.9, false }, { 3, 1, 0.82, false } } });

        std::vector<PatternHit> eighths;
        for (int i = 0; i < 8; ++i)
        {
            const double accent = (i % 2) ? 0.72 : (i == 0 ? 1.0 : (i == 4 ? 0.92 : 0.85));
            eighths.push_back ({ i * 0.5, 0.5, accent, (i % 2) == 1 });
        }
        p.push_back ({ "Eighth pulse", eighths });

        p.push_back ({ "Offbeat stabs", { { 0.5, 0.25, 0.92, true }, { 1.5, 0.25, 0.82, true },
                                          { 2.5, 0.25, 0.9, true },  { 3.5, 0.25, 0.8, true } } });
        p.push_back ({ "Charleston", { { 0, 1.5, 1.0, false }, { 1.5, 2.5, 0.86, true } } });
        p.push_back ({ "Pop 3-3-2", { { 0, 1.5, 1.0, false }, { 1.5, 1.5, 0.86, true }, { 3, 1, 0.9, false } } });
        p.push_back ({ "Guitar strum", { { 0, 1, 1.0, false }, { 1, 0.5, 0.86, false }, { 1.5, 1, 0.7, true },
                                         { 2.5, 0.5, 0.72, true }, { 3, 0.5, 0.88, false }, { 3.5, 0.5, 0.7, true } },
                       false, 0, true });
        p.push_back ({ "Half-time stab", { { 0, 3, 1.0, false }, { 3.5, 0.5, 0.74, true } } });

        std::vector<PatternHit> shuffle;
        for (int b = 0; b < 4; ++b)
        {
            shuffle.push_back ({ (double) b, 0.667, b == 0 ? 1.0 : (b == 2 ? 0.9 : 0.82), false });
            shuffle.push_back ({ b + 0.667, 0.333, 0.62, true });
        }
        p.push_back ({ "Shuffle", shuffle });

        std::vector<PatternHit> arp8;
        for (int i = 0; i < 8; ++i)
            arp8.push_back ({ i * 0.5, 0.5, (i % 2) ? 0.74 : (i == 0 ? 1.0 : 0.86), false });
        p.push_back ({ "Arp up 8ths", arp8, false, 1 });

        std::vector<PatternHit> arp16;
        for (int i = 0; i < 16; ++i)
            arp16.push_back ({ i * 0.25, 0.25, (i % 4) == 0 ? 0.95 : 0.7, false });
        p.push_back ({ "Arp up and down 16ths", arp16, false, 2 });

        return p;
    }();
    return patterns;
}

static bool usesSixteenthGrid (const PatternDef& p)
{
    for (const auto& h : p.hits)
    {
        const double frac = h.pos - std::floor (h.pos);
        if (std::abs (frac - 0.25) < 1e-6 || std::abs (frac - 0.75) < 1e-6)
            return true;
    }
    return false;
}

static double swingOffset (double pos, double swing, bool sixteenths)
{
    const double frac = pos - std::floor (pos);
    if (sixteenths)
    {
        if (std::abs (frac - 0.25) < 1e-6 || std::abs (frac - 0.75) < 1e-6)
            return swing / 12.0;
        return 0.0;
    }
    return std::abs (frac - 0.5) < 1e-6 ? swing / 6.0 : 0.0;
}

// ---------------------------------------------------------------- humanizer

Humanizer::Humanizer (uint32_t s) : rng (s), seed (s) {}

void Humanizer::reset()
{
    rng.seed (seed);
    normal.reset();
    uni.reset();
    timingWalk = velWalk = 0.0;
    liveUp = true;
}

double Humanizer::gauss()     { return std::clamp (normal (rng), -3.0, 3.0); }
double Humanizer::uniform01() { return uni (rng); }

double Humanizer::maxEarlyMs (const HumanizeSettings& hs)
{
    return std::max (0.0, -hs.laidBackMs) + 3.0 * hs.timingMs + 2.0;
}

double Humanizer::hitOffset (const HumanizeSettings& hs, bool allowEarly)
{
    // A random walk drifts like a player instead of jumping like noise.
    timingWalk = 0.65 * timingWalk + 0.35 * gauss();
    if (allowEarly)
        return std::max (hs.laidBackMs + hs.timingMs * timingWalk, -maxEarlyMs (hs));
    return std::max (0.0, hs.laidBackMs) + hs.timingMs * std::abs (timingWalk) * 0.8;
}

int Humanizer::velocity (double v, const HumanizeSettings& hs)
{
    v += hs.velocityVariation * 14.0 * (0.6 * velWalk + 0.4 * gauss());
    return std::clamp ((int) std::lround (v), 1, 127);
}

void Humanizer::chord (const std::vector<int>& notes, bool hasBass, double accent, int baseVelocity,
                       bool upHint, double lengthBeats, bool allowEarly,
                       const HumanizeSettings& hs, std::vector<NoteEvent>& out)
{
    const int n = (int) notes.size();
    if (n == 0)
        return;

    bool up = false;
    switch (hs.strumDir)
    {
        case StrumDirection::Down:      up = false; break;
        case StrumDirection::Up:        up = true; break;
        case StrumDirection::Alternate: up = upHint; break;
        case StrumDirection::Random:    up = uniform01() < 0.5; break;
    }

    const double base = hitOffset (hs, allowEarly);
    const double strumTotal = hs.strumMs * (0.85 + 0.3 * uniform01()) * (up ? 0.75 : 1.0);
    velWalk = 0.7 * velWalk + 0.3 * gauss();
    const double chordLen = lengthBeats * std::clamp (1.0 + hs.lengthVariation * 0.2 * gauss(), 0.6, 1.15);
    const double baseVel = baseVelocity * (0.4 + 0.6 * accent);

    for (int i = 0; i < n; ++i)
    {
        const int order = up ? (n - 1 - i) : i;
        const double frac = n > 1 ? (double) order / (double) (n - 1) : 0.0;
        double jitter = hs.timingMs * 0.15 * gauss();
        if (! allowEarly)
            jitter = std::abs (jitter);

        double on = base + strumTotal * std::pow (frac, 0.9) + jitter;
        on = allowEarly ? std::max (on, -maxEarlyMs (hs)) : std::max (on, 0.0);

        double v = baseVel;
        const bool isTop = i == n - 1;
        const bool isBass = hasBass && i == 0;
        if (n >= 3)
        {
            if (isTop)       v += hs.topVoice * 14.0;
            else if (isBass) v -= hs.topVoice * 2.0;
            else             v -= hs.topVoice * 5.0;
        }
        if (up) v -= 5.0;
        if (hs.strumMs > 4.0) v -= 0.6 * order;

        NoteEvent e;
        e.note = notes[(size_t) i];
        e.velocity = velocity (v, hs);
        e.onMs = on;
        e.lengthBeats = chordLen * std::clamp (1.0 + hs.lengthVariation * 0.08 * gauss(), 0.85, 1.1);
        e.offMs = (allowEarly ? base * 0.5 : 0.0) + hs.timingMs * 0.25 * gauss();
        out.push_back (e);
    }
}

void Humanizer::single (int note, double accent, int baseVelocity, double lengthBeats, bool allowEarly,
                        const HumanizeSettings& hs, std::vector<NoteEvent>& out)
{
    const double base = hitOffset (hs, allowEarly);
    velWalk = 0.7 * velWalk + 0.3 * gauss();

    NoteEvent e;
    e.note = note;
    e.velocity = velocity (baseVelocity * (0.4 + 0.6 * accent), hs);
    e.onMs = base;
    e.lengthBeats = lengthBeats * std::clamp (1.0 + hs.lengthVariation * 0.2 * gauss(), 0.6, 1.15);
    e.offMs = hs.timingMs * 0.25 * gauss();
    out.push_back (e);
}

bool Humanizer::nextLiveUpStroke (const HumanizeSettings& hs)
{
    switch (hs.strumDir)
    {
        case StrumDirection::Down:   return false;
        case StrumDirection::Up:     return true;
        case StrumDirection::Random: return uniform01() < 0.5;
        case StrumDirection::Alternate:
        default:
            liveUp = ! liveUp;
            return liveUp;
    }
}

// ---------------------------------------------------------------- engine

Engine::Engine() { rebuild(); }

const Progression& Engine::progression() const
{
    return progressionLibrary()[(size_t) s.progression];
}

void Engine::setSettings (const EngineSettings& newSettings)
{
    if (newSettings == s && ! autoVoicings.empty())
        return;
    s = newSettings;
    rebuild();
}

Chord Engine::coloredSlotChord (int slot) const
{
    const auto& p = progression();
    return applyColor (p.chords[(size_t) slot], s.color, p.minor);
}

Chord Engine::coloredDiatonicChord (int index) const
{
    const bool minor = minorKey();
    return applyColor (diatonicChords (minor)[(size_t) index], s.color, minor);
}

void Engine::rebuild()
{
    const auto& lib = progressionLibrary();
    s.progression = std::clamp (s.progression, 0, (int) lib.size() - 1);
    s.pattern = std::clamp (s.pattern, 0, (int) patternLibrary().size() - 1);
    s.beatsPerChord = std::max (0.25, s.beatsPerChord);

    const auto& p = progression();
    const size_t n = p.chords.size();

    starts.clear();
    durations.clear();
    double t = 0.0;
    maxDuration = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double d = p.weight (i) * s.beatsPerChord;
        starts.push_back (t);
        durations.push_back (d);
        t += d;
        maxDuration = std::max (maxDuration, d);
    }
    cycle = t;

    // Voice-lead around the loop twice so the last chord flows back into the first.
    autoVoicings.assign (n, {});
    Voicing prev;
    for (int pass = 0; pass < 2; ++pass)
    {
        for (size_t i = 0; i < n; ++i)
        {
            const Voicing v = voiceChord (coloredSlotChord ((int) i), s.keyRoot, s.voicing, prev.empty() ? nullptr : &prev);
            autoVoicings[i] = v;
            prev = v;
        }
    }
}

int Engine::slotAt (double beat) const
{
    if (cycle <= 0.0 || starts.empty())
        return 0;
    const double pos = beat - std::floor (beat / cycle) * cycle;
    for (size_t i = 0; i < starts.size(); ++i)
        if (pos >= starts[i] - 1e-9 && pos < starts[i] + durations[i] - 1e-9)
            return (int) i;
    return (int) starts.size() - 1;
}

void Engine::hitsInRange (double from, double to, std::vector<Hit>& out) const
{
    const int n = numSlots();
    if (n == 0 || to <= from || cycle <= 0.0)
        return;

    const auto& pat = patternLibrary()[(size_t) s.pattern];
    const bool sixteenths = usesSixteenthGrid (pat);

    for (double cycleStart = std::floor (from / cycle) * cycle; cycleStart < to; cycleStart += cycle)
    {
        for (int i = 0; i < n; ++i)
        {
            const double chordStart = cycleStart + starts[(size_t) i];
            const double d = durations[(size_t) i];
            if (chordStart >= to)
                return;
            if (chordStart + d <= from)
                continue;

            if (pat.sustain)
            {
                if (chordStart >= from)
                    out.push_back ({ chordStart, d, i, 0, 1.0, false });
                continue;
            }

            int index = 0;
            for (int bar = 0; bar * 4.0 < d - 1e-9; ++bar)
            {
                for (const auto& ph : pat.hits)
                {
                    const double rel = bar * 4.0 + ph.pos;
                    if (rel >= d - 1e-9)
                        continue;

                    const double swung = rel + swingOffset (ph.pos, s.swing, sixteenths);
                    const double t = chordStart + swung;
                    const double len = std::max (0.05, std::min (ph.len, d - swung));
                    const double accent = ph.accent * (bar == 0 ? 1.0 : 0.94);
                    if (t >= from && t < to)
                        out.push_back ({ t, len, i, index, accent, ph.up });
                    ++index;
                }
            }
        }
    }
}

void Engine::ongoingHits (double t, std::vector<Hit>& out) const
{
    std::vector<Hit> earlier;
    hitsInRange (t - maxDuration - 0.5, t, earlier);
    for (const auto& h : earlier)
    {
        const double end = h.start + h.length;
        if (h.start < t && end > t + 0.05)
        {
            Hit k = h;
            k.start = t;
            k.length = end - t;
            out.push_back (k);
        }
    }
}

void Engine::realizeHit (const Hit& hit, Humanizer& h, bool allowEarly, std::vector<NoteEvent>& out) const
{
    if (hit.slot < 0 || hit.slot >= numSlots())
        return;

    const auto& v = autoVoicings[(size_t) hit.slot];
    const auto& pat = patternLibrary()[(size_t) s.pattern];
    const double len = hit.length * s.gate;

    if (pat.arp != 0)
    {
        const auto pool = v.allNotes();
        const int m = (int) pool.size();
        if (m == 0)
            return;
        int k = hit.index % m;
        if (pat.arp == 2 && m >= 2)
        {
            const int cycleLen = 2 * m - 2;
            const int q = hit.index % cycleLen;
            k = q < m ? q : cycleLen - q;
        }
        h.single (pool[(size_t) k], hit.accent, s.baseVelocity, len, allowEarly, s.human, out);
        return;
    }

    auto notes = v.allNotes();
    bool hasBass = v.bass >= 0;
    if (pat.guitarStrum && hit.up && hasBass && notes.size() > 3)
    {
        notes.erase (notes.begin()); // up-strokes catch the top strings
        hasBass = false;
    }
    h.chord (notes, hasBass, hit.accent, s.baseVelocity, hit.up, len, allowEarly, s.human, out);
}

Voicing Engine::liveVoicing (const Chord& chord)
{
    const Voicing v = voiceChord (chord, s.keyRoot, s.voicing, lastLive.empty() ? nullptr : &lastLive);
    lastLive = v;
    return v;
}

} // namespace cw
