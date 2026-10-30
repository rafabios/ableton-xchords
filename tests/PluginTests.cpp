#include <juce_audio_utils/juce_audio_utils.h>
#include "MidiExport.h"
#include "MusicalState.h"
#include <iostream>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
void runFeatureTests(const juce::File& folder);
void runInstrumentUiTests(const juce::File& folder);
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
std::unique_ptr<juce::FileOutputStream> imageOutput(const juce::File& file) {
 auto out=file.createOutputStream();if(out){out->setPosition(0);out->truncate();}return out;
}
juce::Component* find(juce::Component& c,const juce::String& id) {
 if(c.getComponentID()==id)return &c;
 for(auto* child:c.getChildren())if(auto* result=find(*child,id))return result;
 return nullptr;
}
void clickKey(juce::Component& keyboard,float x,float y) {
 const juce::Point<float> point{x,y};auto time=juce::Time::getCurrentTime();
 keyboard.mouseDown(juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),point,juce::ModifierKeys::leftButtonModifier,1,0,0,0,0,&keyboard,&keyboard,time,point,time,1,false));
}
int main(int argc,char** argv) {
 using namespace juce;
 ScopedJuceInitialiser_GUI gui;
 try {
  auto folder=File::getCurrentWorkingDirectory().getChildFile("test-artifacts");require(folder.createDirectory().wasOk(),"Create artifacts");
  auto file=folder.getChildFile("C-major.mid");
  require(xc::writeMidi(file,xc::sequence({60,64,67},1,.5,90),120),"Write MIDI");
  auto input=file.createInputStream();MidiFile midi;require(input && midi.readFrom(*input),"Read MIDI");
  require(midi.getTimeFormat()==960 && midi.getNumTracks()==1,"MIDI structure");
  auto track=midi.getTrack(0);int on=0,off=0;bool tempo=false;
  for(int i=0;i<track->getNumEvents();++i) {const auto& m=track->getEventPointer(i)->message;if(m.isNoteOn()){require(m.getNoteNumber()==std::array<int,3>{60,64,67}[static_cast<size_t>(on)],"MIDI pitches");require(m.getTimeStamp()==on*480,"MIDI onset");++on;}if(m.isNoteOff())++off;if(m.isTempoMetaEvent())tempo=m.getTempoSecondsPerQuarterNote()==.5;}
  require(on==3 && off==3 && tempo && midi.getLastTimestamp()==1440,"MIDI duration and tempo");
  std::unique_ptr<AudioProcessor> processor(createPluginFilter());processor->setPlayConfigDetails(0,2,44100,512);processor->prepareToPlay(44100,512);
  AudioBuffer<float> audio(2,512);MidiBuffer events;events.addEvent(MidiMessage::noteOn(1,60,(uint8)100),0);processor->processBlock(audio,events);require(audio.getMagnitude(0,512)>.001f,"Preview synthesis");
  MemoryBlock state;processor->getStateInformation(state);require(state.getSize()>0,"Serialize state");processor->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
  std::unique_ptr<AudioProcessorEditor> editor(processor->createEditorAndMakeActive());require(editor && editor->getWidth()==1180 && editor->isResizable(),"Resizable editor");
  auto image=editor->createComponentSnapshot(editor->getLocalBounds());auto output=imageOutput(folder.getChildFile("xchords-pt.png"));PNGImageFormat png;require(output && png.writeImageToStream(image,*output),"Render editor");output.reset();
  for(int i=0;i<editor->getNumChildComponents();++i)if(auto* c=dynamic_cast<ComboBox*>(editor->getChildComponent(i)))if(c->getNumItems()==12){c->setSelectedId(11,sendNotificationSync);break;}
  processor->getStateInformation(state);std::unique_ptr<AudioProcessor> restored(createPluginFilter());restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));MemoryBlock restoredState;restored->getStateInformation(restoredState);require(state==restoredState,"Round-trip selected root");
  image=editor->createComponentSnapshot(editor->getLocalBounds());output=imageOutput(folder.getChildFile("xchords-accidentals.png"));require(output && png.writeImageToStream(image,*output),"Render accidentals");output.reset();
  if(argc>1 && String(argv[1])=="--english") {for(int i=0;i<editor->getNumChildComponents();++i)if(auto* c=dynamic_cast<ComboBox*>(editor->getChildComponent(i)))if(c->getNumItems()==2 && c->getItemText(0)=="English"){c->setSelectedId(1,sendNotificationSync);break;}image=editor->createComponentSnapshot(editor->getLocalBounds());output=imageOutput(folder.getChildFile("xchords-en.png"));require(output && png.writeImageToStream(image,*output),"Render English");output.reset();}
  auto* mode=dynamic_cast<ComboBox*>(find(*editor,"mode"));require(mode,"Mode selector");mode->setSelectedId(2,sendNotificationSync);
  auto* key=dynamic_cast<Button*>(find(*editor,"camelot-8B"));require(key,"Camelot sector");key->onClick();
  auto* wheel=key->getParentComponent();auto centre=wheel->getLocalBounds().getCentre().toFloat();float angle=8*MathConstants<float>::pi/6;float radius=wheel->getWidth()*.49f*.85f;
  auto point=centre+Point<float>(radius*std::sin(angle),-radius*std::cos(angle));require(wheel->getComponentAt(point.roundToInt())==key,"Wheel hit-test selects the correct visible sector");
  auto* root=dynamic_cast<ComboBox*>(find(*editor,"root"));require(root && root->getSelectedId()==1,"Camelot selects C");
  auto* keyboard=find(*editor,"keyboard");auto* noteList=dynamic_cast<TextEditor*>(find(*editor,"notes"));require(keyboard && noteList,"Editable keyboard");
  float w=keyboard->getWidth()/21.0f;clickKey(*keyboard,2.5f*w,100);clickKey(*keyboard,2*w,20);
  require(!noteList->getText().contains("E4") && noteList->getText().contains("D#4"),"Click removes E and adds Eb; black-key hit priority");
  xc::Settings edited;edited.scale=1;edited.pattern=1;xc::toggleNote(edited,64);xc::toggleNote(edited,63);
  auto customFile=folder.getChildFile("custom-scale.mid");require(xc::writeMidi(customFile,xc::makeEvents(edited),edited.bpm),"Export edited scale");
  auto customInput=customFile.createInputStream();MidiFile customMidi;require(customInput && customMidi.readFrom(*customInput),"Read edited MIDI");
  bool hasEb=false,hasE=false;for(int i=0;i<customMidi.getTrack(0)->getNumEvents();++i){auto m=customMidi.getTrack(0)->getEventPointer(i)->message;if(m.isNoteOn()){hasEb|=m.getNoteNumber()==63;hasE|=m.getNoteNumber()==64;}}
  require(hasEb && !hasE,"MIDI contains the replacement note, not the original");
  auto* loop=dynamic_cast<ToggleButton*>(find(*editor,"loop"));auto* play=dynamic_cast<Button*>(find(*editor,"play"));auto* stop=dynamic_cast<Button*>(find(*editor,"stop"));require(loop && play && stop,"Transport controls");
  loop->setToggleState(true,dontSendNotification);loop->onClick();play->onClick();
  for(int i=0;i<400;++i)processor->processBlock(audio,events);
  require(audio.getMagnitude(0,512)>.001f,"Loop still audible beyond one full cycle");
  // Editing while playing must rebuild the preview without losing the edited set.
  clickKey(*keyboard,2*w,20);clickKey(*keyboard,2.5f*w,100);processor->processBlock(audio,events);
  require(noteList->getText().contains("E4") && !noteList->getText().contains("D#4"),"Live edit replaces the note");
  loop->setToggleState(false,dontSendNotification);loop->onClick();
  for(int i=0;i<420;++i)processor->processBlock(audio,events);
  require(audio.getMagnitude(0,512)<.0001f,"Loop off finishes the current cycle");
  loop->setToggleState(true,dontSendNotification);loop->onClick();play->onClick();processor->processBlock(audio,events);stop->onClick();
  for(int i=0;i<50;++i)processor->processBlock(audio,events);
  require(audio.getMagnitude(0,512)<.0001f,"Stop releases notes and cancels looping");
  play->onClick();stop->onClick();processor->processBlock(audio,events);require(audio.getMagnitude(0,512)<.0001f,"Stop cancels pending preview");
  processor->getStateInformation(state);restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));restored->getStateInformation(restoredState);require(state==restoredState,"Custom notes and loop persisted");
  clickKey(*keyboard,2.5f*w,100);clickKey(*keyboard,2*w,20);
  image=editor->createComponentSnapshot(editor->getLocalBounds());output=imageOutput(folder.getChildFile("xchords-custom.png"));require(output && png.writeImageToStream(image,*output),"Render custom selection");output.reset();
  auto* reset=dynamic_cast<Button*>(find(*editor,"reset"));require(reset,"Reset");reset->onClick();
  for(int white:{0,1,2,3,4,5,6,7})clickKey(*keyboard,(white+.5f)*w,100);
  require(!play->isEnabled(),"Empty selection disables playback");
  reset->onClick();require(play->isEnabled() && noteList->getText().contains("C5"),"Restore original scale after empty selection");
  for(auto size:std::array<Point<int>,3>{{{1020,720},{1180,800},{1600,1000}}}) {
   editor->setSize(size.x,size.y);
   for(auto* child:editor->getChildren())if(child->isVisible())require(editor->getLocalBounds().contains(child->getBounds()),"Child stays inside resized editor");
   require(keyboard->getWidth()==size.x-72,"Keyboard adapts to width");
   // Hit testing must still work after the key widths change.
   auto before=noteList->getText();clickKey(*keyboard,keyboard->getWidth()/42.0f,100);require(noteList->getText()!=before,"Resized keyboard click");clickKey(*keyboard,keyboard->getWidth()/42.0f,100);
   image=editor->createComponentSnapshot(editor->getLocalBounds());output=imageOutput(folder.getChildFile("layout-"+String(size.x)+".png"));require(output && png.writeImageToStream(image,*output),"Render resized editor");output.reset();
  }
  processor->getStateInformation(state);restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
  std::unique_ptr<AudioProcessorEditor> restoredEditor(restored->createEditorAndMakeActive());require(restoredEditor->getWidth()==1600 && restoredEditor->getHeight()==1000,"Window size persists");
  processor->releaseResources();runFeatureTests(folder);runInstrumentUiTests(folder);std::cout<<"MIDI, audio, state, Camelot, keyboard, loop, resizing, presets, history, DAW tempo and progressions OK\n";
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
