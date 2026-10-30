#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MusicalState.h"
class Fretboard final:public juce::Component,public juce::SettableTooltipClient {
public:
 xc::Settings settings;std::array<bool,128> sounding{};std::function<void(int,int)> onToggle;
 std::vector<int> notes,frets;
 void updateSettings(const xc::Settings& value){settings=value;notes=xc::selectedNotes(settings);frets=xc::selectedFrets(settings);repaint();}
 void mouseMove(const juce::MouseEvent& e)override{auto strings=xc::tuning(settings.instrument);float width=(getWidth()-48.f)/13,dy=(getHeight()-22.f)/strings.size();int fret=static_cast<int>((e.position.x-48)/width),row=static_cast<int>((e.position.y-20)/dy);if(e.position.x>=48 && e.position.y>=20 && fret>=0 && fret<=12 && row>=0 && row<static_cast<int>(strings.size()))setTooltip(juce::String::fromUTF8(xc::noteNames(strings[strings.size()-1-static_cast<size_t>(row)]+fret,settings.convention).c_str()));else setTooltip({});}
 void mouseDown(const juce::MouseEvent& e)override{auto strings=xc::tuning(settings.instrument);float width=(getWidth()-48.f)/13,dy=(getHeight()-22.f)/strings.size();int fret=static_cast<int>((e.position.x-48)/width),row=static_cast<int>((e.position.y-20)/dy);if(e.position.x>=48 && e.position.y>=20 && fret>=0 && fret<=12 && row>=0 && row<static_cast<int>(strings.size()) && onToggle)onToggle(static_cast<int>(strings.size())-1-row,fret);}
 void paint(juce::Graphics& g)override{
  auto strings=xc::tuning(settings.instrument);float width=(getWidth()-48.f)/13,dy=(getHeight()-22.f)/strings.size();
  g.fillAll(juce::Colour(0xff111a26));g.setFont(10.f);
  for(int f=0;f<=12;++f){g.setColour(juce::Colour(0xff8799b0));g.drawText(juce::String(f),static_cast<int>(48+f*width),0,static_cast<int>(width),18,juce::Justification::centred);g.setColour(juce::Colour(0xff304052));g.drawVerticalLine(static_cast<int>(48+f*width),20.f,static_cast<float>(getHeight()));}
  for(size_t row=0;row<strings.size();++row){size_t string=strings.size()-1-row;float y=20+dy*(row+.5f);g.setColour(juce::Colour(0xff62758d));g.drawHorizontalLine(static_cast<int>(y),48.f,static_cast<float>(getWidth()));auto label=juce::String::fromUTF8(xc::noteNames(strings[string],settings.convention).c_str()).upToFirstOccurrenceOf(" /",false,false).upToFirstOccurrenceOf(juce::String::fromUTF8(" ·"),false,false);g.drawText(label,0,static_cast<int>(y-8),44,16,juce::Justification::centred);
   for(int f=0;f<=12;++f){int n=strings[string]+f;bool selected=settings.instrument==1 && !settings.scale?frets[string]==f:std::find(notes.begin(),notes.end(),n)!=notes.end();bool hint=std::any_of(notes.begin(),notes.end(),[&](int p){return p%12==n%12;});bool playing=sounding[static_cast<size_t>(n)];float x=48+width*(f+.5f),diam=std::min(14.f,dy-1);
    if(selected || playing){g.setColour(playing?juce::Colour(0xffffc875):juce::Colour(0xff70e2bf));g.fillEllipse(x-diam/2,y-diam/2,diam,diam);}else if(hint){g.setColour(juce::Colour(0xff526d71));g.drawEllipse(x-diam/2,y-diam/2,diam,diam,1);}
   }
  }
 }
};
