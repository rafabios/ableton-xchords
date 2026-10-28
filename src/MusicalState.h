#pragma once
#include "Theory.h"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace xc {
struct Settings {
 int language=1, scale=0, root=0, formula=0, octave=5, convention=4, inversion=0, pattern=0, duration=2, velocity=90, bpm=120;
 bool custom=false, loop=false, syncTempo=false;
 int timbre=0;
 std::vector<int> customNotes;
};
inline std::vector<int> selectedNotes(const Settings& s) {
 if(s.custom) return s.customNotes;
 const auto& cat=catalog(s.scale!=0);
 auto notes=pitches(s.octave*12+s.root,cat[static_cast<size_t>(std::clamp(s.formula,0,static_cast<int>(cat.size())-1))],s.scale?0:s.inversion);
 if(s.scale) notes.push_back(s.octave*12+s.root+12);
 return notes;
}
inline void toggleNote(Settings& s,int note) {
 if(note<0 || note>127) return;
 auto notes=selectedNotes(s);auto it=std::find(notes.begin(),notes.end(),note);
 if(it==notes.end()) notes.push_back(note); else notes.erase(it);
 std::sort(notes.begin(),notes.end());s.custom=true;s.customNotes=std::move(notes);
}
inline void resetNotes(Settings& s) {s.custom=false;s.customNotes.clear();}
inline std::vector<Event> makeEvents(const Settings& s) {
 return sequence(selectedNotes(s),s.pattern,std::array<double,4>{0.25,0.5,1,2}[static_cast<size_t>(s.duration)],s.velocity);
}
// A modified note collection is not itself a detected key. Only the original
// major/minor selection has an unambiguous Camelot reference here.
inline std::string referenceCamelot(const Settings& s) {
 return s.formula<=1 ? camelotCode(s.root,s.formula==1) : std::string{};
}
struct ChordStep {
 std::string name;
 std::vector<int> notes;
 double beats=4;
 int velocity=90;
 int root=0,formula=0,scale=0,octave=5;
};
inline std::vector<Event> progressionEvents(const std::vector<ChordStep>& steps) {
 std::vector<Event> result;double start=0;
 for(const auto& step:steps) {
  if(step.beats<=0 || step.beats>64)throw std::invalid_argument("Step duration");
  for(auto note:step.notes)result.push_back({note,start,step.beats,step.velocity});
  start+=step.beats;
 }
 return result;
}
}
