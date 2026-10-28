#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theory.h"
#include <array>
class CamelotWheel final : public juce::Component {
 struct Sector final : juce::Button {
  Sector():Button({}) {}
  juce::Path shape;juce::Point<float> label;juce::String code,note;juce::Colour colour;
  bool selected=false;
  bool hitTest(int x,int y) override {return shape.contains(static_cast<float>(x),static_cast<float>(y));}
  void paintButton(juce::Graphics& g,bool hover,bool down) override {
   g.setColour(colour.withMultipliedBrightness(selected?1.4f:(hover||down?1.2f:0.8f)));g.fillPath(shape);
   g.setColour(selected?juce::Colours::white:juce::Colour(0xff101620));g.strokePath(shape,juce::PathStrokeType(selected?2.5f:1.5f));
   g.setColour(juce::Colours::white);g.setFont(13.0f);g.drawText(code,juce::Rectangle<float>(label.x-23,label.y-15,46,17),juce::Justification::centred);
   g.setFont(11.0f);g.drawText(note,juce::Rectangle<float>(label.x-23,label.y+2,46,15),juce::Justification::centred);
  }
 };
 std::array<Sector,24> sectors;
public:
 std::function<void(int,bool)> onSelect;
 juce::String selectedCode;
 CamelotWheel() {
  const char* major[]{"B","F# / Gb","Db / C#","Ab","Eb","Bb","F","C","G","D","A","E"};
  const char* minor[]{"G#m","D#m","Bbm","Fm","Cm","Gm","Dm","Am","Em","Bm","F#m","C#m"};
  for(int i=0;i<24;++i) {
   const int n=i%12+1;const bool isMinor=i<12;auto& s=sectors[static_cast<size_t>(i)];
   s.code=juce::String(n)+(isMinor?"A":"B");s.note=isMinor?minor[n-1]:major[n-1];
   s.setName("Camelot "+s.code+" "+s.note);s.setComponentID("camelot-"+s.code);
   s.colour=juce::Colour::fromHSV((n-1)/12.0f,.55f,isMinor?.55f:.7f,1.0f);
   s.onClick=[this,n,isMinor]{if(onSelect)onSelect(n,isMinor);};addAndMakeVisible(s);
  }
 }
 void select(const juce::String& code) {selectedCode=code;for(auto& s:sectors){s.selected=s.code==code;s.repaint();}repaint();}
 void resized() override {
  auto centre=getLocalBounds().toFloat().getCentre();const float radius=juce::jmin(getWidth(),getHeight())*.49f;
  const auto pi=juce::MathConstants<float>::pi;
  for(int i=0;i<24;++i) {
   auto& s=sectors[static_cast<size_t>(i)];s.setBounds(getLocalBounds());s.shape.clear();
   const float inner=radius*(i<12?.38f:.70f),outer=radius*(i<12?.70f:1.0f);
   // Clock positions: 12 at the top, 1 at one o'clock.
   const float angle=(i%12+1)*pi/6,from=angle-pi/12,to=angle+pi/12;
   auto point=[centre](float r,float a){return centre+juce::Point<float>(r*std::sin(a),-r*std::cos(a));};
   s.shape.startNewSubPath(point(inner,from));
   for(int j=0;j<=16;++j)s.shape.lineTo(point(outer,from+(to-from)*j/16));
   for(int j=16;j>=0;--j)s.shape.lineTo(point(inner,from+(to-from)*j/16));s.shape.closeSubPath();
   s.label=point((inner+outer)*.5f,angle);
  }
 }
 void paint(juce::Graphics& g) override {
  g.setColour(juce::Colour(0xffaab6c9));g.setFont(13.0f);auto c=getLocalBounds().getCentre();
  g.drawText("CAMELOT",c.x-45,c.y-22,90,20,juce::Justification::centred);
  g.setColour(juce::Colour(0xff63dbc3));g.setFont(24.0f);g.drawText(selectedCode.isEmpty()?"-":selectedCode,c.x-45,c.y,90,30,juce::Justification::centred);
 }
};
