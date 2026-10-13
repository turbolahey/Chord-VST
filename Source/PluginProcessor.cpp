#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

using namespace juce;

// Studio One, Arturia and Native Instruments all call MIDI note 60 "C3".
static String noteLabel (int midiNote)
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return String (names[midiNote % 12]) + String (midiNote / 12 - 2);
}

static StringArray keyChoices()
{
    return { "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };
}

AudioProcessorValueTreeState::ParameterLayout ChordwrightProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto id = [] (const char* name) { return ParameterID { name, 1 }; };

    auto percent = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + "%"; });
    auto millis = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; });
    auto signedMillis = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
        const int ms = roundToInt (v);
        if (ms == 0) return String ("on grid");
        return ms > 0 ? String (ms) + " ms late" : String (-ms) + " ms early";
    });

    StringArray progressions;
    for (const auto& pr : cw::progressionLibrary())
        progressions.add (String::fromUTF8 (pr.displayName().c_str()));

    StringArray patterns;
    for (const auto& pt : cw::patternLibrary())
        patterns.add (pt.name);

    p.push_back (std::make_unique<AudioParameterChoice> (id ("key"), "Key", keyChoices(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (id ("progression"), "Progression", progressions, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (id ("mode"), "Play mode",
                                                         StringArray { "Chord keys", "Auto play", "Humanize my playing" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (id ("preview"), "Preview", false));

    p.push_back (std::make_unique<AudioParameterChoice> (id ("pattern"), "Pattern", patterns, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (id ("chordLength"), "Chord length",
                                                         StringArray { "1 beat", "2 beats", "1 bar", "2 bars" }, 2));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("swing"), "Swing", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f, percent));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("gate"), "Gate", NormalisableRange<float> (10.0f, 100.0f, 1.0f), 96.0f, percent));

    p.push_back (std::make_unique<AudioParameterChoice> (id ("color"), "Color",
                                                         StringArray { "As written", "Triads", "7ths", "9ths", "Lush" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (id ("voicing"), "Voicing",
                                                         StringArray { "Close", "Open", "Wide", "Rootless" }, 0));
    p.push_back (std::make_unique<AudioParameterInt> (id ("register"), "Register", 48, 72, 60,
                                                      AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return noteLabel (v); })));
    p.push_back (std::make_unique<AudioParameterBool> (id ("voiceLeading"), "Voice leading", true));
    p.push_back (std::make_unique<AudioParameterBool> (id ("bass"), "Bass note", true));

    p.push_back (std::make_unique<AudioParameterFloat> (id ("timing"), "Timing", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 35.0f, percent));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("laidBack"), "Laid back", NormalisableRange<float> (-30.0f, 30.0f, 1.0f), 0.0f, signedMillis));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("strum"), "Strum", NormalisableRange<float> (0.0f, 150.0f, 1.0f, 0.6f), 18.0f, millis));
    p.push_back (std::make_unique<AudioParameterChoice> (id ("strumDir"), "Strum direction",
                                                         StringArray { "Down", "Up", "Alternate", "Random" }, 2));
    p.push_back (std::make_unique<AudioParameterInt> (id ("velocity"), "Velocity", 1, 127, 92));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("velVar"), "Velocity variation", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f, percent));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("topVoice"), "Top voice", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 40.0f, percent));
    p.push_back (std::make_unique<AudioParameterFloat> (id ("lengthVar"), "Length variation", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 25.0f, percent));

    p.push_back (std::make_unique<AudioParameterInt> (id ("zone"), "Keyboard zone", -2, 2, 0,
                                                      AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return noteLabel (48 + 12 * v); })));

    return { p.begin(), p.end() };
}

