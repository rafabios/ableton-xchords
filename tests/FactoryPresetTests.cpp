#include <juce_audio_utils/juce_audio_utils.h>
#include "LibraryPanel.h"
#include "MusicalState.h"
#include "MidiExport.h"
#include <iostream>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
void require(bool,const char*);
juce::Component* find(juce::Component&,const juce::String&);
std::unique_ptr<juce::FileOutputStream> imageOutput(const juce::File&);
void runFactoryPresetTests(const juce::File& folder) {
 using namespace juce;
 auto directory=folder.getChildFile("factory-"+Uuid().toString());
 PresetLibrary library(directory,true);auto entries=library.list();require(entries.size()==1520,"1520 embedded presets");
 require(!directory.exists(),"Factory bank requires no installation of user files");
 std::unique_ptr<AudioProcessor> processor(createPluginFilter());
 int count=0,psy=0,rests=0;auto* bank=PresetLibrary::factoryBank();require(bank,"Embedded XML parses");
 for(auto* preset:bank->getChildIterator()) {
  ++count;if(preset->getStringAttribute("genre").startsWith("PSY"))++psy;
  auto* original=preset->getChildByName("XChords");require(original,"Factory state");
  MemoryBlock binary;AudioProcessor::copyXmlToBinary(*original,binary);processor->setStateInformation(binary.getData(),static_cast<int>(binary.getSize()));processor->getStateInformation(binary);
  auto restored=AudioProcessor::getXmlFromBinary(binary.getData(),static_cast<int>(binary.getSize()));auto* sequence=restored->getChildByName("Progression");
  require(sequence && sequence->getNumChildElements()==original->getChildByName("Progression")->getNumChildElements(),"Restore retains every rest and note step");
  std::vector<xc::ChordStep> steps;
  for(auto* xml:sequence->getChildIterator()) {xc::ChordStep step;step.beats=xml->getDoubleAttribute("beats");step.gate=xml->getDoubleAttribute("gate");step.rest=xml->getBoolAttribute("rest");step.velocity=xml->getIntAttribute("velocity");step.instrument=xml->getIntAttribute("instrument");step.root=xml->getIntAttribute("root");step.strum=xml->getBoolAttribute("strum");for(auto token:StringArray::fromTokens(xml->getStringAttribute("notes"),",",""))if(token.isNotEmpty())step.notes.push_back(token.getIntValue());steps.push_back(step);rests+=step.rest?1:0;}
  double length=xc::progressionLength(steps);require(std::abs(length-preset->getIntAttribute("bars")*4)<1e-8,"Factory bar lengths survive processor restore");
  auto events=xc::progressionEvents(steps);require(!events.empty(),"Factory phrase audible");
  for(auto event:events)require(event.note>=0 && event.note<=127 && std::isfinite(event.start) && std::isfinite(event.duration) && event.duration>0 && event.start+event.duration<=length+1e-8,"Factory MIDI events valid and inside loop");
  auto path=folder.getChildFile("factory-validation.mid");require(xc::writeMidi(path,events,restored->getIntAttribute("bpm"),length),"Factory MIDI export");auto input=path.createInputStream();MidiFile midi;require(input && midi.readFrom(*input),"Factory MIDI round trip");require(std::abs(midi.getLastTimestamp()-length*960)<.01,"Factory MIDI preserves full bars including trailing silence");
 }
 require(count==1520 && psy==960 && rests>1000,"Complete bank and psy coverage");
 require(library.favourite(entries.front().file,true),"Favourite factory preset");PresetLibrary reopened(directory,true);require(reopened.list(true).size()==1,"Factory favourite survives reopening");
 auto state=library.load(entries.front().file);require(state!=nullptr,"Load factory through same library API");auto userFile=library.save("Minha variante",*state);require(userFile.existsAsFile() && library.list().size()==1521,"Save own copy without changing factory bank");
 LibraryPanel panel(directory,true);panel.setSize(780,440);
 auto* search=dynamic_cast<TextEditor*>(find(panel,"preset-search"));auto* list=dynamic_cast<ComboBox*>(find(panel,"preset-list"));require(search && list,"Factory search controls");
 search->setText("guitar lead forest",false);search->onTextChange();require(list->getNumItems()==8,"AND search across instrument role and subgenre");
 search->clear();search->onTextChange();
 for(auto selection:std::vector<std::pair<String,String>>{{"preset-instrument","PIANO"},{"preset-role","BASS"},{"preset-genre","PSYTRANCE FULL-ON"}}){auto* combo=dynamic_cast<ComboBox*>(find(panel,selection.first));require(combo,"Factory filter");for(int i=0;i<combo->getNumItems();++i)if(combo->getItemText(i)==selection.second)combo->setSelectedItemIndex(i,sendNotificationSync);}
 require(list->getNumItems()==4,"Combined filters select four Full-On piano bass ideas");
 bool loaded=false;panel.restore=[&](const XmlElement& xml){loaded=xml.getChildByName("Progression")!=nullptr;};dynamic_cast<Button*>(find(panel,"preset-load"))->onClick();require(loaded,"Load filtered factory sequence");
 auto output=imageOutput(folder.getChildFile("factory-presets-pt.png"));PNGImageFormat png;require(output && png.writeImageToStream(panel.createComponentSnapshot(panel.getLocalBounds()),*output),"Factory browser screenshot");
 xc::ChordStep note;note.notes={36};note.beats=.25;note.gate=.5;xc::ChordStep rest;rest.rest=true;rest.beats=.75;
 auto gated=xc::progressionEvents({note,rest});require(gated.size()==1 && gated.front().duration==.125 && xc::progressionLength({note,rest})==1,"Gate shortens note while trailing rest preserves cycle");
 require(xc::progressionEvents({rest}).empty(),"Rest emits no MIDI note");
 std::cout<<"1520 factory presets restored and MIDI-exported; 960 psy presets; filters, favourites, rests and gates OK\n";
}
