#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Theory.h"
namespace xc {
inline bool writeMidi(const juce::File& file, const std::vector<Event>& events, int bpm,double length=0) {
 using namespace juce;
 if(bpm<1 || events.empty()) return false;
 MidiMessageSequence seq;
 seq.addEvent(MidiMessage::tempoMetaEvent(60000000/bpm));
 seq.addEvent(MidiMessage::timeSignatureMetaEvent(4,4));
 double end=std::isfinite(length)?jmax(0.0,length)*960:0;
 for(auto e:events) {
  if(e.note<0 || e.note>127 || e.velocity<1 || e.velocity>127 || !std::isfinite(e.duration) || !std::isfinite(e.start) || e.duration<=0 || e.start<0) return false;
  auto on=MidiMessage::noteOn(1,e.note,static_cast<uint8>(e.velocity)); on.setTimeStamp(e.start*960); seq.addEvent(on);
  auto off=MidiMessage::noteOff(1,e.note); off.setTimeStamp((e.start+e.duration)*960); seq.addEvent(off);
  end=jmax(end,(e.start+e.duration)*960);
 }
 auto final=MidiMessage::endOfTrack(); final.setTimeStamp(end); seq.addEvent(final); seq.updateMatchedPairs();
 MidiFile midi; midi.setTicksPerQuarterNote(960); midi.addTrack(seq);
 TemporaryFile temp(file); auto output=temp.getFile().createOutputStream();
 if(!output || !midi.writeTo(*output)) return false;
 output->flush(); if(output->getStatus().failed()) return false; output.reset();
 return temp.overwriteTargetFileWithTemporary();
}
}
