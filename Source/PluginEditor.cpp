#include "PluginEditor.h"

using namespace juce;

static Font uiFont (float size, bool bold = false)
{
    auto options = FontOptions (size);
    return Font (bold ? options.withStyle ("Bold") : options);
}

static String studioNoteName (int midiNote)
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return String (names[midiNote % 12]) + String (midiNote / 12 - 2);
}

static String utf8 (const std::string& s) { return String::fromUTF8 (s.c_str()); }

// ================================================================== look and feel

ChordwrightLookAndFeel::ChordwrightLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, Palette::background);
    setColour (Label::textColourId, Palette::text);
    setColour (ComboBox::backgroundColourId, Palette::raised);
    setColour (ComboBox::textColourId, Palette::text);
    setColour (ComboBox::outlineColourId, Palette::edge);
    setColour (ComboBox::arrowColourId, Palette::dim);
    setColour (PopupMenu::backgroundColourId, Palette::raised);
    setColour (PopupMenu::textColourId, Palette::text);
    setColour (PopupMenu::headerTextColourId, Palette::accent);
    setColour (PopupMenu::highlightedBackgroundColourId, Palette::accent);
    setColour (PopupMenu::highlightedTextColourId, Palette::ink);
    setColour (TextButton::buttonColourId, Palette::raised);
    setColour (TextButton::buttonOnColourId, Palette::accent);
    setColour (TextButton::textColourOffId, Palette::text);
    setColour (TextButton::textColourOnId, Palette::ink);
    setColour (ToggleButton::textColourId, Palette::text);
    setColour (Slider::textBoxTextColourId, Palette::dim);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, Palette::accentSoft);
    setColour (TextEditor::backgroundColourId, Palette::raised);
    setColour (TextEditor::textColourId, Palette::text);
    setColour (TextEditor::highlightColourId, Palette::accentSoft);
    setColour (CaretComponent::caretColourId, Palette::accent);
}

void ChordwrightLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                               float startAngle, float endAngle, Slider& slider)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 5.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float track = 3.5f;

    g.setColour (Palette::raised);
    g.fillEllipse (Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre));

    Path background;
    background.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (Palette::edge);
    g.strokePath (background, PathStrokeType (track, PathStrokeType::curved, PathStrokeType::rounded));

    const bool bipolar = (bool) slider.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.01f)
    {
        Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (slider.isEnabled() ? Palette::accent : Palette::dim);
        g.strokePath (value, PathStrokeType (track, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const auto tip = centre.getPointOnCircumference (radius * 0.62f, angle);
    const auto base = centre.getPointOnCircumference (radius * 0.2f, angle);
    g.setColour (Palette::text);
    g.drawLine (Line<float> (base, tip), 2.2f);
}

void ChordwrightLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<int> (width, height).toFloat().reduced (0.5f);
    g.setColour (box.isEnabled() ? Palette::raised : Palette::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.hasKeyboardFocus (true) ? Palette::accent.withAlpha (0.6f) : Palette::edge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.5f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (Palette::dim);
    g.strokePath (chevron, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

void ChordwrightLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

Font ChordwrightLookAndFeel::getComboBoxFont (ComboBox&) { return uiFont (14.0f); }
Font ChordwrightLookAndFeel::getPopupMenuFont()          { return uiFont (14.0f); }
Font ChordwrightLookAndFeel::getTextButtonFont (TextButton&, int) { return uiFont (13.5f, true); }

void ChordwrightLookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour&, bool highlighted, bool down)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();
    auto fill = on ? Palette::accent : Palette::raised;
    if (highlighted && ! on) fill = fill.brighter (0.08f);
    if (down) fill = fill.darker (0.1f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 7.0f);
    if (! on)
    {
        g.setColour (Palette::edge);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
    }
}

void ChordwrightLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& button, bool highlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const auto pill = Rectangle<float> (34.0f, 18.0f).withY (bounds.getCentreY() - 9.0f).withX (2.0f);
    const bool on = button.getToggleState();

    g.setColour (on ? Palette::accent : (highlighted ? Palette::edge.brighter (0.1f) : Palette::edge));
    g.fillRoundedRectangle (pill, 9.0f);
    const auto knob = Rectangle<float> (14.0f, 14.0f).withCentre ({ on ? pill.getRight() - 9.0f : pill.getX() + 9.0f, pill.getCentreY() });
    g.setColour (on ? Palette::ink : Palette::dim);
    g.fillEllipse (knob);

    g.setColour (button.isEnabled() ? Palette::text : Palette::dim);
    g.setFont (uiFont (13.5f));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (44), Justification::centredLeft, 1);
}