ChordwrightProcessor::ChordwrightProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Chordwright", createLayout())
{
    pKey          = apvts.getRawParameterValue ("key");
    pProgression  = apvts.getRawParameterValue ("progression");
    pMode         = apvts.getRawParameterValue ("mode");
    pPreview      = apvts.getRawParameterValue ("preview");
    pPattern      = apvts.getRawParameterValue ("pattern");
    pChordLength  = apvts.getRawParameterValue ("chordLength");
    pSwing        = apvts.getRawParameterValue ("swing");
    pGate         = apvts.getRawParameterValue ("gate");
    pColor        = apvts.getRawParameterValue ("color");
    pVoicing      = apvts.getRawParameterValue ("voicing");
    pRegister     = apvts.getRawParameterValue ("register");
    pVoiceLeading = apvts.getRawParameterValue ("voiceLeading");
    pBass         = apvts.getRawParameterValue ("bass");
    pTiming       = apvts.getRawParameterValue ("timing");
    pLaidBack     = apvts.getRawParameterValue ("laidBack");
    pStrum        = apvts.getRawParameterValue ("strum");
    pStrumDir     = apvts.getRawParameterValue ("strumDir");
    pVelocity     = apvts.getRawParameterValue ("velocity");
    pVelVar       = apvts.getRawParameterValue ("velVar");
    pTopVoice     = apvts.getRawParameterValue ("topVoice");
    pLengthVar    = apvts.getRawParameterValue ("lengthVar");
    pZone         = apvts.getRawParameterValue ("zone");

    pending.reserve (16384);
    gathered.reserve (64);
    noteScratch.reserve (64);
    hitScratch.reserve (2048);
    eventScratch.reserve (512);
    lastChannel.fill (1);
    humanChannel.fill (1);
}

bool ChordwrightProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == AudioChannelSet::stereo() || out == AudioChannelSet::mono()) && layouts.getMainInputChannelSet().isDisabled();
}

void ChordwrightProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate > 0 ? sampleRate : 44100.0;
    outBuffer.ensureSize ((size_t) jmax (4096, samplesPerBlock * 4));

    pending.clear();
    for (auto& o : owned) o.fill (0);
    totalOn.fill (0);
    for (auto& t : triggers) t.active = false;
    humanActive.fill (false);
    gathered.clear();
    gatherActive = false;
    autoRunning = false;
    engine.resetLiveVoicing();
    humanizer.reset();
}

void ChordwrightProcessor::releaseResources() {}

cw::EngineSettings ChordwrightProcessor::readSettings() const
{
    static const double chordLengths[] = { 1.0, 2.0, 4.0, 8.0 };

    cw::EngineSettings s;
    s.keyRoot = (int) pKey->load();
    s.progression = (int) pProgression->load();
    s.color = (cw::ColorMode) (int) pColor->load();
    s.voicing.style = (cw::VoicingStyle) (int) pVoicing->load();
    s.voicing.center = (int) pRegister->load();
    s.voicing.voiceLeading = pVoiceLeading->load() > 0.5f;
    s.voicing.bassNote = pBass->load() > 0.5f;
    s.voicing.bassCenter = jlimit (31, 47, s.voicing.center - 20);
    s.pattern = (int) pPattern->load();
    s.beatsPerChord = chordLengths[jlimit (0, 3, (int) pChordLength->load())];
    s.swing = pSwing->load() / 100.0;
    s.gate = pGate->load() / 100.0;
    s.baseVelocity = (int) pVelocity->load();
    s.human.timingMs = pTiming->load() / 100.0 * 30.0;
    s.human.laidBackMs = pLaidBack->load();
    s.human.strumMs = pStrum->load();
    s.human.strumDir = (cw::StrumDirection) (int) pStrumDir->load();
    s.human.velocityVariation = pVelVar->load() / 100.0;
    s.human.topVoice = pTopVoice->load() / 100.0;
    s.human.lengthVariation = pLengthVar->load() / 100.0;
    return s;
}

ChordwrightProcessor::PlayMode ChordwrightProcessor::playMode() const
{
    return (PlayMode) jlimit (0, 2, (int) pMode->load());
}

int ChordwrightProcessor::zoneOctave() const
{
    return jlimit (-2, 2, (int) pZone->load());
}

// ------------------------------------------------------------------ processing

void ChordwrightProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    const int64 blockStart = sampleCounter;
    blockStartSample = blockStart;
    blockNumSamples = numSamples;
    outBuffer.clear();

    const auto s = readSettings();
    engine.setSettings (s);
    const auto mode = playMode();

    if (mode != lastMode)
    {
        killAll (blockStart);
        engine.resetLiveVoicing();
        lastMode = mode;
        currentSlotAtomic.store (-1);
    }

    bool hostPlaying = false;
    double ppq = 0.0, bpm = lastBpmAtomic.load();
    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            if (const auto b = pos->getBpm())
                bpm = *b;
            if (const auto q = pos->getPpqPosition())
            {
                ppq = *q;
                hostPlaying = pos->getIsPlaying();
            }
        }
    }
    if (! (bpm > 20.0 && bpm < 999.0))
        bpm = 120.0;
    lastBpmAtomic.store (bpm);

    if (const int slot = auditionRequest.exchange (-1); slot >= 0 && slot < engine.numSlots())
    {
        triggerChord (engine.coloredSlotChord (slot), 128, 100, 1, blockStart, s, slot);
        releaseChord (128, blockStart + msToSamples (900.0), s);
    }

    for (const auto meta : midi)
        handleInput (meta.getMessage(), blockStart + meta.samplePosition, mode, s);

    if (mode == PlayMode::HumanizeInput)
        flushGather (blockStart + numSamples, s, false);

    if (mode == PlayMode::AutoPlay)
    {
        runAuto (blockStart, numSamples, hostPlaying, ppq, bpm, pPreview->load() > 0.5f, s);
    }
    else if (autoRunning)
    {
        stopOwner (OwnerAuto, blockStart);
        autoRunning = false;
    }
    autoRunningAtomic.store (autoRunning);

    emitPending (blockStart, numSamples);
    midi.swapWith (outBuffer);
    sampleCounter += numSamples;
}

void ChordwrightProcessor::passThrough (const MidiMessage& msg, int64 time)
{
    outBuffer.addEvent (msg, (int) jlimit<int64> (0, jmax (0, blockNumSamples - 1), time - blockStartSample));
}

void ChordwrightProcessor::handleInput (const MidiMessage& msg, int64 time, PlayMode mode, const cw::EngineSettings& s)
{
    if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        killAll (time);
        passThrough (msg, time);
        return;
    }

    if (! msg.isNoteOnOrOff())
    {
        passThrough (msg, time); // sustain pedal, mod wheel, pitch bend and friends
        return;
    }

    const int note = msg.getNoteNumber();
    const int channel = msg.getChannel();
    const bool isOn = msg.isNoteOn();
    const int velocity = msg.getVelocity();

    if (mode == PlayMode::ChordKeys)
    {
        const int rel = note - (48 + 12 * zoneOctave());

        if (rel >= 0 && rel < 12)
        {
            // White keys: the 7 chords of the key. Black keys: borrowed color chords.
            static const int keyIndex[12] = { 0, 7, 1, 8, 2, 3, 9, 4, 10, 5, 11, 6 };
            if (isOn) triggerChord (engine.coloredDiatonicChord (keyIndex[rel]), note, velocity, channel, time, s, -1);
            else      releaseChord (note, time, s);
            return;
        }

        if (rel >= 12 && rel <= 24)
        {
            // White keys from middle C up to the next C: progression slots 1 to 8.
            static const int slotIndex[13] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6, 7 };
            const int slot = slotIndex[rel - 12];
            if (slot < 0)
                return;
            if (isOn)
            {
                if (slot < engine.numSlots())
                    triggerChord (engine.coloredSlotChord (slot), note, velocity, channel, time, s, slot);
            }
            else
            {
                releaseChord (note, time, s);
            }
            return;
        }
    }
    else if (mode == PlayMode::HumanizeInput)
    {
        if (isOn) humanizeNoteOn (note, velocity, channel, time, s);
        else      humanizeNoteOff (note, time, s);
        return;
    }

    scheduleNote (time, note, isOn ? velocity : 0, channel, isOn, OwnerPass);
}

void ChordwrightProcessor::triggerChord (const cw::Chord& chord, int triggerIndex, int velocity, int channel,
                                        int64 time, const cw::EngineSettings& s, int slotForUi)
{
    if (triggers[(size_t) triggerIndex].active)
        releaseChord (triggerIndex, time, s);

    const auto voicing = engine.liveVoicing (chord);
    const auto notes = voicing.allNotes();

    eventScratch.clear();
    const bool up = humanizer.nextLiveUpStroke (s.human);
    humanizer.chord (notes, voicing.bass >= 0, 1.0, velocity, up, 1.0, false, s.human, eventScratch);

    auto& t = triggers[(size_t) triggerIndex];
    t.active = true;
    t.count = 0;
    t.channel = channel;
    for (const auto& e : eventScratch)
    {
        if (t.count >= (int) t.notes.size())
            break;
        const int64 on = time + msToSamples (e.onMs);
        scheduleNote (on, e.note, e.velocity, channel, true, OwnerLive);
        t.notes[(size_t) t.count] = e.note;
        t.onTimes[(size_t) t.count] = on;
        ++t.count;
    }

    if (slotForUi >= 0)
        currentSlotAtomic.store (slotForUi);
}

