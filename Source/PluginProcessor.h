#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine.h"

#include <array>
#include <atomic>
#include <vector>

class ChordwrightProcessor : public juce::AudioProcessor
{
public:
    enum class PlayMode { ChordKeys = 0, AutoPlay, HumanizeInput };

    ChordwrightProcessor();
    ~ChordwrightProcessor() override = default;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameters
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    cw::EngineSettings readSettings() const;
    PlayMode playMode() const;
    int zoneOctave() const;

    // Editor helpers (message thread)
    void requestAudition (int slot) { auditionRequest.store (slot); }
    int currentSlot() const { return currentSlotAtomic.load(); }
    double lastBpm() const { return lastBpmAtomic.load(); }
    bool transportRunning() const { return autoRunningAtomic.load(); }
    juce::File renderMidiFile (int repeats) const;

    juce::String genreFilter; // UI state, saved with the song

    static constexpr int kGatherMs = 20;

private:
    enum Owner : uint8_t { OwnerPass = 0, OwnerLive, OwnerAuto, kNumOwners };

    struct Scheduled
    {
        juce::int64 time;
        int16_t note;
        int16_t velocity;
        int8_t channel;
        bool on;
        uint8_t owner;
    };

    struct Trigger
    {
        bool active = false;
        int count = 0;
        int channel = 1;
        std::array<int, 16> notes {};
        std::array<juce::int64, 16> onTimes {};
    };

    struct GatheredNote { int note; int velocity; int channel; juce::int64 arrival; };

    juce::int64 msToSamples (double ms) const { return (juce::int64) std::llround (ms * sampleRateHz / 1000.0); }

    void handleInput (const juce::MidiMessage& msg, juce::int64 time, PlayMode mode, const cw::EngineSettings& s);
    void triggerChord (const cw::Chord& chord, int triggerIndex, int velocity, int channel, juce::int64 time,
                       const cw::EngineSettings& s, int slotForUi);
    void releaseChord (int triggerIndex, juce::int64 time, const cw::EngineSettings& s);

    void humanizeNoteOn (int note, int velocity, int channel, juce::int64 time, const cw::EngineSettings& s);
    void humanizeNoteOff (int note, juce::int64 time, const cw::EngineSettings& s);
    void flushGather (juce::int64 blockEnd, const cw::EngineSettings& s, bool force);

    void runAuto (juce::int64 blockStart, int numSamples, bool hostPlaying, double hostPpq, double bpm,
                  bool preview, const cw::EngineSettings& s);
    void scheduleHit (const cw::Hit& hit, double ppqStart, juce::int64 blockStart, double samplesPerBeat,
                      bool allowEarly);
    void stopOwner (Owner owner, juce::int64 time);
    void killAll (juce::int64 time);

    void scheduleNote (juce::int64 time, int note, int velocity, int channel, bool on, Owner owner);
    void emitPending (juce::int64 blockStart, int numSamples);
    void passThrough (const juce::MidiMessage& msg, juce::int64 time);

    // Parameter pointers (read on the audio thread)
    std::atomic<float>* pKey = nullptr;
    std::atomic<float>* pProgression = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pPreview = nullptr;
    std::atomic<float>* pPattern = nullptr;
    std::atomic<float>* pChordLength = nullptr;
    std::atomic<float>* pSwing = nullptr;
    std::atomic<float>* pGate = nullptr;
    std::atomic<float>* pColor = nullptr;
    std::atomic<float>* pVoicing = nullptr;
    std::atomic<float>* pRegister = nullptr;
    std::atomic<float>* pVoiceLeading = nullptr;
    std::atomic<float>* pBass = nullptr;
    std::atomic<float>* pTiming = nullptr;
    std::atomic<float>* pLaidBack = nullptr;
    std::atomic<float>* pStrum = nullptr;
    std::atomic<float>* pStrumDir = nullptr;
    std::atomic<float>* pVelocity = nullptr;
    std::atomic<float>* pVelVar = nullptr;
    std::atomic<float>* pTopVoice = nullptr;
    std::atomic<float>* pLengthVar = nullptr;
    std::atomic<float>* pZone = nullptr;

    // Audio thread state
    cw::Engine engine;
    cw::Humanizer humanizer;
    juce::Random random;
    double sampleRateHz = 44100.0;
    juce::int64 sampleCounter = 0;
    juce::int64 blockStartSample = 0;
    int blockNumSamples = 0;
    PlayMode lastMode = PlayMode::ChordKeys;

    std::vector<Scheduled> pending;
    std::array<std::array<int, 128>, kNumOwners> owned {};
    std::array<int, 128> totalOn {};
    std::array<int, 128> lastChannel {};
    juce::MidiBuffer outBuffer;

    std::array<Trigger, 129> triggers; // 0..127 input notes, 128 = audition
    std::array<juce::int64, 128> humanOnTime {};
    std::array<double, 128> humanDelayMs {};
    std::array<bool, 128> humanActive {};
    std::array<bool, 128> gatherReleasePending {};
    std::array<int, 128> humanChannel {};
    std::vector<int> noteScratch;
    std::vector<GatheredNote> gathered;
    juce::int64 gatherStart = 0;
    bool gatherActive = false;

    bool autoRunning = false;
    double generatedTo = 0.0, expectedPpq = 0.0, previewPpq = 0.0;
    std::array<double, 6> lastStructure {};
    std::vector<cw::Hit> hitScratch;
    std::vector<cw::NoteEvent> eventScratch;

    // Shared with the editor
    std::atomic<int> auditionRequest { -1 };
    std::atomic<int> currentSlotAtomic { -1 };
    std::atomic<double> lastBpmAtomic { 120.0 };
    std::atomic<bool> autoRunningAtomic { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordwrightProcessor)
};