Label* ChordwrightLookAndFeel::createSliderTextBox (Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (uiFont (12.0f));
    l->setJustificationType (Justification::centred);
    return l;
}

// ================================================================== slot card

void SlotComponent::set (const String& chordName, const String& numeralText, bool doesExist)
{
    if (name == chordName && numeral == numeralText && exists == doesExist)
        return;
    name = chordName;
    numeral = numeralText;
    exists = doesExist;
    setMouseCursor (exists ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    repaint();
}

void SlotComponent::setActive (bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}

void SlotComponent::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    if (! exists)
    {
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (Palette::edge.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 10.0f, 1.0f);
        return;
    }

    if (active)
    {
        g.setGradientFill (ColourGradient (Palette::accent, r.getX(), r.getY(), Palette::accent.darker (0.25f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 10.0f);
    }
    else
    {
        g.setColour (hover ? Palette::raised.brighter (0.06f) : Palette::raised);
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (hover ? Palette::accent.withAlpha (0.5f) : Palette::edge);
        g.drawRoundedRectangle (r, 10.0f, 1.0f);
    }

    auto content = getLocalBounds().reduced (8, 10);
    g.setColour (active ? Palette::ink : Palette::text);
    g.setFont (uiFont (name.length() > 8 ? 17.0f : 21.0f, true));
    g.drawFittedText (name, content.removeFromTop (content.getHeight() * 3 / 5), Justification::centred, 1, 0.7f);
    g.setColour (active ? Palette::ink.withAlpha (0.75f) : Palette::dim);
    g.setFont (uiFont (13.0f));
    g.drawFittedText (numeral, content, Justification::centred, 1, 0.8f);
}

void SlotComponent::mouseDown (const MouseEvent&)
{
    if (exists && onClick)
        onClick();
}

// ================================================================== drag box

void DragMidiComponent::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (dragging ? Palette::accentSoft : (hover ? Palette::raised.brighter (0.05f) : Palette::raised));
    g.fillRoundedRectangle (r, 10.0f);

    Path outline;
    outline.addRoundedRectangle (r, 10.0f);
    Path dashed;
    const float dashes[] = { 6.0f, 4.0f };
    PathStrokeType (1.4f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (hover || dragging ? Palette::accent : Palette::dim.withAlpha (0.7f));
    g.fillPath (dashed);

    auto content = getLocalBounds().reduced (16, 8);
    auto icon = content.removeFromLeft (28).toFloat();
    Path arrow;
    const auto c = icon.getCentre();
    arrow.startNewSubPath (c.x - 9.0f, c.y);
    arrow.lineTo (c.x + 8.0f, c.y);
    arrow.startNewSubPath (c.x + 2.0f, c.y - 6.0f);
    arrow.lineTo (c.x + 8.0f, c.y);
    arrow.lineTo (c.x + 2.0f, c.y + 6.0f);
    g.setColour (Palette::accent);
    g.strokePath (arrow, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

    content.removeFromLeft (10);
    g.setColour (Palette::text);
    g.setFont (uiFont (15.0f, true));
    g.drawFittedText ("Drag MIDI into a track", content.removeFromTop (content.getHeight() / 2), Justification::bottomLeft, 1);
    g.setColour (Palette::dim);
    g.setFont (uiFont (12.5f));
    g.drawFittedText (detail, content, Justification::topLeft, 1);
}

void DragMidiComponent::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || ! renderFile)
        return;

    const auto file = renderFile();
    if (! file.existsAsFile())
        return;

    dragging = true;
    repaint();
    SafePointer<DragMidiComponent> safeThis (this);
    DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this, [safeThis] {
        if (safeThis != nullptr)
        {
            safeThis->dragging = false;
            safeThis->repaint();
        }
    });
}

// ================================================================== editor