void ChordwrightProcessor::releaseChord (int triggerIndex, int64 time, const cw::EngineSettings& s)
{
    auto& t = triggers[(size_t) triggerIndex];
    if (! t.active)
        return;

    for (int i = 0; i < t.count; ++i)
    {
        // Fingers leave the keys at slightly different moments.
        const double jitter = s.human.timingMs * 0.5 * random.nextDouble();
        const int64 off = jmax (time + msToSamples (jitter), t.onTimes[(size_t) i] + msToSamples (5.0));
        scheduleNote (off, t.notes[(size_t) i], 0, t.channel, false, OwnerLive);
    }
    t.active = false;
}

void ChordwrightProcessor::humanizeNoteOn (int note, int velocity, int channel, int64 time, const cw::EngineSettings& s)
{
    if (humanActive[(size_t) note])
        humanizeNoteOff (note, time, s);

    humanChannel[(size_t) note] = channel;

    if (s.human.strumMs < 1.0)
    {
        eventScratch.clear();
        humanizer.single (note, 1.0, velocity, 1.0, false, s.human, eventScratch);
        const auto& e = eventScratch.front();
        const int64 on = time + msToSamples (e.onMs);
        scheduleNote (on, note, e.velocity, channel, true, OwnerLive);
        humanActive[(size_t) note] = true;
        humanOnTime[(size_t) note] = on;
        humanDelayMs[(size_t) note] = e.onMs;
        return;
    }

    // Collect notes played together for a few ms so they can be strummed in pitch order.
    if (! gatherActive)
    {
        gatherActive = true;
        gatherStart = time;
        gathered.clear();
    }
    if (gathered.size() < gathered.capacity())
        gathered.push_back ({ note, velocity, channel, time });

    humanActive[(size_t) note] = true;
    humanOnTime[(size_t) note] = -1;
    gatherReleasePending[(size_t) note] = false;
}

void ChordwrightProcessor::humanizeNoteOff (int note, int64 time, const cw::EngineSettings& s)
{
    if (! humanActive[(size_t) note])
        return;

    if (humanOnTime[(size_t) note] < 0)
    {
        gatherReleasePending[(size_t) note] = true; // released before the strum went out
        return;
    }

    const double jitter = s.human.timingMs * 0.4 * random.nextDouble();
    const int64 off = jmax (time + msToSamples (humanDelayMs[(size_t) note] + jitter),
                            humanOnTime[(size_t) note] + msToSamples (5.0));
    scheduleNote (off, note, 0, humanChannel[(size_t) note], false, OwnerLive);
    humanActive[(size_t) note] = false;
}

void ChordwrightProcessor::flushGather (int64 blockEnd, const cw::EngineSettings& s, bool force)
{
    if (! gatherActive)
        return;

    const int64 deadline = gatherStart + msToSamples (kGatherMs);
    if (! force && deadline >= blockEnd)
        return;

    gatherActive = false;
    if (gathered.empty())
        return;

    std::sort (gathered.begin(), gathered.end(), [] (const GatheredNote& a, const GatheredNote& b) { return a.note < b.note; });

    noteScratch.clear();
    int velocitySum = 0;
    for (const auto& g : gathered)
    {
        noteScratch.push_back (g.note);
        velocitySum += g.velocity;
    }
    const int avgVelocity = velocitySum / (int) gathered.size();

    eventScratch.clear();
    const bool up = humanizer.nextLiveUpStroke (s.human);
    humanizer.chord (noteScratch, false, 1.0, avgVelocity, up, 1.0, false, s.human, eventScratch);

    const int64 base = jmax (deadline, blockStartSample);
    for (size_t i = 0; i < gathered.size() && i < eventScratch.size(); ++i)
    {
        const auto& g = gathered[i];
        const auto& e = eventScratch[i];
        const int vel = jlimit (1, 127, e.velocity + (g.velocity - avgVelocity));
        const int64 on = base + msToSamples (e.onMs);
        scheduleNote (on, g.note, vel, g.channel, true, OwnerLive);

        const auto n = (size_t) g.note;
        humanOnTime[n] = on;
        humanDelayMs[n] = (double) (on - g.arrival) * 1000.0 / sampleRateHz;
        if (gatherReleasePending[n])
        {
            scheduleNote (on + msToSamples (80.0), g.note, 0, g.channel, false, OwnerLive);
            humanActive[n] = false;
            gatherReleasePending[n] = false;
        }
    }
    gathered.clear();
}

