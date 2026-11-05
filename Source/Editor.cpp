#include "Plugin.h"
#include "BinaryData.h"

namespace
{
// Palette from mackyar.com: navy-black, warm off-white, grey, one lime accent.
namespace C
{
    const juce::Colour bg     { 0xff080b12 };
    const juce::Colour panel  { 0xff0e121b };
    const juce::Colour raised { 0xff171c27 };
    const juce::Colour line   { 0xff222835 };
    const juce::Colour text   { 0xfff2f2ee };
    const juce::Colour dim    { 0xffa3a3a0 };
    const juce::Colour faint  { 0xff4a505b };
    const juce::Colour accent { 0xffdafa9c };
    const juce::Colour ink    { 0xff0f0f0f }; // text on accent
}

enum class W { Regular, SemiBold, Bold };

juce::Typeface::Ptr outfit (W w)
{
    static auto reg  = juce::Typeface::createSystemTypefaceFor (BinaryData::OutfitRegular_ttf, BinaryData::OutfitRegular_ttfSize);
    static auto semi = juce::Typeface::createSystemTypefaceFor (BinaryData::OutfitSemiBold_ttf, BinaryData::OutfitSemiBold_ttfSize);
    static auto bold = juce::Typeface::createSystemTypefaceFor (BinaryData::OutfitBold_ttf, BinaryData::OutfitBold_ttfSize);
    return w == W::Bold ? bold : w == W::SemiBold ? semi : reg;
}

juce::Font uiFont (float size, W w = W::Regular) { return juce::Font (juce::FontOptions (outfit (w)).withHeight (size)); }
juce::Font monoFont (float size) { return uiFont (size, W::SemiBold); } // numbers: Outfit too, one family throughout
juce::Font capsFont (float size) { return uiFont (size, W::SemiBold).withExtraKerningFactor (0.10f); }

struct Look : juce::LookAndFeel_V4
{
    Look()
    {
        setDefaultSansSerifTypeface (outfit (W::Regular)); // anything JUCE draws itself (menus, edit boxes) uses Outfit too
        setColour (juce::ComboBox::textColourId, C::text);
        setColour (juce::PopupMenu::backgroundColourId, C::raised);
        setColour (juce::PopupMenu::textColourId, C::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, C::accent.withAlpha (0.18f));
        setColour (juce::PopupMenu::highlightedTextColourId, C::text);
        setColour (juce::Slider::textBoxTextColourId, C::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, C::text);
        setColour (juce::TextEditor::backgroundColourId, C::bg);
        setColour (juce::TextEditor::highlightColourId, C::accent.withAlpha (0.3f));
        setColour (juce::CaretComponent::caretColourId, C::accent);
        setColour (juce::TextButton::buttonColourId, C::panel);
        setColour (juce::TextButton::textColourOffId, C::text);
        setColour (juce::ComboBox::outlineColourId, C::line);
        setColour (juce::HyperlinkButton::textColourId, C::accent);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        const auto r = juce::Rectangle<float> ((float) w, (float) h).reduced (0.5f);
        g.setColour (C::bg);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (box.isMouseOver (true) ? C::faint : C::line);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        juce::Path chev;
        const float cx = (float) w - 14.0f, cy = (float) h * 0.5f;
        chev.startNewSubPath (cx - 3.5f, cy - 1.5f);
        chev.lineTo (cx, cy + 2.0f);
        chev.lineTo (cx + 3.5f, cy - 1.5f);
        g.setColour (C::dim);
        g.strokePath (chev, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return uiFont (13.0f); }
    juce::Font getPopupMenuFont() override { return uiFont (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (13.0f, W::SemiBold); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 0, box.getWidth() - 30, box.getHeight());
        label.setFont (getComboBoxFont (box));
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (C::raised);
        g.setColour (C::line);
        g.drawRect (0, 0, w, h);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearBar)
            return LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);

