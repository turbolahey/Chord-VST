#include "Theory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>

namespace cw
{

static const int kMajorScale[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int kMinorScale[7] = { 0, 2, 3, 5, 7, 8, 10 };

static bool matchAt (const std::string& s, size_t pos, const char* token)
{
    const size_t len = std::strlen (token);
    return pos + len <= s.size() && s.compare (pos, len, token) == 0;
}

static bool parseRoman (const std::string& s, size_t& pos, int& degree, bool& lower)
{
    struct Entry { const char* text; int degree; };
    static const Entry table[] = { { "VII", 7 }, { "III", 3 }, { "VI", 6 }, { "IV", 4 },
                                   { "II", 2 },  { "V", 5 },   { "I", 1 } };

    for (const auto& e : table)
    {
        const size_t len = std::strlen (e.text);
        if (pos + len > s.size())
            continue;

        bool allUpper = true, allLower = true, same = true;
        for (size_t i = 0; i < len; ++i)
        {
            const char ch = s[pos + i];
            if (std::toupper ((unsigned char) ch) != e.text[i]) { same = false; break; }
            if (std::islower ((unsigned char) ch)) allUpper = false; else allLower = false;
        }

        if (same && (allUpper || allLower))
        {
            degree = e.degree;
            lower = allLower;
            pos += len;
            return true;
        }
    }
    return false;
}

static bool parseAccidental (const std::string& s, size_t& pos, int& acc)
{
    acc = 0;
    if (pos < s.size() && s[pos] == 'b') { acc = -1; ++pos; }
    else if (pos < s.size() && s[pos] == '#') { acc = 1; ++pos; }
    return true;
}

bool parseNumeral (const std::string& text, Chord& out)
{
    Chord c;
    c.text = text;

    std::string main = text, slash;
    if (const auto slashPos = text.find ('/'); slashPos != std::string::npos)
    {
        main = text.substr (0, slashPos);
        slash = text.substr (slashPos + 1);
    }

    size_t pos = 0;
    parseAccidental (main, pos, c.accidental);
    bool lower = false;
    if (! parseRoman (main, pos, c.degree, lower))
        return false;

    c.rootOffset = ((kMajorScale[c.degree - 1] + c.accidental) % 12 + 12) % 12;
    c.triad = lower ? Triad::Minor : Triad::Major;

    auto ensureSeventh = [&c] { if (c.seventh == 0) c.seventh = 10; };

    while (pos < main.size())
    {
        if (matchAt (main, pos, "maj13"))      { c.seventh = 11; c.ninth = c.thirteenth = true; pos += 5; }
        else if (matchAt (main, pos, "maj9"))  { c.seventh = 11; c.ninth = true; pos += 4; }
        else if (matchAt (main, pos, "maj7"))  { c.seventh = 11; pos += 4; }
        else if (matchAt (main, pos, "M7"))    { c.seventh = 11; pos += 2; }
        else if (matchAt (main, pos, "add9"))  { c.ninth = true; pos += 4; }
        else if (matchAt (main, pos, "sus2"))  { c.triad = Triad::Sus2; pos += 4; }
        else if (matchAt (main, pos, "sus4"))  { c.triad = Triad::Sus4; pos += 4; }
        else if (matchAt (main, pos, "sus"))   { c.triad = Triad::Sus4; pos += 3; }
        else if (matchAt (main, pos, "dim") || matchAt (main, pos, "\xC2\xB0") || matchAt (main, pos, "o"))
        {
            pos += matchAt (main, pos, "dim") ? 3u : (main[pos] == 'o' ? 1u : 2u);
            c.triad = Triad::Dim;
            if (matchAt (main, pos, "7")) { c.seventh = 9; ++pos; }
        }
        else if (matchAt (main, pos, "\xC3\xB8") || matchAt (main, pos, "h"))
        {
            pos += main[pos] == 'h' ? 1u : 2u;
            c.triad = Triad::Dim;
            c.seventh = 10;
            if (matchAt (main, pos, "7")) ++pos;
        }
        else if (matchAt (main, pos, "aug"))   { c.triad = Triad::Aug; pos += 3; }
        else if (matchAt (main, pos, "+"))     { c.triad = Triad::Aug; pos += 1; }
        else if (matchAt (main, pos, "69"))    { c.sixth = c.ninth = true; pos += 2; }
        else if (matchAt (main, pos, "b9"))    { ensureSeventh(); c.ninth = true; c.ninthAlt = -1; pos += 2; }
        else if (matchAt (main, pos, "#9"))    { ensureSeventh(); c.ninth = true; c.ninthAlt = 1; pos += 2; }
        else if (matchAt (main, pos, "#11"))   { c.eleventh = true; c.eleventhAlt = 1; pos += 3; }
        else if (matchAt (main, pos, "b13"))   { c.thirteenth = true; c.thirteenthAlt = -1; pos += 3; }
        else if (matchAt (main, pos, "13"))    { ensureSeventh(); c.ninth = c.thirteenth = true; pos += 2; }
        else if (matchAt (main, pos, "11"))    { ensureSeventh(); c.ninth = c.eleventh = true; pos += 2; }
        else if (matchAt (main, pos, "9"))     { ensureSeventh(); c.ninth = true; pos += 1; }
        else if (matchAt (main, pos, "7"))     { ensureSeventh(); pos += 1; }
        else if (matchAt (main, pos, "6"))     { c.sixth = true; pos += 1; }
        else return false;
    }

    if (! slash.empty())
    {
        size_t bp = 0;
        int acc = 0, deg = 1;
        bool dummy = false;
        parseAccidental (slash, bp, acc);
        if (! parseRoman (slash, bp, deg, dummy) || bp != slash.size())
            return false;
        c.hasSlash = true;
        c.bassOffset = ((kMajorScale[deg - 1] + acc) % 12 + 12) % 12;
    }

    out = c;
    return true;
}

bool parseProgression (const std::string& text, std::vector<Chord>& out)
{
    out.clear();
    std::istringstream ss (text);
    std::string token;
    while (ss >> token)
    {
        Chord c;
        if (! parseNumeral (token, c))
            return false;
        out.push_back (c);
    }
    return ! out.empty();
}

bool scaleContains (int semis, bool minorKey)
{
    semis = ((semis % 12) + 12) % 12;
    const int* scale = minorKey ? kMinorScale : kMajorScale;
    return std::find (scale, scale + 7, semis) != scale + 7;
}

static int defaultSeventh (const Chord& c, bool minorKey)
{
    switch (c.triad)
    {
        case Triad::Major:
        case Triad::Aug:
            // Major 7th when it belongs to the key, otherwise it behaves as a dominant.
            return scaleContains (c.rootOffset + 11, minorKey) ? 11 : 10;
        case Triad::Minor:
        case Triad::Dim:
        case Triad::Sus2:
        case Triad::Sus4:
        default:
            return 10;
    }
}

Chord applyColor (const Chord& in, ColorMode mode, bool minorKey)
{
    Chord c = in;
    if (mode == ColorMode::AsWritten)
        return c;

    if (mode == ColorMode::Triads)
    {
        c.seventh = 0;
        c.sixth = c.ninth = c.eleventh = c.thirteenth = false;
        c.ninthAlt = c.eleventhAlt = c.thirteenthAlt = 0;
        return c;
    }

    if (c.seventh == 0 && ! c.sixth)
        c.seventh = defaultSeventh (c, minorKey);

    if (mode == ColorMode::Sevenths)
        return c;

    const bool dominant = c.triad == Triad::Major && c.seventh == 10;

    if (! c.ninth && c.seventh != 9)
    {
        c.ninth = true;
        if (minorKey && dominant && c.rootOffset == 7)
            c.ninthAlt = -1; // V7b9 in minor keys
    }

    if (mode == ColorMode::Ninths)
        return c;

    if (c.triad == Triad::Minor)
    {
        c.eleventh = true;
    }
    else if (c.triad == Triad::Major && ! c.thirteenth && ! c.sixth)
    {
        c.thirteenth = true;
        if (dominant && c.ninthAlt == -1)
            c.thirteenthAlt = -1;
    }
    else if (c.triad == Triad::Sus4 && c.seventh == 10)
    {
        c.thirteenth = true;
    }
    return c;
}

namespace
{
    enum Role { RoleRoot, RoleThird, RoleFifth, RoleSeventh, RoleExt };
    struct Tone { int rel; Role role; };

    void triadIntervals (Triad t, int& third, int& fifth)
    {
        switch (t)
        {
            case Triad::Major: third = 4; fifth = 7; break;
            case Triad::Minor: third = 3; fifth = 7; break;
            case Triad::Dim:   third = 3; fifth = 6; break;
            case Triad::Aug:   third = 4; fifth = 8; break;
            case Triad::Sus2:  third = 2; fifth = 7; break;
            case Triad::Sus4:  third = 5; fifth = 7; break;
        }
    }

    std::vector<Tone> chordTones (const Chord& c)
    {
        int third = 4, fifth = 7;
        triadIntervals (c.triad, third, fifth);
        std::vector<Tone> t { { 0, RoleRoot }, { third, RoleThird }, { fifth, RoleFifth } };
        if (c.sixth)       t.push_back ({ 9, RoleSeventh });
        if (c.seventh)     t.push_back ({ c.seventh, RoleSeventh });
        if (c.ninth)       t.push_back ({ 14 + c.ninthAlt, RoleExt });
        if (c.eleventh)    t.push_back ({ 17 + c.eleventhAlt, RoleExt });
        if (c.thirteenth)  t.push_back ({ 21 + c.thirteenthAlt, RoleExt });
        return t;
    }
}

std::vector<int> chordIntervals (const Chord& c)
{
    std::vector<int> result;
    for (const auto& t : chordTones (c))
        result.push_back (t.rel);
    std::sort (result.begin(), result.end());
    result.erase (std::unique (result.begin(), result.end()), result.end());
    return result;
}

std::string noteName (int pc, bool flats)
{
    static const char* sharps[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* flatNames[12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    pc = ((pc % 12) + 12) % 12;
    return flats ? flatNames[pc] : sharps[pc];
}

bool keyPrefersFlats (int keyRoot, bool minorKey)
{
    keyRoot = ((keyRoot % 12) + 12) % 12;
    static const bool majorFlats[12] = { false, true, false, true, false, true, true, false, true, false, true, false };
    static const bool minorFlats[12] = { true, false, true, true, false, true, false, true, false, false, true, false };
    return minorKey ? minorFlats[keyRoot] : majorFlats[keyRoot];
}

std::string chordName (const Chord& c, int keyRoot, bool minorKey)
{
    const bool flats = c.accidental < 0 ? true : (c.accidental > 0 ? false : keyPrefersFlats (keyRoot, minorKey));
    const int rootPc = (keyRoot + c.rootOffset) % 12;
    std::string name = noteName (rootPc, flats);

    int ext = 0;
    if (c.seventh) ext = 7;
    if (c.seventh && c.ninth && c.ninthAlt == 0) ext = 9;
    if (c.seventh && c.eleventh && c.eleventhAlt == 0) ext = 11;
    if (c.seventh && c.thirteenth && c.thirteenthAlt == 0) ext = 13;
    const std::string e = std::to_string (ext);
    const bool sus = c.triad == Triad::Sus2 || c.triad == Triad::Sus4;
    const std::string susText = c.triad == Triad::Sus2 ? "sus2" : "sus4";

    std::string q;
    if (c.seventh)
    {
        switch (c.triad)
        {
            case Triad::Major: q = (c.seventh == 11 ? "maj" : "") + e; break;
            case Triad::Aug:   q = c.seventh == 11 ? "maj" + e + "#5" : "aug" + e; break;
            case Triad::Minor: q = c.seventh == 11 ? "m(maj" + e + ")" : "m" + e; break;
            case Triad::Dim:   q = c.seventh == 9 ? "dim7" : "m" + e + "b5"; break;
            case Triad::Sus2:
            case Triad::Sus4:  q = (c.seventh == 11 ? "maj" : "") + e + susText; break;
        }
    }
    else
    {
        std::string base;
        switch (c.triad)
        {
            case Triad::Minor: base = "m"; break;
            case Triad::Dim:   base = "dim"; break;
            case Triad::Aug:   base = "aug"; break;
            case Triad::Major:
            case Triad::Sus2:
            case Triad::Sus4:  break;
        }
        if (c.sixth)                          q = base + (c.ninth ? "6/9" : "6");
        else if (c.ninth && c.ninthAlt == 0)  q = base + "add9";
        else                                  q = base;
        if (sus) q += susText;
    }

    if (c.ninthAlt == -1) q += "b9";
    if (c.ninthAlt == 1)  q += "#9";
    if (c.eleventhAlt == 1) q += "#11";
    if (c.thirteenthAlt == -1) q += "b13";

    name += q;

    if (c.hasSlash)
    {
        const int bassPc = (keyRoot + c.bassOffset) % 12;
        if (bassPc != rootPc)
            name += "/" + noteName (bassPc, flats);
    }
    return name;
}

std::string numeralName (const Chord& c)
{
    static const char* romans[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    const bool lower = c.triad == Triad::Minor || c.triad == Triad::Dim;

    std::string roman = romans[c.degree - 1];
    if (lower)
        for (auto& ch : roman) ch = (char) std::tolower ((unsigned char) ch);
    std::string out = (c.accidental < 0 ? "b" : (c.accidental > 0 ? "#" : "")) + roman;

    // Reuse the chord-symbol suffix (C as a neutral root), then adapt it to numeral style.
    Chord plain = c;
    plain.hasSlash = false;
    plain.rootOffset = 0;
    plain.accidental = 0;
    std::string suffix = chordName (plain, 0, false).substr (1);

    auto startsWith = [&suffix] (const char* t) { return suffix.rfind (t, 0) == 0; };
    if (c.triad == Triad::Dim)
    {
        if (startsWith ("dim7"))      suffix = "\xC2\xB0" "7" + suffix.substr (4);
        else if (startsWith ("dim"))  suffix = "\xC2\xB0" + suffix.substr (3);
        else if (startsWith ("m"))
        {
            // m7b5 -> ø7, m9b5 -> ø9
            const auto b5 = suffix.find ("b5");
            suffix = "\xC3\xB8" + suffix.substr (1, b5 == std::string::npos ? std::string::npos : b5 - 1)
                   + (b5 == std::string::npos ? "" : suffix.substr (b5 + 2));
        }
    }
    else if (c.triad == Triad::Minor && startsWith ("m") && ! startsWith ("maj"))
    {
        suffix = suffix.substr (1);
    }
    out += suffix;

    if (c.hasSlash)
    {
        const auto slash = c.text.find ('/');
        if (slash != std::string::npos)
            out += c.text.substr (slash);
    }
    return out;
}

// ---------------------------------------------------------------- voicing

std::vector<int> Voicing::allNotes() const
{
    std::vector<int> n;
    if (bass >= 0)
        n.push_back (bass);
    n.insert (n.end(), upper.begin(), upper.end());
    return n;
}

namespace
{
    double centerCost (const std::vector<int>& v, int center)
    {
        double sum = 0;
        for (int n : v) sum += n;
        return std::abs (sum / (double) v.size() - center);
    }

    double movementCost (const std::vector<int>& a, const std::vector<int>& b)
    {
        double cost = 0;
        for (int x : a)
        {
            int best = 1000;
            for (int y : b) best = std::min (best, std::abs (x - y));
            cost += best;
        }
        for (int y : b)
        {
            int best = 1000;
            for (int x : a) best = std::min (best, std::abs (x - y));
            cost += best;
        }
        return cost * 0.5;
    }

    // Arrangers avoid tight intervals in low registers because they turn to mud.
    double lowIntervalPenalty (const std::vector<int>& v)
    {
        double p = 0;
        for (size_t i = 1; i < v.size(); ++i)
        {
            const int lo = v[i - 1], gap = v[i] - lo;
            if (lo < 48 && gap < 3) p += 6;
            if (lo < 43 && gap < 5) p += 4;
            if (lo < 40 && gap < 7) p += 4;
        }
        return p;
    }

    void applyStyle (std::vector<int>& v, VoicingStyle style)
    {
        std::sort (v.begin(), v.end());
        const int n = (int) v.size();
        if (style == VoicingStyle::Open || style == VoicingStyle::Wide)
        {
            if (n == 3)
                v[1] += 12;                      // open triad: root, fifth, third
            else if (n >= 4)
            {
                v[(size_t) (n - 2)] -= 12;       // drop 2
                if (style == VoicingStyle::Wide)
                    v[(size_t) (n - 4)] -= 12;   // drop 2 and 4
            }
        }
        std::sort (v.begin(), v.end());
    }
}

Voicing voiceChord (const Chord& chord, int keyRoot, const VoicingSettings& vs, const Voicing* previous)
{
    keyRoot = ((keyRoot % 12) + 12) % 12;
    const int rootPc = (keyRoot + chord.rootOffset) % 12;

    // 1. Unique chord tones by pitch class, in order of importance.
    std::vector<Tone> tones;
    for (const auto& t : chordTones (chord))
    {
        const bool dup = std::any_of (tones.begin(), tones.end(), [&] (const Tone& o) { return o.rel % 12 == t.rel % 12; });
        if (! dup)
            tones.push_back (t);
    }

    auto removeFirst = [&tones] (auto pred) {
        auto it = std::find_if (tones.begin(), tones.end(), pred);
        if (it == tones.end()) return false;
        tones.erase (it);
        return true;
    };
    auto isRoot = [] (const Tone& t) { return t.role == RoleRoot; };
    auto isPerfectFifth = [] (const Tone& t) { return t.role == RoleFifth && t.rel == 7; };

    // Rootless voicings (3rd, 7th, 9th, 13th on top of a bass note) for keys players.
    if (vs.style == VoicingStyle::Rootless && tones.size() >= (vs.bassNote ? 4u : 5u))
    {
        removeFirst (isRoot);
        if (tones.size() >= 5)
            removeFirst (isPerfectFifth);
    }

    while ((int) tones.size() > std::max (3, vs.maxVoices))
    {
        if (removeFirst (isPerfectFifth)) continue;
        if (vs.bassNote && removeFirst (isRoot)) continue;
        if (chord.triad != Triad::Minor && removeFirst ([] (const Tone& t) { return t.rel == 17; })) continue;
        if (removeFirst (isRoot)) continue;
        tones.pop_back();
    }

    if (vs.bassNote && tones.size() >= 5)
        removeFirst (isRoot);

    std::vector<int> pcs;
    for (const auto& t : tones)
        pcs.push_back ((rootPc + t.rel) % 12);

    // Root position order (starting from the lowest chord tone above the root).
    std::vector<int> relOrder;
    for (const auto& t : tones)
        relOrder.push_back (t.rel % 12);
    std::vector<size_t> idx (pcs.size());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::sort (idx.begin(), idx.end(), [&] (size_t a, size_t b) { return relOrder[a] < relOrder[b]; });

    std::vector<int> ordered;
    for (auto i : idx) ordered.push_back (pcs[i]);

    const size_t n = ordered.size();
    const bool useLeading = vs.voiceLeading && previous != nullptr && ! previous->upper.empty();

    std::vector<int> best;
    double bestCost = std::numeric_limits<double>::max();

    const size_t rotations = vs.voiceLeading ? n : 1;
    for (size_t r = 0; r < rotations; ++r)
    {
        // Stack the tones upward in close position starting from rotation r.
        std::vector<int> stack;
        int prev = ordered[r];
        stack.push_back (prev);
        for (size_t k = 1; k < n; ++k)
        {
            const int pc = ordered[(r + k) % n];
            int note = prev + 1;
            while (note % 12 != pc) ++note;
            stack.push_back (note);
            prev = note;
        }

        for (int low = vs.center - 18; low <= vs.center + 8; ++low)
        {
            if (((low % 12) + 12) % 12 != ordered[r])
                continue;

            std::vector<int> cand;
            for (int s : stack) cand.push_back (s - stack[0] + low);
            applyStyle (cand, vs.style);

            if (cand.front() < 33 || cand.back() > 98)
                continue;

            double cost = 0.0;
            if (useLeading)
            {
                cost = movementCost (cand, previous->upper)
                     + 0.6 * std::abs (cand.back() - previous->upper.back())
                     + 0.35 * centerCost (cand, vs.center);
            }
            else
            {
                cost = centerCost (cand, vs.center);
            }
            cost += lowIntervalPenalty (cand);
            if (cand.front() < 50)
                cost += (50 - cand.front()) * 1.5; // keep room under the chord for the bass

            if (cost < bestCost - 1e-9)
            {
                bestCost = cost;
                best = cand;
            }
        }
    }

    Voicing v;
    v.upper = best;

    if (vs.bassNote && ! v.upper.empty())
    {
        const int bassPc = chord.hasSlash ? (keyRoot + chord.bassOffset) % 12 : rootPc;
        int chosen = -1;
        double chosenCost = std::numeric_limits<double>::max();
        for (int b = vs.bassCenter - 7; b <= vs.bassCenter + 7; ++b)
        {
            if (b % 12 != bassPc) continue;
            double cost = std::abs (b - vs.bassCenter) * 0.5;
            if (vs.voiceLeading && previous != nullptr && previous->bass >= 0)
                cost += std::abs (b - previous->bass);
            if (cost < chosenCost) { chosenCost = cost; chosen = b; }
        }
        const int gap = v.upper.front() < 52 ? 7 : 3;
        while (chosen > v.upper.front() - gap && chosen - 12 >= 24)
            chosen -= 12;
        v.bass = chosen;
    }

    return v;
}

} // namespace cw