void ChordwrightProcessor::runAuto (int64 blockStart, int numSamples, bool hostPlaying, double hostPpq, double bpm,
                                    bool preview, const cw::EngineSettings& s)
{
    const bool running = hostPlaying || preview;
    if (! running)
    {
        if (autoRunning)
            stopOwner (OwnerAuto, blockStart);
        autoRunning = false;
        currentSlotAtomic.store (-1);
        return;
    }

    const double samplesPerBeat = sampleRateHz * 60.0 / bpm;
    const double ppqStart = hostPlaying ? hostPpq : previewPpq;
    const double ppqEnd = ppqStart + numSamples / samplesPerBeat;

    // Changing key, progression, color, voicing, pattern or chord length takes effect right away.
    const std::array<double, 6> structure { (double) s.keyRoot, (double) s.progression, (double) (int) s.color,
                                            (double) (int) s.voicing.style, (double) s.pattern, s.beatsPerChord };
    const bool structural = structure != lastStructure;
    lastStructure = structure;

    const bool jump = ! autoRunning || std::abs (ppqStart - expectedPpq) > 0.02 || structural;
    if (jump)
    {
        stopOwner (OwnerAuto, blockStart);
        generatedTo = ppqStart;
        hitScratch.clear();
        engine.ongoingHits (ppqStart, hitScratch);
        for (const auto& h : hitScratch)
            scheduleHit (h, ppqStart, blockStart, samplesPerBeat, false);
    }

    // Look ahead so notes can be pushed slightly ahead of the beat.
    const double lookBeats = cw::Humanizer::maxEarlyMs (s.human) / 1000.0 * bpm / 60.0;
    const double generateEnd = ppqEnd + lookBeats;
    if (generateEnd > generatedTo)
    {
        hitScratch.clear();
        engine.hitsInRange (generatedTo, generateEnd, hitScratch);
        for (const auto& h : hitScratch)
            scheduleHit (h, ppqStart, blockStart, samplesPerBeat, true);
        generatedTo = generateEnd;
    }

    expectedPpq = ppqEnd;
    autoRunning = true;
    if (! hostPlaying)
        previewPpq = ppqEnd;

    currentSlotAtomic.store (engine.slotAt (ppqStart + 1.0e-6));
}

void ChordwrightProcessor::scheduleHit (const cw::Hit& hit, double ppqStart, int64 blockStart,
                                        double samplesPerBeat, bool allowEarly)
{
    eventScratch.clear();
    engine.realizeHit (hit, humanizer, allowEarly, eventScratch);

    for (const auto& e : eventScratch)
    {
        const double onSamples = (hit.start - ppqStart) * samplesPerBeat + e.onMs * sampleRateHz / 1000.0;
        const int64 on = jmax (blockStart, blockStart + (int64) std::llround (onSamples));

        const double offSamples = (hit.start + e.lengthBeats - ppqStart) * samplesPerBeat + e.offMs * sampleRateHz / 1000.0;
        const int64 off = jmax (on + msToSamples (15.0), blockStart + (int64) std::llround (offSamples));

        scheduleNote (on, e.note, e.velocity, 1, true, OwnerAuto);
        scheduleNote (off, e.note, 0, 1, false, OwnerAuto);
    }
}

void ChordwrightProcessor::stopOwner (Owner owner, int64 time)
{
    pending.erase (std::remove_if (pending.begin(), pending.end(),
                                   [owner] (const Scheduled& e) { return e.owner == owner; }),
                   pending.end());

    for (int n = 0; n < 128; ++n)
        for (int k = owned[owner][(size_t) n]; k > 0; --k)
            scheduleNote (time, n, 0, lastChannel[(size_t) n], false, owner);
}

void ChordwrightProcessor::killAll (int64 time)
{
    stopOwner (OwnerPass, time);
    stopOwner (OwnerLive, time);
    stopOwner (OwnerAuto, time);
    for (auto& t : triggers) t.active = false;
    humanActive.fill (false);
    gathered.clear();
    gatherActive = false;
}

void ChordwrightProcessor::scheduleNote (int64 time, int note, int velocity, int channel, bool on, Owner owner)
{
    if (note < 0 || note > 127 || pending.size() >= pending.capacity())
        return; // never allocate on the audio thread

    const Scheduled ev { time, (int16_t) note, (int16_t) velocity, (int8_t) jlimit (1, 16, channel), on, owner };
    const auto it = std::upper_bound (pending.begin(), pending.end(), time,
                                      [] (int64 t, const Scheduled& e) { return t < e.time; });
    pending.insert (it, ev);
}