ChordwrightEditor::ChordwrightEditor (ChordwrightProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lookAndFeel);

    // Header: play mode
    const char* modeNames[] = { "Chord keys", "Auto play", "Humanize my playing" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[(size_t) i];
        b.setButtonText (modeNames[i]);
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { setParameter ("mode", (float) i); };
        addAndMakeVisible (b);
    }
    setupToggle (previewButton, "preview");
    previewButton.setTooltip ("Play the progression without pressing play in Studio One");

    // Progression row
    genreBox.addItem ("All genres", 1);
    for (int i = 0; i < (int) cw::genreNames().size(); ++i)
        genreBox.addItem (utf8 (cw::genreNames()[(size_t) i]), i + 2);
    {
        int selected = 1;
        for (int i = 0; i < (int) cw::genreNames().size(); ++i)
            if (proc.genreFilter == utf8 (cw::genreNames()[(size_t) i]))
                selected = i + 2;
        genreBox.setSelectedId (selected, dontSendNotification);
    }
    genreBox.onChange = [this] {
        const int id = genreBox.getSelectedId();
        proc.genreFilter = id <= 1 ? String() : utf8 (cw::genreNames()[(size_t) (id - 2)]);
        rebuildProgressionBox();
    };
    addAndMakeVisible (genreBox);

    progressionBox.setTextWhenNothingSelected ("Pick a progression");
    progressionBox.onChange = [this] {
        if (const int id = progressionBox.getSelectedId(); id > 0)
            setParameter ("progression", (float) (id - 1));
    };
    addAndMakeVisible (progressionBox);
    rebuildProgressionBox();

    setupCombo (keyBox, "key");
    setupCombo (zoneBox, "zone");

    for (auto* l : { &keyLabel, &heardInLabel, &infoLabel, &mapLabel })
    {
        l->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*l);
    }
    keyLabel.setFont (uiFont (18.0f, true));
    keyLabel.setColour (Label::textColourId, Palette::accent);
    heardInLabel.setFont (uiFont (13.0f));
    heardInLabel.setColour (Label::textColourId, Palette::dim);
    infoLabel.setFont (uiFont (13.0f));
    infoLabel.setColour (Label::textColourId, Palette::text.withAlpha (0.85f));
    mapLabel.setFont (uiFont (13.0f));
    mapLabel.setColour (Label::textColourId, Palette::dim);

    for (int i = 0; i < (int) slots.size(); ++i)
    {
        slots[(size_t) i].onClick = [this, i] { proc.requestAudition (i); };
        addAndMakeVisible (slots[(size_t) i]);
    }

    // Rhythm
    setupCombo (patternBox, "pattern");
    setupCombo (chordLengthBox, "chordLength");
    setupKnob (swingKnob, "swing");
    setupKnob (gateKnob, "gate");

    // Voicing
    setupCombo (colorBox, "color");
    setupCombo (voicingBox, "voicing");
    setupKnob (registerKnob, "register");
    setupToggle (voiceLeadingButton, "voiceLeading");
    setupToggle (bassButton, "bass");
    voiceLeadingButton.setTooltip ("Moves each chord to the nearest inversion so the voices glide instead of jumping");
    bassButton.setTooltip ("Adds the root (or slash bass) an octave or two below the chord");

    // Humanize
    setupKnob (timingKnob, "timing");
    setupKnob (laidBackKnob, "laidBack", true);
    setupKnob (strumKnob, "strum");
    setupCombo (strumDirBox, "strumDir");
    setupKnob (velocityKnob, "velocity");
    setupKnob (velVarKnob, "velVar");
    setupKnob (topVoiceKnob, "topVoice");
    setupKnob (lengthVarKnob, "lengthVar");
    timingKnob.setTooltip ("How far notes drift off the grid. The drift wanders like a player instead of jumping at random");
    laidBackKnob.setTooltip ("Play behind the beat (neo soul, lo-fi) or push ahead of it (house, pop)");
    strumKnob.setTooltip ("Time from the first to the last note of each chord");
    topVoiceKnob.setTooltip ("Brings the top note forward like a pianist bringing out the melody");

    // Footer
    dragBox.renderFile = [this] { return proc.renderMidiFile (repeatsBox.getSelectedId()); };
    addAndMakeVisible (dragBox);
    repeatsBox.addItem ("1 loop", 1);
    repeatsBox.addItem ("2 loops", 2);
    repeatsBox.addItem ("4 loops", 4);
    repeatsBox.setSelectedId (2, dontSendNotification);
    addAndMakeVisible (repeatsBox);
    saveButton.onClick = [this] {
        const auto file = proc.renderMidiFile (repeatsBox.getSelectedId());
        chooser = std::make_unique<FileChooser> ("Save MIDI clip",
                                                 File::getSpecialLocation (File::userDesktopDirectory).getChildFile (file.getFileName()),
                                                 "*.mid");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                                  | FileBrowserComponent::warnAboutOverwriting,
                              [file] (const FileChooser& fc) {
                                  const auto target = fc.getResult();
                                  if (target != File())
                                      file.copyFileTo (target.withFileExtension ("mid"));
                              });
    };
    addAndMakeVisible (saveButton);

    setSize (1040, 600);
    refreshTexts();
    startTimerHz (20);
}

