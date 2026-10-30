#include <juce_audio_utils/juce_audio_utils.h>
#include "MusicalState.h"
#include "MidiExport.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
juce::Component* find(juce::Component&,const juce::String&);
void clickKey(juce::Component&,float,float);
void require(bool,const char*);
std::unique_ptr<juce::FileOutputStream> imageOutput(const juce::File&);
void runInstrumentUiTests(const juce::File& folder){
 using namespace juce;
 std::unique_ptr<AudioProcessor> processor(createPluginFilter());std::unique_ptr<AudioProcessorEditor> ui(processor->createEditorAndMakeActive());
 auto combo=[&](const char* id){auto* result=dynamic_cast<ComboBox*>(find(*ui,id));require(result!=nullptr,id);return result;};
 auto click=[&](const char* id){auto* result=dynamic_cast<Button*>(find(*ui,id));require(result && result->isEnabled(),id);result->onClick();};
 auto state=[&]{MemoryBlock memory;processor->getStateInformation(memory);return AudioProcessor::getXmlFromBinary(memory.getData(),static_cast<int>(memory.getSize()));};
 auto image=[&](const char* name){auto output=imageOutput(folder.getChildFile(name));PNGImageFormat png;require(output && png.writeImageToStream(ui->createComponentSnapshot(ui->getLocalBounds()),*output),"Instrument screenshot");};
 combo("instrument")->setSelectedId(2,sendNotificationSync);require(!find(*ui,"keyboard")->isVisible() && find(*ui,"fretboard")->isVisible(),"Instrument view switch");
 require(combo("voicing")->getNumItems()>1,"Alternative guitar shapes");combo("voicing")->setSelectedId(2,sendNotificationSync);require(state()->getIntAttribute("voicing")==1,"Shape selection persisted");
 auto* board=find(*ui,"fretboard");float cell=(board->getWidth()-48.f)/13;clickKey(*board,48+3.5f*cell,board->getHeight()-5.f);auto edited=state();require(edited->getBoolAttribute("custom") && edited->getStringAttribute("frets").startsWith("3,"),"Low E third fret edit");
 click("undo");require(!state()->getBoolAttribute("custom"),"Undo guitar edit");click("redo");require(state()->getStringAttribute("notes")==edited->getStringAttribute("notes"),"Redo guitar edit");
 auto* strum=dynamic_cast<ToggleButton*>(find(*ui,"strum"));strum->setToggleState(true,dontSendNotification);click("strum");click("progression-open");click("step-add");auto steps=state();auto* step=steps->getChildByName("Progression")->getChildElement(0);require(step->getIntAttribute("instrument")==1 && step->getBoolAttribute("strum") && step->getStringAttribute("frets")==edited->getStringAttribute("frets"),"Step saves guitar performance");click("progression-close");
 ui->setSize(1020,720);image("guitar-1020.png");ui->setSize(1600,1000);image("guitar-1600.png");
 MemoryBlock saved;processor->getStateInformation(saved);std::unique_ptr<AudioProcessor> copy(createPluginFilter());copy->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));MemoryBlock restored;copy->getStateInformation(restored);require(saved==restored,"Instrument state round trip");
 combo("instrument")->setSelectedId(4,sendNotificationSync);combo("bass-style")->setSelectedId(2,sendNotificationSync);require(state()->getIntAttribute("instrument")==3 && state()->getIntAttribute("bassPattern")==1,"Five string bass settings");ui->setSize(1180,800);image("bass-1180.png");click("progression-open");click("step-add");click("step-load");require(combo("instrument")->getSelectedId()==4 && combo("bass-style")->getSelectedId()==2,"Edit restores bass instrument and pattern");
 auto* near=dynamic_cast<ToggleButton*>(find(*ui,"near-voices"));near->setToggleState(true,dontSendNotification);click("near-voices");require(state()->getBoolAttribute("nearVoices"),"Voice leading toggle");
 // Legacy scenes have no instrument attributes and must remain keyboard scenes.
 XmlElement legacy("XChords");AudioProcessor::copyXmlToBinary(legacy,saved);copy->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));copy->getStateInformation(restored);auto old=AudioProcessor::getXmlFromBinary(restored.getData(),static_cast<int>(restored.getSize()));require(old->getIntAttribute("instrument")==0 && !old->getBoolAttribute("strum"),"Legacy state compatibility");
 // Actual exported mixed performance: strum and bass onsets survive MIDI writing.
 xc::Settings s;s.instrument=1;s.strum=true;auto events=xc::makeEvents(s);auto file=folder.getChildFile("guitar-strum.mid");require(xc::writeMidi(file,events,120),"Write guitar MIDI");MidiFile midi;auto input=file.createInputStream();require(input && midi.readFrom(*input),"Read guitar MIDI");require(midi.getLastTimestamp()==960,"Exported strum duration");
}