void ChordwrightProcessor::emitPending (int64 blockStart, int numSamples)
{
    const int64 blockEnd = blockStart + numSamples;
    size_t i = 0;
    for (; i < pending.size() && pending[i].time < blockEnd; ++i)
    {
        const auto& e = pending[i];
        const int pos = (int) jlimit<int64> (0, jmax (0, numSamples - 1), e.time - blockStart);
        const auto n = (size_t) e.note;

        if (e.on)
        {
            if (totalOn[n] > 0) // same pitch already sounding: retrigger it cleanly
                outBuffer.addEvent (MidiMessage::noteOff (lastChannel[n], e.note), pos);
            outBuffer.addEvent (MidiMessage::noteOn (e.channel, e.note, (uint8) jlimit (1, 127, (int) e.velocity)), pos);
            ++totalOn[n];
            ++owned[e.owner][n];
            lastChannel[n] = e.channel;
        }
        else if (owned[e.owner][n] > 0)
        {
            --owned[e.owner][n];
            if (--totalOn[n] == 0)
                outBuffer.addEvent (MidiMessage::noteOff (lastChannel[n], e.note), pos);
        }
    }
    pending.erase (pending.begin(), pending.begin() + (std::ptrdiff_t) i);
}

// ------------------------------------------------------------------ MIDI export

File ChordwrightProcessor::renderMidiFile (int repeats) const
{
    cw::Engine local;
    const auto s = readSettings();
    local.setSettings (s);
    cw::Humanizer h ((uint32_t) Random::getSystemRandom().nextInt());

    const double bpm = lastBpm();
    const double msToBeats = bpm / 60000.0;
    const int ticksPerQuarter = 960;

    struct Span { int note; int velocity; double on; double off; };
    std::vector<Span> spans;
    std::vector<cw::Hit> hits;
    std::vector<cw::NoteEvent> events;

    local.hitsInRange (0.0, local.cycleBeats() * jmax (1, repeats), hits);
    for (const auto& hit : hits)
    {
        events.clear();
        local.realizeHit (hit, h, true, events);
        for (const auto& e : events)
        {
            const double on = jmax (0.0, hit.start + e.onMs * msToBeats);
            const double off = jmax (on + 0.03, hit.start + e.lengthBeats + e.offMs * msToBeats);
            spans.push_back ({ e.note, e.velocity, on, off });
        }
    }

    // Repeated pitches must not overlap inside a MIDI clip.
    std::sort (spans.begin(), spans.end(), [] (const Span& a, const Span& b) {
        return a.note != b.note ? a.note < b.note : a.on < b.on;
    });
    for (size_t i = 1; i < spans.size(); ++i)
        if (spans[i].note == spans[i - 1].note && spans[i - 1].off > spans[i].on - 0.004)
            spans[i - 1].off = jmax (spans[i - 1].on + 0.01, spans[i].on - 0.004);

    MidiMessageSequence seq;
    for (const auto& sp : spans)
    {
        seq.addEvent (MidiMessage::noteOn (1, sp.note, (uint8) sp.velocity), std::round (sp.on * ticksPerQuarter));
        seq.addEvent (MidiMessage::noteOff (1, sp.note), std::round (sp.off * ticksPerQuarter));
    }
    seq.sort();
    seq.updateMatchedPairs();

    MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);
    file.addTrack (seq);

    const auto& prog = local.progression();
    const String keyName = String (cw::noteName (s.keyRoot, cw::keyPrefersFlats (s.keyRoot, prog.minor)))
                         + (prog.minor ? " minor" : " major");
    const String fileName = File::createLegalFileName (String::fromUTF8 (prog.name.c_str()) + " in " + keyName) + ".mid";

    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("Chordwright");
    dir.createDirectory();
    auto out = dir.getChildFile (fileName);
    out.deleteFile();
    if (FileOutputStream stream (out); stream.openedOk())
        file.writeTo (stream);
    return out;
}

// ------------------------------------------------------------------ state

void ChordwrightProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("genreFilter", genreFilter, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChordwrightProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            genreFilter = tree.getProperty ("genreFilter").toString();
            apvts.replaceState (tree);
        }
    }
}

AudioProcessorEditor* ChordwrightProcessor::createEditor()
{
    return new ChordwrightEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordwrightProcessor();
}
