#pragma once
#include "Theory.h"
#include "Instruments.h"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace xc {
struct Settings {
 int language=1, scale=0, root=0, formula=0, octave=5, convention=4, inversion=0, pattern=0, duration=2, velocity=90, bpm=120;
 bool custom=false, loop=false, syncTempo=false;
 int timbre=0;
 int instrument=0,voicing=0,bassPattern=0;
 bool strum=false,nearVoices=false;
 std::vector<int> frets;
 std::vector<int> customNotes;
};
inline std::vector<int> selectedNotes(const Settings& s) {
 if(s.custom) return s.customNotes;
 if(s.instrument==1 && !s.scale){auto shapes=guitarShapes(s.root,catalog(false)[static_cast<size_t>(s.formula)]);return shapes.empty()?std::vector<int>{}:shapeNotes(shapes[static_cast<size_t>(s.voicing)%shapes.size()].frets);}
 const auto& cat=catalog(s.scale!=0);
 int low=s.instrument>=2?tuning(s.instrument).front():0;
 int base=s.instrument==1?48:s.instrument>=2?low+(s.root-low%12+12)%12-s.root:s.octave*12;
 auto notes=pitches(base+s.root,cat[static_cast<size_t>(std::clamp(s.formula,0,static_cast<int>(cat.size())-1))],s.scale || s.instrument!=0?0:s.inversion);
 if(s.scale) notes.push_back(base+s.root+12);
 return notes;
}
inline void toggleNote(Settings& s,int note) {
 if(note<0 || note>127) return;
 auto notes=selectedNotes(s);auto it=std::find(notes.begin(),notes.end(),note);
 if(it==notes.end()) notes.push_back(note); else notes.erase(it);
 std::sort(notes.begin(),notes.end());s.custom=true;s.customNotes=std::move(notes);
}
inline void resetNotes(Settings& s) {s.custom=false;s.customNotes.clear();s.frets.clear();s.voicing=0;}
inline std::vector<int> selectedFrets(const Settings& s){if(s.frets.size()==6)return s.frets;if(s.custom || s.scale)return std::vector<int>(6,-1);auto shapes=guitarShapes(s.root,catalog(false)[static_cast<size_t>(s.formula)]);return shapes.empty()?std::vector<int>(6,-1):shapes[static_cast<size_t>(s.voicing)%shapes.size()].frets;}
inline void toggleFret(Settings& s,int string,int fret){if(s.instrument!=1 || s.scale){toggleNote(s,tuning(s.instrument)[static_cast<size_t>(string)]+fret);return;}auto frets=selectedFrets(s);frets[static_cast<size_t>(string)]=frets[static_cast<size_t>(string)]==fret?-1:fret;s.frets=frets;s.custom=true;s.customNotes=shapeNotes(frets);}
inline std::vector<Event> makeEvents(const Settings& s) {
 double length=std::array<double,4>{0.25,0.5,1,2}[static_cast<size_t>(s.duration)];
 if(s.instrument>=2)return bassLine(selectedNotes(s),s.root,s.instrument,s.bassPattern,length,s.velocity);
 if(s.instrument==1 && s.strum && !s.scale){auto notes=selectedNotes(s);std::vector<Event> result;for(size_t i=0;i<notes.size();++i){double offset=std::min(.04,length/(notes.size()+1))*i;result.push_back({notes[i],offset,length-offset,s.velocity});}return result;}
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
 int instrument=0,bassPattern=0;
 bool strum=false;
 std::vector<int> frets;
};
inline std::vector<Event> progressionEvents(const std::vector<ChordStep>& steps) {
 std::vector<Event> result;double start=0;
 for(const auto& step:steps) {
  if(step.beats<=0 || step.beats>64)throw std::invalid_argument("Step duration");
  if(step.instrument>=2){auto line=bassLine(step.notes,step.root,step.instrument,step.bassPattern,step.beats,step.velocity);for(auto e:line){e.start+=start;result.push_back(e);}}
  else for(size_t i=0;i<step.notes.size();++i){double offset=step.instrument==1 && step.strum?std::min(.04,step.beats/(step.notes.size()+1))*i:0;result.push_back({step.notes[i],start+offset,step.beats-offset,step.velocity});}
  start+=step.beats;
 }
 return result;
}
inline void leadVoices(std::vector<ChordStep>& steps){for(size_t i=1;i<steps.size();++i){auto& step=steps[i];if(step.instrument==1){auto shapes=guitarShapes(step.root,catalog(step.scale!=0)[static_cast<size_t>(step.formula)]);auto pcs=[](const std::vector<int>& ns){std::vector<int> p;for(int n:ns)p.push_back(n%12);return uniqueNotes(p);};int best=movement(steps[i-1].notes,step.notes);for(const auto& shape:shapes){auto ns=shapeNotes(shape.frets);if(pcs(ns)!=pcs(step.notes))continue;int score=movement(steps[i-1].notes,ns);if(score<best){best=score;step.notes=ns;step.frets=shape.frets;}}}else if(step.instrument==0)step.notes=closeVoicing(steps[i-1].notes,step.notes,24,96);else if(!step.notes.empty() && !steps[i-1].notes.empty()){auto strings=tuning(step.instrument);int shift=0,best=128;for(int offset=-36;offset<=36;offset+=12){int root=step.notes.front()+offset;if(root<strings.front() || root>strings.back()+12)continue;int distance=std::abs(root-steps[i-1].notes.front());if(distance<best){best=distance;shift=offset;}}for(auto& n:step.notes)n+=shift;}}}
}
