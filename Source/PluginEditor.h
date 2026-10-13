#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace Palette
{
    const juce::Colour background { 0xff111318 };
    const juce::Colour panel      { 0xff1a1d23 };
    const juce::Colour raised     { 0xff23272f };
    const juce::Colour edge       { 0xff2e333d };
    const juce::Colour text       { 0xffeceae6 };
    const juce::Colour dim        { 0xff8d93a0 };
    const juce::Colour accent     { 0xfff0a93c };
    const juce::Colour accentSoft { 0xff5c431c };
    const juce::Colour ink        { 0xff16130e };
}

class ChordwrightLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChordwrightLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& background,
                               bool highlighted, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// One chord card. Click it to hear the chord.
class SlotComponent : public juce::Component
{
public:
    std::function<void()> onClick;

    void set (const juce::String& chordName, const juce::String& numeral, bool exists);
    void setActive (bool shouldBeActive);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }

private:
    juce::String name, numeral;
    bool exists = false, active = false, hover = false;
};

// Drag from here into a Studio One track to drop the humanized clip.
class DragMidiComponent : public juce::Component
{
public:
    std::function<juce::File()> renderFile;
    juce::String detail;

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }

private:
    bool dragging = false, hover = false;
};

class ChordwrightEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChordwrightEditor (ChordwrightProcessor&);
    ~ChordwrightEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Caption { juce::Rectangle<int> area; juce::String text; bool centred = true; };

    void timerCallback() override;
    void refreshTexts();
    void rebuildProgressionBox();
    void setParameter (const juce::String& id, float plainValue);

    void setupKnob (juce::Slider&, const juce::String& paramId, bool bipolar = false);
    void setupCombo (juce::ComboBox&, const juce::String& paramId);
    void setupToggle (juce::ToggleButton&, const juce::String& paramId);
    void placeKnob (juce::Slider&, juce::Rectangle<int> area, const juce::String& caption);
    void placeCombo (juce::ComboBox&, juce::Rectangle<int> area, const juce::String& caption);

    ChordwrightProcessor& proc;
    ChordwrightLookAndFeel lookAndFeel;

    std::array<juce::TextButton, 3> modeButtons;
    juce::ToggleButton previewButton { "Preview" };

    juce::ComboBox genreBox, progressionBox, keyBox, zoneBox;
    juce::Label keyLabel, heardInLabel, infoLabel, mapLabel;
    std::array<SlotComponent, 8> slots;

    juce::ComboBox patternBox, chordLengthBox;
    juce::Slider swingKnob, gateKnob;

    juce::ComboBox colorBox, voicingBox;
    juce::Slider registerKnob;
    juce::ToggleButton voiceLeadingButton { "Voice leading" }, bassButton { "Bass note" };

    juce::Slider timingKnob, laidBackKnob, strumKnob, velocityKnob, velVarKnob, topVoiceKnob, lengthVarKnob;
    juce::ComboBox strumDirBox;

    DragMidiComponent dragBox;
    juce::ComboBox repeatsBox;
    juce::TextButton saveButton { "Save MIDI" };
    std::unique_ptr<juce::FileChooser> chooser;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    std::vector<Caption> captions;
    juce::Rectangle<int> headerArea, rhythmPanel, voicingPanel, humanizePanel;

    // What is currently on screen, so the timer only redraws on change.
    int shownProgression = -1, shownKey = -1, shownColor = -1, shownMode = -1, shownZone = -99;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordwrightEditor)
};