        const auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (0.5f);
        g.setColour (C::bg);
        g.fillRoundedRectangle (r, 6.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (r, 6.0f);
            g.reduceClipRegion (clip);
            g.setColour (C::accent.withAlpha (0.2f));
            g.fillRect (r.withRight (pos));
        }
        g.setColour (s.isMouseOverOrDragging() ? C::accent.withAlpha (0.55f) : C::line);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (monoFont (12.5f));
        l->setColour (juce::Label::textColourId, C::text);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        return l;
    }
};

// About overlay: dims the editor, card in the middle, click outside or Close to dismiss.
class About : public juce::Component
{
public:
    About()
    {
        setComponentID ("about");
        const std::pair<const char*, const char*> links[] = {
            { "mackyar.com", "https://www.mackyar.com" },
            { "Spotify", "https://open.spotify.com/artist/1l0nNfz6ftZ5qgg8VcisYB" },
            { "Bandcamp", "https://mackyar.bandcamp.com/" },
            { "YouTube", "https://www.youtube.com/@MackyAr" },
            { "Instagram", "https://www.instagram.com/macky_ar/" },
            { "Email", "mailto:macky.business725@gmail.com" },
        };
        for (auto& [name, url] : links)
        {
            auto* b = linkButtons.add (new juce::HyperlinkButton (name, juce::URL (url)));
            b->setFont (uiFont (14.5f, W::SemiBold), false, juce::Justification::centredLeft);
            b->setTooltip (url);
            addAndMakeVisible (b);
        }
        close.onClick = [this] { setVisible (false); };
        addAndMakeVisible (close);
    }

    juce::Rectangle<float> cardBounds() const { return getLocalBounds().toFloat().withSizeKeepingCentre (540.0f, 410.0f); }

    void resized() override
    {
        const auto c = cardBounds().toNearestInt();
        for (int i = 0; i < linkButtons.size(); ++i)
            linkButtons[i]->setBounds (c.getX() + 32 + (i % 3) * 160, c.getY() + 248 + (i / 3) * 30, 150, 26);
        close.setBounds (c.getRight() - 32 - 88, c.getBottom() - 32 - 32, 88, 32);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (C::bg.withAlpha (0.82f));
        const auto c = cardBounds();
        g.setColour (C::panel);
        g.fillRoundedRectangle (c, 14.0f);
        g.setColour (C::line);
        g.drawRoundedRectangle (c.reduced (0.5f), 14.0f, 1.0f);

        const float x = c.getX() + 32.0f, w = c.getWidth() - 64.0f;
        g.setColour (C::text);
        g.setFont (uiFont (26.0f, W::Bold).withExtraKerningFactor (-0.03f));
        g.drawText ("StickBend", juce::Rectangle<float> (x, c.getY() + 28.0f, w, 32.0f), juce::Justification::centredLeft);
        g.setColour (C::dim);
        g.setFont (capsFont (10.5f));
        g.drawText ("VERSION " STICKBEND_VERSION "  /  BY MACKY AR", juce::Rectangle<float> (x, c.getY() + 62.0f, w, 14.0f),
                    juce::Justification::centredLeft);

        juce::AttributedString body;
        body.setLineSpacing (5.0f);
        body.append ("Play your keyboard. Bend with your thumb.\n\n", uiFont (16.0f, W::SemiBold), C::text);
        body.append ("Built by musician Macky Ar (Mahesh Arora), a multi-instrumentalist who wanted pitch bend to feel "
                     "as natural as moving a thumb. A game controller is cheap, familiar and comfortable to hold, which "
                     "makes expressive bends, swells and vibrato accessible to anyone, on any keyboard.",
                     uiFont (14.0f), C::dim);
        juce::TextLayout tl;
        tl.createLayout (body, w);
        tl.draw (g, { x, c.getY() + 96.0f, w, 160.0f });

        g.setColour (C::dim);
        g.setFont (capsFont (10.5f));
        g.drawText ("FIND MACKY AR", juce::Rectangle<float> (x, c.getY() + 224.0f, w, 14.0f), juce::Justification::centredLeft);

        g.setColour (C::faint.brighter (0.4f));
        g.setFont (uiFont (12.0f));
        g.drawFittedText ("Free and open source under AGPL-3.0.\nCollects no data and never goes online by itself.",
                          juce::Rectangle<float> (x, c.getBottom() - 70.0f, w - 110.0f, 40.0f).toNearestInt(),
                          juce::Justification::centredLeft, 2);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! cardBounds().contains (e.position)) setVisible (false);
    }