ChordwrightEditor::~ChordwrightEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ChordwrightEditor::setParameter (const String& id, float plainValue)
{
    if (auto* param = proc.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
        param->endChangeGesture();
    }
}

void ChordwrightEditor::setupKnob (Slider& s, const String& id, bool bipolar)
{
    addAndMakeVisible (s); // attach first so the text box picks up the custom look
    s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::TextBoxBelow, false, 84, 16);
    s.getProperties().set ("bipolar", bipolar);
    s.setPopupDisplayEnabled (false, false, nullptr);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, id, s));
}

void ChordwrightEditor::setupCombo (ComboBox& box, const String& id)
{
    if (auto* param = dynamic_cast<RangedAudioParameter*> (proc.apvts.getParameter (id)))
    {
        const auto choices = param->getAllValueStrings();
        if (! choices.isEmpty())
            box.addItemList (choices, 1);
        else
            for (int v = (int) param->getNormalisableRange().start; v <= (int) param->getNormalisableRange().end; ++v)
                box.addItem (param->getText (param->convertTo0to1 ((float) v), 32), box.getNumItems() + 1);
    }
    addAndMakeVisible (box);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (proc.apvts, id, box));
}

void ChordwrightEditor::setupToggle (ToggleButton& b, const String& id)
{
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, id, b));
}

void ChordwrightEditor::rebuildProgressionBox()
{
    progressionBox.clear (dontSendNotification);
    const auto& lib = cw::progressionLibrary();
    const bool all = proc.genreFilter.isEmpty();

    for (const auto& genre : cw::genreNames())
    {
        if (! all && proc.genreFilter != utf8 (genre))
            continue;
        if (all)
            progressionBox.addSectionHeading (utf8 (genre));
        for (int i = 0; i < (int) lib.size(); ++i)
            if (lib[(size_t) i].genre == genre)
                progressionBox.addItem (utf8 (lib[(size_t) i].displayName()), i + 1);
    }
    progressionBox.setSelectedId (proc.readSettings().progression + 1, dontSendNotification);
}

void ChordwrightEditor::refreshTexts()
{
    const auto s = proc.readSettings();
    const auto mode = proc.playMode();
    const int zone = proc.zoneOctave();
    const auto& prog = cw::progressionLibrary()[(size_t) jlimit (0, (int) cw::progressionLibrary().size() - 1, s.progression)];

    shownProgression = s.progression;
    shownKey = s.keyRoot;
    shownColor = (int) s.color;
    shownMode = (int) mode;
    shownZone = zone;

    for (int i = 0; i < (int) slots.size(); ++i)
    {
        if (i < (int) prog.chords.size())
        {
            const auto colored = cw::applyColor (prog.chords[(size_t) i], s.color, prog.minor);
            slots[(size_t) i].set (utf8 (cw::chordName (colored, s.keyRoot, prog.minor)), utf8 (cw::numeralName (colored)), true);
        }
        else
        {
            slots[(size_t) i].set ({}, {}, false);
        }
    }

    keyLabel.setText (utf8 (cw::noteName (s.keyRoot, cw::keyPrefersFlats (s.keyRoot, prog.minor))) + (prog.minor ? " minor" : " major"),
                      dontSendNotification);
    heardInLabel.setText (prog.heardIn.empty() ? String() : "Same changes as: " + utf8 (prog.heardIn), dontSendNotification);

    if (progressionBox.getSelectedId() != s.progression + 1)
        progressionBox.setSelectedId (s.progression + 1, dontSendNotification);

    const int base = 48 + 12 * zone;
    switch (mode)
    {
        case ChordwrightProcessor::PlayMode::ChordKeys:
        {
            infoLabel.setText (studioNoteName (base) + " to " + studioNoteName (base + 11) + " play the chords of the key, black keys add color chords.  "
                                   + studioNoteName (base + 12) + " to " + studioNoteName (base + 24) + " play the cards above.  Other notes pass straight through.",
                               dontSendNotification);
            String map;
            for (int i = 0; i < 12; ++i)
            {
                const auto colored = cw::applyColor (cw::diatonicChords (prog.minor)[(size_t) i], s.color, prog.minor);
                map << utf8 (cw::chordName (colored, s.keyRoot, prog.minor)) << (i == 6 ? "      Black keys:  " : "   ");
            }
            mapLabel.setText ("White keys:  " + map.trim(), dontSendNotification);
            break;
        }
        case ChordwrightProcessor::PlayMode::AutoPlay:
            infoLabel.setText ("Plays the progression in time with Studio One. Press play, or switch on Preview to hear it now.", dontSendNotification);
            mapLabel.setText ("Click any chord card to hear it on its own. Keys you play pass straight through, so you can solo over the top.", dontSendNotification);
            break;
        case ChordwrightProcessor::PlayMode::HumanizeInput:
            infoLabel.setText ("Play anything. Your notes come out with the timing, velocity and strum settings below.", dontSendNotification);
            mapLabel.setText ("Strum waits 20 ms to collect the notes of a chord. Set strum to 0 ms for no added delay.", dontSendNotification);
            break;
    }

    previewButton.setVisible (mode == ChordwrightProcessor::PlayMode::AutoPlay);
    zoneBox.setVisible (mode == ChordwrightProcessor::PlayMode::ChordKeys);
    dragBox.detail = "Loops the progression with your current rhythm, voicing and feel";
    dragBox.repaint();
    resized();
    repaint();
}

