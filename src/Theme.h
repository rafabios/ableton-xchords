#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
class XChordsTheme final : public juce::LookAndFeel_V4 {
public:
 XChordsTheme() {
  using namespace juce;
  setColour(ComboBox::backgroundColourId,Colour(0xff222a37));
  setColour(ComboBox::outlineColourId,Colour(0xff364152));
  setColour(ComboBox::textColourId,Colour(0xffedf2fa));
  setColour(PopupMenu::backgroundColourId,Colour(0xff1b2330));
  setColour(PopupMenu::highlightedBackgroundColourId,Colour(0xff345950));
  setColour(TextButton::buttonColourId,Colour(0xff293443));
  setColour(TextButton::textColourOffId,Colour(0xffe5edf8));
  setColour(ToggleButton::textColourId,Colour(0xffc3cfdf));
  setColour(ToggleButton::tickColourId,Colour(0xff70e2bf));
  setColour(Slider::thumbColourId,Colour(0xff70e2bf));
  setColour(Slider::trackColourId,Colour(0xff448d7b));
  setColour(Slider::backgroundColourId,Colour(0xff2b3645));
  setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);
  setColour(Slider::textBoxBackgroundColourId,Colour(0xff111925));
  setColour(Slider::textBoxTextColourId,Colour(0xffe5edf8));
 }
 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool hover,bool down) override {
  auto r=b.getLocalBounds().toFloat().reduced(.5f);auto c=colour;
  if(hover)c=c.brighter(.12f);if(down)c=c.darker(.12f);if(!b.isEnabled())c=c.withAlpha(.35f);
  g.setGradientFill(juce::ColourGradient(c.brighter(.04f),0,0,c.darker(.08f),0,r.getBottom(),false));g.fillRoundedRectangle(r,7);
  g.setColour(juce::Colours::white.withAlpha(b.hasKeyboardFocus(true)?.35f:.09f));g.drawRoundedRectangle(r,7,1);
 }
 juce::Font getTextButtonFont(juce::TextButton&,int) override {return juce::Font(juce::FontOptions(13.0f,juce::Font::bold));}
 juce::Font getComboBoxFont(juce::ComboBox&) override {return juce::Font(juce::FontOptions(13.0f));}
};
