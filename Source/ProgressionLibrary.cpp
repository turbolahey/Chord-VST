#include "ProgressionLibrary.h"

#include <stdexcept>

namespace cw
{

std::string Progression::displayName() const
{
    std::string n = numerals;
    for (auto& ch : n)
        if (ch == ' ') ch = '-';
    return name + "   " + n;
}

const std::vector<std::string>& genreNames()
{
    static const std::vector<std::string> g { "Pop and rock", "R&B, neo soul and gospel", "Hip hop and trap", "EDM and house" };
    return g;
}

namespace
{
    std::vector<Progression> buildLibrary()
    {
        const auto& g = genreNames();
        const std::string pop = g[0], rnb = g[1], hip = g[2], edm = g[3];
        const std::vector<double> halves8 { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };

        auto P = [] (std::string name, std::string genre, bool minor, std::string numerals,
                     std::vector<double> weights, std::string heardIn) {
            Progression p;
            p.name = std::move (name);
            p.genre = std::move (genre);
            p.minor = minor;
            p.numerals = std::move (numerals);
            p.weights = std::move (weights);
            p.heardIn = std::move (heardIn);
            return p;
        };

        std::vector<Progression> lib {
            // ------------------------------------------------------------ pop and rock
            P ("Axis", pop, false, "I V vi IV", {}, "Let It Be, Don't Stop Believin', With or Without You, Someone Like You" ),
            P ("Axis from the 6", pop, false, "vi IV I V", {}, "Zombie (The Cranberries)" ),
            P ("Doo-wop", pop, false, "I vi IV V", {}, "Stand By Me, Every Breath You Take" ),
            P ("Three-chord rock", pop, false, "I IV V V", {}, "La Bamba, Twist and Shout" ),
            P ("Canon", pop, false, "I V vi iii IV I IV V", halves8, "Canon in D (Pachelbel), Graduation (Vitamin C)" ),
            P ("Mixolydian rock", pop, false, "I bVII IV I", {}, "Sweet Child O' Mine (verse), Hey Jude (outro)" ),
            P ("Creep", pop, false, "I III IV iv", {}, "Creep (Radiohead)" ),
            P ("Borrowed iv", pop, false, "I IV iv I", {}, "" ),
            P ("Pop rotation", pop, false, "IV I V vi", {}, "" ),
            P ("Andalusian", pop, true, "i bVII bVI V", {}, "Hit the Road Jack, Runaway (Del Shannon)" ),
            P ("Watchtower", pop, true, "i bVII bVI bVII", {}, "All Along the Watchtower (Jimi Hendrix)" ),

            // ------------------------------------------------------------ R&B, neo soul and gospel
            P ("Just the two of us", rnb, false, "IVmaj7 III7 vi7 v7 I7", { 1, 1, 1, 0.5, 0.5 },
               "Just the Two of Us (Grover Washington Jr. and Bill Withers)"),
            P ("Two-five-one", rnb, false, "ii9 V13 Imaj9 Imaj9", {}, "" ),
            P ("Turnaround", rnb, false, "Imaj7 vi9 ii9 V13", {}, "" ),
            P ("Neo soul descent", rnb, false, "IVmaj9 iii7 ii9 Imaj9", {}, "" ),
            P ("Neo soul vamp", rnb, false, "Imaj9 IVmaj9", {}, "" ),
            P ("Circle", rnb, false, "iii7 vi9 ii9 V13", {}, "" ),
            P ("Gospel walk-up", rnb, false, "I I7 IV #iv°7 I/V vi7 ii7 V7", halves8, "" ),
            P ("Gospel turnaround", rnb, false, "iii7 VI7 ii9 V13", {}, "" ),
            P ("Plagal soul", rnb, false, "IVmaj7 iv7 Imaj7 Imaj7", {}, "" ),
            P ("Backdoor", rnb, false, "IVmaj7 bVII9 Imaj7 Imaj7", {}, "" ),
            P ("Tritone sub", rnb, false, "ii9 bII7 Imaj9 Imaj9", {}, "" ),
            P ("Minor R&B vamp", rnb, true, "i9 iv9", {}, "" ),
            P ("Dorian groove", rnb, true, "i7 IV9", {}, "Oye Como Va (Santana)" ),
            P ("Minor soul circle", rnb, true, "i9 iv9 bVII9 bIIImaj7", {}, "" ),

            // ------------------------------------------------------------ hip hop and trap
            P ("Dark minor", hip, true, "i bVI bVII i", {}, "" ),
            P ("Harmonic trap", hip, true, "i iv bVI V", {}, "" ),
            P ("Phrygian", hip, true, "i bII i bII", {}, "" ),
            P ("Minor drop", hip, true, "i bVI iv V", {}, "" ),
            P ("Boom bap jazz", hip, true, "i7 bVImaj7 iiø7 V7", {}, "" ),
            P ("Lo-fi", hip, false, "Imaj7 iii7 IVmaj7 iv7", {}, "" ),
            P ("Melodic trap", hip, true, "i bIII bVII bVI", {}, "" ),

            // ------------------------------------------------------------ EDM and house
            P ("Festival", edm, false, "vi IV I V", {}, "Wake Me Up (Avicii)" ),
            P ("Anthem minor", edm, true, "i bVI bIII bVII", {}, "" ),
            P ("Royal road", edm, false, "IVmaj7 V7 iii7 vi", {}, "" ),
            P ("Deep house stabs", edm, true, "i9 bVII9 bVImaj9 iv9", {}, "" ),
            P ("Build-up", edm, false, "IV V vi vi", {}, "" ),
            P ("Melodic techno", edm, true, "i bVI bVII v", {}, "" ),
            P ("Piano house", edm, false, "vi7 IVmaj7 I V", {}, "" ),
        };

        for (auto& p : lib)
            if (! parseProgression (p.numerals, p.chords))
                throw std::runtime_error ("Bad progression: " + p.name);

        return lib;
    }

    std::vector<Chord> parseAll (const std::vector<std::string>& numerals)
    {
        std::vector<Chord> out;
        for (const auto& n : numerals)
        {
            Chord c;
            if (! parseNumeral (n, c))
                throw std::runtime_error ("Bad numeral: " + n);
            out.push_back (c);
        }
        return out;
    }
}

const std::vector<Progression>& progressionLibrary()
{
    static const std::vector<Progression> lib = buildLibrary();
    return lib;
}

const std::vector<std::string>& diatonicNumerals (bool minorKey)
{
    // White keys C D E F G A B, then black keys C# D# F# G# A#.
    static const std::vector<std::string> major { "I", "ii", "iii", "IV", "V", "vi", "vii°",
                                                  "II7", "bIII", "iv", "bVI", "bVII" };
    static const std::vector<std::string> minor { "i", "ii°", "bIII", "iv", "v", "bVI", "bVII",
                                                  "bII", "ii", "IV", "V", "vii°7" };
    return minorKey ? minor : major;
}

const std::vector<Chord>& diatonicChords (bool minorKey)
{
    static const std::vector<Chord> major = parseAll (diatonicNumerals (false));
    static const std::vector<Chord> minor = parseAll (diatonicNumerals (true));
    return minorKey ? minor : major;
}

} // namespace cw