void ChordwrightEditor::timerCallback()
{
    const auto s = proc.readSettings();
    const auto mode = proc.playMode();

    if (s.progression != shownProgression || s.keyRoot != shownKey || (int) s.color != shownColor
        || (int) mode != shownMode || proc.zoneOctave() != shownZone)
        refreshTexts();

    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i].setToggleState ((int) mode == i, dontSendNotification);

    const int active = mode == ChordwrightProcessor::PlayMode::HumanizeInput ? -1 : proc.currentSlot();
    for (int i = 0; i < (int) slots.size(); ++i)
        slots[(size_t) i].setActive (i == active);
}

void ChordwrightEditor::placeKnob (Slider& s, Rectangle<int> area, const String& caption)
{
    captions.push_back ({ area.removeFromTop (16), caption });
    s.setBounds (area);
}

void ChordwrightEditor::placeCombo (ComboBox& box, Rectangle<int> area, const String& caption)
{
    captions.push_back ({ area.removeFromTop (16).withTrimmedLeft (2), caption, false });
    box.setBounds (area.removeFromTop (28));
}

void ChordwrightEditor::resized()
{
    captions.clear();
    auto area = getLocalBounds().reduced (18, 0);

    // Header
    headerArea = area.removeFromTop (62);
    {
        auto h = headerArea.reduced (0, 14);
        auto right = h.removeFromRight (560);
        previewButton.setBounds (right.removeFromRight (100));
        right.removeFromRight (12);
        for (int i = 2; i >= 0; --i)
        {
            modeButtons[(size_t) i].setBounds (right.removeFromRight (i == 2 ? 168 : 120));
            right.removeFromRight (6);
        }
    }

    area.removeFromTop (6);
    {
        auto row = area.removeFromTop (30);
        genreBox.setBounds (row.removeFromLeft (210));
        row.removeFromLeft (10);
        progressionBox.setBounds (row.removeFromLeft (420));
        row.removeFromLeft (10);
        keyBox.setBounds (row.removeFromLeft (92));
        row.removeFromLeft (14);
        keyLabel.setBounds (row);
    }

    area.removeFromTop (14);
    {
        auto row = area.removeFromTop (84);
        const int gap = 8;
        const int w = (row.getWidth() - gap * 7) / 8;
        for (auto& slot : slots)
        {
            slot.setBounds (row.removeFromLeft (w));
            row.removeFromLeft (gap);
        }
    }

    area.removeFromTop (6);
    heardInLabel.setBounds (area.removeFromTop (20));

    area.removeFromTop (4);
    {
        auto info = area.removeFromTop (40);
        auto zone = info.removeFromRight (150);
        captions.push_back ({ zone.removeFromTop (14), zoneBox.isVisible() ? "Keyboard zone starts at" : "", false });
        zoneBox.setBounds (zone.removeFromTop (24).removeFromLeft (110));
        infoLabel.setBounds (info.removeFromTop (20));
        mapLabel.setBounds (info);
    }

    area.removeFromTop (12);
    {
        auto panels = area.removeFromTop (242);
        rhythmPanel = panels.removeFromLeft (252);
        panels.removeFromLeft (12);
        voicingPanel = panels.removeFromLeft (292);
        panels.removeFromLeft (12);
        humanizePanel = panels;

        auto knobRow = [] (Rectangle<int>& r) { return r.removeFromTop (94); };

        {
            auto r = rhythmPanel.reduced (14, 10);
            r.removeFromTop (26);
            placeCombo (patternBox, r.removeFromTop (46), "Pattern");
            r.removeFromTop (4);
            placeCombo (chordLengthBox, r.removeFromTop (46), "Chord length");
            r.removeFromTop (6);
            auto row = knobRow (r);
            placeKnob (swingKnob, row.removeFromLeft (row.getWidth() / 2), "Swing");
            placeKnob (gateKnob, row, "Gate");
        }
        {
            auto r = voicingPanel.reduced (14, 10);
            r.removeFromTop (26);
            auto combos = r.removeFromTop (46);
            placeCombo (colorBox, combos.removeFromLeft (combos.getWidth() / 2 - 5), "Color");
            combos.removeFromLeft (10);
            placeCombo (voicingBox, combos, "Voicing");
            r.removeFromTop (10);
            auto row = knobRow (r);
            placeKnob (registerKnob, row.removeFromLeft (110), "Register");
            row.removeFromLeft (12);
            row.removeFromTop (22);
            voiceLeadingButton.setBounds (row.removeFromTop (28));
            row.removeFromTop (6);
            bassButton.setBounds (row.removeFromTop (28));
        }
        {
            auto r = humanizePanel.reduced (14, 10);
            r.removeFromTop (26);
            const int w = r.getWidth() / 4;
            auto row1 = knobRow (r);
            r.removeFromTop (6);
            auto row2 = knobRow (r);
            placeKnob (timingKnob, row1.removeFromLeft (w), "Timing");
            placeKnob (laidBackKnob, row1.removeFromLeft (w), "Laid back");
            placeKnob (strumKnob, row1.removeFromLeft (w), "Strum");
            auto dir = row1.reduced (6, 0);
            dir.removeFromTop (18);
            placeCombo (strumDirBox, dir.removeFromTop (46), "Direction");
            placeKnob (velocityKnob, row2.removeFromLeft (w), "Velocity");
            placeKnob (velVarKnob, row2.removeFromLeft (w), "Variation");
            placeKnob (topVoiceKnob, row2.removeFromLeft (w), "Top voice");
            placeKnob (lengthVarKnob, row2, "Length");
        }
    }

    area.removeFromTop (12);
    {
        auto footer = area.removeFromTop (58);
        dragBox.setBounds (footer.removeFromLeft (560));
        footer.removeFromLeft (16);
        auto loops = footer.removeFromLeft (120);
        captions.push_back ({ loops.removeFromTop (16).translated (0, 2), "Clip length", false });
        repeatsBox.setBounds (loops.removeFromTop (30).translated (0, 4));
        footer.removeFromLeft (12);
        saveButton.setBounds (footer.removeFromLeft (120).withSizeKeepingCentre (120, 32).translated (0, 8));
    }
}