private:
    juce::OwnedArray<juce::HyperlinkButton> linkButtons;
    juce::TextButton close { "Close" };
};

using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (StickBend& p) : AudioProcessorEditor (p), proc (p)
    {
        setLookAndFeel (&look);
        auto& st = proc.state;

        for (int i = 1; i <= 4; ++i) padBox.addItem ("Pad " + juce::String (i), i);
        for (int i = 1; i <= 16; ++i) chBox.addItem ("Ch " + juce::String (i), i);
        addAndMakeVisible (padBox);
        addAndMakeVisible (chBox);
        padAtt = std::make_unique<ComboAtt> (st, "pad", padBox);
        chAtt = std::make_unique<ComboAtt> (st, "channel", chBox);

        setupBar (deadzone);
        dzAtt = std::make_unique<SliderAtt> (st, "deadzone", deadzone);
        deadzone.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
        deadzone.updateText();
        dzParam = st.getRawParameterValue ("deadzone");

        for (int i = 0; i < 4; ++i)
        {
            const juce::String id (i);
            target[i].addItemList (targetNames, 1);
            addAndMakeVisible (target[i]);
            targetAtt[i] = std::make_unique<ComboAtt> (st, "target" + id, target[i]);

            setupBar (cc[i]);
            ccAtt[i] = std::make_unique<SliderAtt> (st, "cc" + id, cc[i]);
            cc[i].textFromValueFunction = [] (double v) { return "CC " + juce::String ((int) v); };
            cc[i].updateText();

            modeParam[i] = st.getRawParameterValue ("target" + id);
        }

        aboutBtn.onClick = [this] { about.setVisible (true); about.toFront (false); };
        addAndMakeVisible (aboutBtn);
        addChildComponent (about); // last, so it sits on top

        setSize (780, 600);
        timerCallback(); // correct enabled state before first paint
        startTimerHz (60);
    }

    ~Editor() override { setLookAndFeel (nullptr); }

    void resized() override
    {
        // Header controls, right-aligned.
        chBox.setBounds (getWidth() - 24 - 88, 22, 88, 30);
        aboutBtn.setBounds (176, 25, 64, 24);
        about.setBounds (getLocalBounds());
        padBox.setBounds (chBox.getX() - 8 - 88, 22, 88, 30);

        for (int i = 0; i < 4; ++i)
        {
            const auto row = rowBounds (i);
            target[i].setBounds (row.getX() + 36, row.getY(), 142, row.getHeight());
            cc[i].setBounds (target[i].getRight() + 8, row.getY(), 78, row.getHeight());
        }

        const auto vis = visCard();
        deadzone.setBounds (vis.getRight() - 16 - 140, vis.getY() + 14, 140, 24);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (C::bg);
        paintHeader (g);
        for (int s = 0; s < 2; ++s) paintStickCard (g, s);
        paintVisualizer (g);
    }