void ChordwrightEditor::paint (Graphics& g)
{
    g.fillAll (Palette::background);

    // Header
    {
        auto h = headerArea;
        g.setColour (Palette::text);
        g.setFont (uiFont (24.0f, true));
        g.drawText ("Chordwright", h.removeFromLeft (170), Justification::centredLeft);
        g.setColour (Palette::dim);
        g.setFont (uiFont (13.0f));
        g.drawText ("chord ideas, played like a person", h.removeFromLeft (260).translated (0, 2), Justification::centredLeft);
        g.setColour (Palette::edge);
        g.fillRect (headerArea.withTop (headerArea.getBottom() - 1));
    }

    auto drawPanel = [&g] (Rectangle<int> r, const String& title) {
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (r.toFloat(), 12.0f);
        g.setColour (Palette::edge);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 12.0f, 1.0f);
        g.setColour (Palette::accent);
        g.setFont (uiFont (13.0f, true));
        g.drawText (title.toUpperCase(), r.reduced (14, 10).removeFromTop (20), Justification::centredLeft);
    };
    drawPanel (rhythmPanel, "Rhythm");
    drawPanel (voicingPanel, "Voicing");
    drawPanel (humanizePanel, "Humanize");

    g.setFont (uiFont (12.0f));
    g.setColour (Palette::dim);
    for (const auto& c : captions)
        g.drawText (c.text, c.area, c.centred ? Justification::centred : Justification::centredLeft);
}