private:
    // ---- layout -------------------------------------------------------------------------------
    juce::Rectangle<float> card (int s) const { return { 24.0f + (float) s * 374.0f, 88.0f, 358.0f, 150.0f }; }
    juce::Rectangle<int> rowBounds (int axis) const
    {
        const auto c = card (axis / 2).toNearestInt();
        return { c.getX() + 16, c.getY() + 50 + (axis % 2) * 48, c.getWidth() - 32, 32 };
    }
    juce::Rectangle<float> visCard() const { return { 24.0f, 254.0f, (float) getWidth() - 48.0f, (float) getHeight() - 254.0f - 24.0f }; }

    static void setupBarStyle (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearBar);
        s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
    }
    void setupBar (juce::Slider& s)
    {
        addAndMakeVisible (s); // parent first, so the text box is built by our LookAndFeel
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setupBarStyle (s);
    }

    static void panelBox (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (C::panel);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (C::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    }

    static void caps (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (C::dim);
        g.setFont (capsFont (10.5f));
        g.drawText (s, r, j);
    }

    // ---- header -------------------------------------------------------------------------------
    void paintHeader (juce::Graphics& g)
    {
        g.setColour (C::text);
        g.setFont (uiFont (26.0f, W::Bold).withExtraKerningFactor (-0.03f));
        g.drawText ("StickBend", 24, 16, 200, 28, juce::Justification::centredLeft);
        caps (g, "BY MACKY AR", { 25.0f, 44.0f, 200.0f, 14.0f });

        // Status pill, left of the pad/channel selectors.
        const bool on = proc.connected.load();
        const auto pill = juce::Rectangle<float> (150.0f, 30.0f).withPosition ((float) padBox.getX() - 12.0f - 150.0f, 22.0f);
        g.setColour (on ? C::accent.withAlpha (0.10f) : C::panel);
        g.fillRoundedRectangle (pill, 15.0f);
        g.setColour (on ? C::accent.withAlpha (0.35f) : C::line);
        g.drawRoundedRectangle (pill.reduced (0.5f), 15.0f, 1.0f);
        const auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ pill.getX() + 16.0f, pill.getCentreY() });
        g.setColour (on ? C::accent : C::faint);
        g.fillEllipse (dot);
        g.setColour (on ? C::text : C::dim);
        g.setFont (uiFont (12.5f));
        g.drawText (on ? "Controller live" : "No controller", pill.withTrimmedLeft (28.0f), juce::Justification::centredLeft);
    }

    // ---- mapping cards ------------------------------------------------------------------------
    void paintStickCard (juce::Graphics& g, int s)
    {
        const auto c = card (s);
        panelBox (g, c);
        caps (g, s == 0 ? "LEFT STICK" : "RIGHT STICK", { c.getX() + 16.0f, c.getY() + 14.0f, 160.0f, 16.0f });
        const auto r0 = rowBounds (s * 2).toFloat();
        caps (g, "SENDS", { (float) target[s * 2].getX(), c.getY() + 32.0f, 100.0f, 12.0f });
        caps (g, "CC", { (float) cc[s * 2].getX(), c.getY() + 32.0f, 60.0f, 12.0f });
        caps (g, "OUT", { (float) cc[s * 2].getRight() + 10.0f, c.getY() + 32.0f, r0.getRight() - (float) cc[s * 2].getRight() - 10.0f, 12.0f },
              juce::Justification::centredRight);

        for (int a = s * 2; a < s * 2 + 2; ++a)
        {
            const auto row = rowBounds (a).toFloat();
            const int mode = StickBend::toInt (modeParam[a]->load(), Off, CCUnipolar, Off);

            // Axis badge
            const auto badge = row.withWidth (28.0f);
            g.setColour (C::raised);
            g.fillRoundedRectangle (badge, 6.0f);
            g.setColour (mode == Off ? C::dim : C::text);
            g.setFont (monoFont (13.0f));
            g.drawText (a % 2 == 0 ? "X" : "Y", badge, juce::Justification::centred);

            // Live readout: number + meter
            const auto out = row.withLeft ((float) cc[a].getRight() + 10.0f);
            const float v = proc.effectiveAxis (a); // includes envelope playback, not just the physical stick
            juce::String txt = "-";
            if (mode == Bend)  txt = juce::String (toPitchBend (v) - 8192);
            else if (mode != Off) txt = juce::String (toCC (v, mode == CCUnipolar));
            if (mode == Bend && toPitchBend (v) > 8192) txt = "+" + txt;

            g.setColour (mode == Off ? C::faint : C::text);
            g.setFont (monoFont (13.0f));
            g.drawText (txt, out.withHeight (20.0f), juce::Justification::centredRight);

            const auto meter = juce::Rectangle<float> (out.getX(), out.getBottom() - 6.0f, out.getWidth(), 3.0f);
            g.setColour (C::raised);
            g.fillRoundedRectangle (meter, 1.5f);
            if (mode != Off)
            {
                g.setColour (C::accent);
                if (mode == CCUnipolar)
                    g.fillRoundedRectangle (meter.withWidth (meter.getWidth() * std::abs (v)), 1.5f);
                else
                {
                    const float cx = meter.getCentreX(), x2 = cx + v * meter.getWidth() * 0.5f;
                    g.fillRoundedRectangle (juce::Rectangle<float> (juce::Point<float> (cx, meter.getY()),
                                                                    juce::Point<float> (x2, meter.getBottom())), 1.5f);
                    g.setColour (C::faint);
                    g.fillRect (cx - 0.5f, meter.getY() - 2.0f, 1.0f, meter.getHeight() + 4.0f);
                }
            }
        }
    }

    // ---- visualizer ---------------------------------------------------------------------------
    void paintVisualizer (juce::Graphics& g)
    {
        const auto vc = visCard();
        panelBox (g, vc);
        caps (g, "CONTROLLER", { vc.getX() + 16.0f, vc.getY() + 18.0f, 160.0f, 16.0f });
        caps (g, "DEADZONE", { (float) deadzone.getX() - 90.0f, (float) deadzone.getY(), 80.0f, 24.0f }, juce::Justification::centredRight);

        const auto area = vc.withTrimmedTop (44.0f).reduced (16.0f, 8.0f);
        const float cw = 420.0f, ch = 250.0f;
        const auto c = juce::Rectangle<float> (cw, ch).withCentre (area.getCentre());
        auto P = [&] (float ux, float uy) { return juce::Point<float> (c.getX() + ux * cw, c.getY() + uy * ch); };

        const unsigned b = proc.buttons.load();
        const bool on = proc.connected.load();
        const float dz = dzParam->load();

        // Triggers as side meters
        paintTrigger (g, { c.getX() - 64.0f, c.getY() + 40.0f, 14.0f, 130.0f }, proc.trig[0].load(), "LT");
        paintTrigger (g, { c.getRight() + 50.0f, c.getY() + 40.0f, 14.0f, 130.0f }, proc.trig[1].load(), "RT");

        // Bumpers
        paintKey (g, juce::Rectangle<float> (P (0.10f, 0.04f), P (0.32f, 0.115f)), "LB", (b & Btn::LB) != 0, 7.0f);
        paintKey (g, juce::Rectangle<float> (P (0.68f, 0.04f), P (0.90f, 0.115f)), "RB", (b & Btn::RB) != 0, 7.0f);

        // Body silhouette
        const float pts[][2] = { { 0.22f, 0.14f }, { 0.40f, 0.18f }, { 0.60f, 0.18f }, { 0.78f, 0.14f }, { 0.90f, 0.21f },
                                 { 0.985f, 0.56f }, { 1.0f, 0.86f }, { 0.93f, 0.98f }, { 0.84f, 0.97f }, { 0.72f, 0.74f },
                                 { 0.28f, 0.74f }, { 0.16f, 0.97f }, { 0.07f, 0.98f }, { 0.0f, 0.86f }, { 0.015f, 0.56f }, { 0.10f, 0.21f } };
        juce::Path poly;
        poly.startNewSubPath (P (pts[0][0], pts[0][1]));
        for (auto& p : pts) poly.lineTo (P (p[0], p[1]));
        poly.closeSubPath();
        const auto body = poly.createPathWithRoundedCorners (30.0f);
        g.setGradientFill (juce::ColourGradient (C::raised, c.getCentreX(), c.getY(), C::panel.darker (0.2f), c.getCentreX(), c.getBottom(), false));
        g.fillPath (body);
        g.setColour (C::line.brighter (0.15f));
        g.strokePath (body, juce::PathStrokeType (1.2f));

        // Guide button: lit while the pad is live
        const auto guide = juce::Rectangle<float> (30.0f, 30.0f).withCentre (P (0.5f, 0.30f));
        g.setColour (C::bg);
        g.fillEllipse (guide);
        g.setColour (on ? C::accent : C::faint);
        g.drawEllipse (guide.reduced (1.0f), 1.5f);

        // Back / Start
        paintKey (g, juce::Rectangle<float> (20.0f, 10.0f).withCentre (P (0.43f, 0.43f)), {}, (b & Btn::Back) != 0, 5.0f);
        paintKey (g, juce::Rectangle<float> (20.0f, 10.0f).withCentre (P (0.57f, 0.43f)), {}, (b & Btn::Start) != 0, 5.0f);

        // Sticks
        const float r = 0.085f * cw;
        paintStick (g, P (0.25f, 0.42f), r, proc.raw[0].load(), proc.raw[1].load(), dz, (b & Btn::LS) != 0, "L");
        paintStick (g, P (0.635f, 0.60f), r, proc.raw[2].load(), proc.raw[3].load(), dz, (b & Btn::RS) != 0, "R");

        // D-pad
        const auto dp = P (0.365f, 0.60f);
        const float arm = 0.042f * cw, wid = 0.036f * cw;
        g.setColour (C::bg);
        g.fillRoundedRectangle (juce::Rectangle<float> (wid, arm * 2.0f + wid).withCentre (dp), 4.0f);
        g.fillRoundedRectangle (juce::Rectangle<float> (arm * 2.0f + wid, wid).withCentre (dp), 4.0f);
        auto dArm = [&] (unsigned bit, float dx, float dy)
        {
            if ((b & bit) == 0) return;
            g.setColour (C::accent);
            g.fillRoundedRectangle (juce::Rectangle<float> (wid - 4.0f, wid - 4.0f).withCentre (dp + juce::Point<float> (dx, dy) * arm), 3.0f);
        };
        dArm (Btn::Up, 0, -1); dArm (Btn::Down, 0, 1); dArm (Btn::Left, -1, 0); dArm (Btn::Right, 1, 0);

        // Face buttons
        const auto fc = P (0.75f, 0.42f);
        const float d = 0.054f * cw, br = 0.026f * cw;
        auto face = [&] (unsigned bit, const char* label, float dx, float dy)
        {
            const auto rr = juce::Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (fc + juce::Point<float> (dx, dy) * d);
            const bool down = (b & bit) != 0;
            g.setColour (down ? C::accent : C::bg);
            g.fillEllipse (rr);
            g.setColour (down ? C::accent : C::line.brighter (0.2f));
            g.drawEllipse (rr.reduced (0.5f), 1.0f);
            g.setColour (down ? C::ink : C::dim);
            g.setFont (uiFont (11.0f, W::SemiBold));
            g.drawText (label, rr, juce::Justification::centred);
        };
        face (Btn::Y, "Y", 0, -1); face (Btn::A, "A", 0, 1); face (Btn::X, "X", -1, 0); face (Btn::B, "B", 1, 0);

        if (! on)
        {
            g.setColour (C::dim);
            g.setFont (uiFont (12.5f));
            g.drawText ("Connect an Xbox-compatible controller, or pick another pad slot above.",
                        vc.withTrimmedTop (vc.getHeight() - 36.0f), juce::Justification::centred);
        }
    }

    static void paintKey (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label, bool down, float radius)
    {
        g.setColour (down ? C::accent : C::bg);
        g.fillRoundedRectangle (r, radius);
        g.setColour (down ? C::accent : C::line.brighter (0.2f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        if (label.isNotEmpty())
        {
            g.setColour (down ? C::ink : C::dim);
            g.setFont (capsFont (9.5f));
            g.drawText (label, r, juce::Justification::centred);
        }
    }

    static void paintTrigger (juce::Graphics& g, juce::Rectangle<float> r, float t, const juce::String& label)
    {
        g.setColour (C::bg);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (C::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
        if (t > 0.01f)
        {
            g.setColour (C::accent);
            g.fillRoundedRectangle (r.reduced (3.0f).withTrimmedTop ((r.getHeight() - 6.0f) * (1.0f - t)), 4.0f);
        }
        caps (g, label, { r.getX() - 10.0f, r.getBottom() + 6.0f, r.getWidth() + 20.0f, 14.0f }, juce::Justification::centred);
    }

    static void paintStick (juce::Graphics& g, juce::Point<float> c, float r, float x, float y, float dz, bool click, const juce::String& label)
    {
        const auto well = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
        const float kr = r * 0.36f, travel = r - kr;
        g.setColour (C::bg);
        g.fillEllipse (well);
        {
            // Deadzone is per axis: inside the vertical band X sends nothing, inside the horizontal band Y does.
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addEllipse (well);
            g.reduceClipRegion (clip);
            g.setColour (C::faint.withAlpha (0.16f));
            g.fillRect (juce::Rectangle<float> (travel * dz * 2.0f, r * 2.0f).withCentre (c));
            g.fillRect (juce::Rectangle<float> (r * 2.0f, travel * dz * 2.0f).withCentre (c));
        }
        g.setColour (click ? C::accent : C::line.brighter (0.2f));
        g.drawEllipse (well.reduced (0.5f), click ? 2.0f : 1.0f);

        const auto pos = c + juce::Point<float> (x, -y) * travel;
        const bool live = std::abs (x) > dz || std::abs (y) > dz;
        if (live)
        {
            g.setColour (C::accent.withAlpha (0.5f));
            g.drawLine ({ c, pos }, 1.5f);
        }
        const auto knob = juce::Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (pos);
        g.setColour (C::raised.brighter (0.12f));
        g.fillEllipse (knob);
        g.setColour (live ? C::accent : C::faint);
        g.drawEllipse (knob.reduced (0.75f), 1.5f);
        g.setColour (live ? C::accent : C::dim);
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (pos));

        caps (g, label, { c.x - 10.0f, well.getBottom() + 4.0f, 20.0f, 12.0f }, juce::Justification::centred);
    }

    void timerCallback() override
    {
        for (int i = 0; i < 4; ++i)
        {
            const bool isCC = StickBend::toInt (modeParam[i]->load(), Off, CCUnipolar, Off) >= CCBipolar;
            cc[i].setEnabled (isCC);
            cc[i].setAlpha (isCC ? 1.0f : 0.3f);
        }
        repaint();
    }

    StickBend& proc;
    Look look;
    juce::ComboBox padBox, chBox;
    juce::Slider deadzone;
    std::array<juce::ComboBox, 4> target;
    std::array<juce::Slider, 4> cc;
    std::unique_ptr<ComboAtt> padAtt, chAtt;
    std::unique_ptr<SliderAtt> dzAtt;
    std::array<std::unique_ptr<ComboAtt>, 4> targetAtt;
    std::array<std::unique_ptr<SliderAtt>, 4> ccAtt;
    std::atomic<float>* dzParam {};
    std::array<std::atomic<float>*, 4> modeParam {};
    juce::TextButton aboutBtn { "About" };
    About about;
};
} // namespace

juce::AudioProcessorEditor* StickBend::createEditor() { return new Editor (*this); }
