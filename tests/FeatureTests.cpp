#include <juce_audio_utils/juce_audio_utils.h>
#include "LibraryPanel.h"
#include "MidiExport.h"
#include "MusicalState.h"
#include <iostream>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
juce::Component* find(juce::Component&,const juce::String&);
void clickKey(juce::Component&,float,float);
void require(bool,const char*);
std::unique_ptr<juce::FileOutputStream> imageOutput(const juce::File&);
namespace {
using namespace juce;
void push(Component& c,const String& id){auto* b=dynamic_cast<Button*>(find(c,id));require(b && b->isEnabled(),("Button: "+id).toRawUTF8());b->onClick();}
std::unique_ptr<XmlElement> state(AudioProcessor& p){MemoryBlock b;p.getStateInformation(b);return AudioProcessor::getXmlFromBinary(b.getData(),static_cast<int>(b.getSize()));}
void restore(AudioProcessor& p,const XmlElement& xml){MemoryBlock b;AudioProcessor::copyXmlToBinary(xml,b);p.setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
void snapshot(Component& c,const File& f){auto output=imageOutput(f);PNGImageFormat png;require(output && png.writeImageToStream(c.createComponentSnapshot(c.getLocalBounds()),*output),"Feature screenshot");}
class FakeHost final:public AudioPlayHead {
public: double bpm=60;bool available=true;
 Optional<PositionInfo> getPosition()const override {PositionInfo p;if(available)p.setBpm(bpm);return p;}
};
}
void runFeatureTests(const juce::File& folder) {
 using namespace juce;
 std::unique_ptr<AudioProcessor> p(createPluginFilter());p->setPlayConfigDetails(0,2,44100,512);p->prepareToPlay(44100,512);
 std::unique_ptr<AudioProcessorEditor> ui(p->createEditorAndMakeActive());
 auto* notes=dynamic_cast<TextEditor*>(find(*ui,"notes"));auto* keyboard=find(*ui,"keyboard");require(notes && keyboard,"Notes UI");
 auto original=notes->getText();clickKey(*keyboard,keyboard->getWidth()*2.5f/21,100);auto edited=notes->getText();require(edited!=original,"Editing updates notes");
 push(*ui,"undo");require(notes->getText()==original,"Undo restores removed note");push(*ui,"redo");require(notes->getText()==edited,"Redo reapplies removal");push(*ui,"undo");
 clickKey(*keyboard,keyboard->getWidth()/21.0f,20);require(!dynamic_cast<Button*>(find(*ui,"redo"))->isEnabled(),"New edit invalidates redo branch");push(*ui,"reset");
 push(*ui,"progression-open");push(*ui,"step-add");
 auto* rows=dynamic_cast<ListBox*>(find(*ui,"progression-list"));auto* length=dynamic_cast<Slider*>(find(*ui,"step-beats"));require(rows && length,"Progression controls");
 require(rows->getModel()->getNumRows()==1,"Add first chord");length->setValue(2,sendNotificationSync);
 push(*ui,"progression-close");dynamic_cast<ComboBox*>(find(*ui,"root"))->setSelectedId(8,sendNotificationSync);
 push(*ui,"progression-open");push(*ui,"step-add");length->setValue(3,sendNotificationSync);
 require(rows->getModel()->getNumRows()==2,"Add second chord");auto scene=state(*p);auto* steps=scene->getChildByName("Progression");require(steps && steps->getNumChildElements()==2,"Serialize progression");
 require(steps->getChildElement(0)->getDoubleAttribute("beats")==2 && steps->getChildElement(1)->getIntAttribute("root")==7,"Step duration and metadata");
 push(*ui,"step-up");scene=state(*p);require(scene->getChildByName("Progression")->getChildElement(0)->getIntAttribute("root")==7,"Reorder earlier");push(*ui,"undo");scene=state(*p);require(scene->getChildByName("Progression")->getChildElement(0)->getIntAttribute("root")==0,"Undo reorder");
 rows->selectRow(1);push(*ui,"step-load");require(dynamic_cast<ComboBox*>(find(*ui,"root"))->getSelectedId()==8,"Edit step restores original root");
 push(*ui,"progression-open");require(rows->getSelectedRow()==1,"Reopening retains edited step");push(*ui,"step-replace");require(length->getValue()==3,"Replace preserves duration");
 push(*ui,"step-remove");require(rows->getModel()->getNumRows()==1,"Remove chord");push(*ui,"undo");require(rows->getModel()->getNumRows()==2,"Undo restores removed chord");
 scene=state(*p);std::unique_ptr<AudioProcessor> second(createPluginFilter());restore(*second,*scene);require(state(*second)->isEquivalentTo(scene.get(),false),"Progression scene round trip");
 // The same event builder used by the progression export must preserve step
 // starts, individual lengths and all note-off events in the MIDI file.
 xc::ChordStep c;c.name="C";c.notes={60,64,67};c.beats=2;xc::ChordStep g;g.name="G";g.notes={67,71,74};g.beats=3;
 auto events=xc::progressionEvents({c,g});require(events.size()==6 && events[3].start==2 && events[3].duration==3,"Progression timeline");
 auto midiFile=folder.getChildFile("progression-C-G.mid");require(xc::writeMidi(midiFile,events,120),"Export progression MIDI");MidiFile midi;auto input=midiFile.createInputStream();require(input && midi.readFrom(*input),"Read progression MIDI");require(midi.getLastTimestamp()==4800,"Progression is exactly five beats");
 int ons=0,offs=0;for(int i=0;i<midi.getTrack(0)->getNumEvents();++i){auto m=midi.getTrack(0)->getEventPointer(i)->message;if(m.isNoteOn()){require(m.getTimeStamp()==(ons<3?0:1920),"Step onsets in MIDI");++ons;}if(m.isNoteOff())++offs;}require(ons==6 && offs==6,"Balanced progression MIDI events");
 // Run the actual preset UI against an isolated test directory, not AppData.
 File libraryFolder=folder.getChildFile("presets-"+Uuid().toString());LibraryPanel library(libraryFolder);library.setSize(540,340);
 library.capture=[&]{return state(*p);};library.restore=[&](const XmlElement& xml){restore(*second,xml);};
 auto* name=dynamic_cast<TextEditor*>(find(library,"preset-name"));require(name,"Preset name");name->setText(String::fromUTF8("Minha progressão"));push(library,"preset-save");push(library,"preset-favourite");
 PresetLibrary disk(libraryFolder);auto favourites=disk.list(true);require(favourites.size()==1 && favourites[0].name==String::fromUTF8("Minha progressão"),"Favourite persisted with UTF-8 name");
 push(library,"preset-load");require(state(*second)->isEquivalentTo(state(*p).get(),false),"Preset restores complete scene");
 PresetLibrary reopened(libraryFolder);require(reopened.list(true).size()==1 && reopened.load(favourites[0].file)!=nullptr,"Library survives reopening");require(reopened.save("   ",*scene)==File{},"Reject empty preset name");
 library.language(false);snapshot(library,folder.getChildFile("presets-en.png"));library.language(true);snapshot(library,folder.getChildFile("presets-pt.png"));
 for(auto size:std::array<Point<int>,2>{{{1020,720},{1180,800}}}){ui->setSize(size.x,size.y);snapshot(*ui,folder.getChildFile("progression-"+String(size.x)+".png"));}
 push(*ui,"progression-close");push(*ui,"library-open");snapshot(*ui,folder.getChildFile("library-overlay.png"));push(*ui,"preset-close");
 // Use a clean processor for deterministic timing and sounding-key tests.
 std::unique_ptr<AudioProcessor> audioP(createPluginFilter());audioP->setPlayConfigDetails(0,2,44100,512);audioP->prepareToPlay(44100,512);
 std::unique_ptr<AudioProcessorEditor> audioUI(audioP->createEditorAndMakeActive());AudioBuffer<float> buffer(2,512);MidiBuffer messages;
 auto* sync=dynamic_cast<ToggleButton*>(find(*audioUI,"sync-tempo"));auto* sound=dynamic_cast<ComboBox*>(find(*audioUI,"timbre"));require(sync && sound,"Sound controls");FakeHost host;audioP->setPlayHead(&host);sync->setToggleState(true,dontSendNotification);sync->onClick();
 push(*audioUI,"play");for(int i=0;i<64;++i)audioP->processBlock(buffer,messages);require(buffer.getMagnitude(0,512)>.001f,"60 BPM holds a one-beat chord past 0.7 seconds");
 MessageManager::getInstance()->runDispatchLoopUntil(80);auto* litKeyboard=find(*audioUI,"keyboard");auto picture=litKeyboard->createComponentSnapshot(litKeyboard->getLocalBounds());auto keyColour=picture.getPixelAt(8,20);require(keyColour.getRed()>keyColour.getGreen(),"Sounding key is amber");snapshot(*audioUI,folder.getChildFile("audition-playing.png"));
 host.bpm=240;for(int i=0;i<40;++i)audioP->processBlock(buffer,messages);require(buffer.getMagnitude(0,512)<.0001f,"Tempo change finishes the remaining beat sooner");MessageManager::getInstance()->runDispatchLoopUntil(80);require(dynamic_cast<Label*>(find(*audioUI,"tempo-status"))->getText().contains("240"),"Displays host tempo");
 host.available=false;audioP->processBlock(buffer,messages);MessageManager::getInstance()->runDispatchLoopUntil(80);require(dynamic_cast<Label*>(find(*audioUI,"tempo-status"))->getText().contains("Manual"),"Missing host tempo falls back to manual");
 audioP->setPlayHead(nullptr);sync->setToggleState(false,dontSendNotification);sync->onClick();
 std::array<float,3> samples{};for(int timbre=0;timbre<3;++timbre){sound->setSelectedId(timbre+1,sendNotificationSync);audioP->prepareToPlay(44100,512);push(*audioUI,"play");audioP->processBlock(buffer,messages);samples[static_cast<size_t>(timbre)]=buffer.getSample(0,300);for(int i=0;i<512;++i)require(std::isfinite(buffer.getSample(0,i)) && std::abs(buffer.getSample(0,i))<=1,"Finite bounded synth output");push(*audioUI,"stop");}
 require(std::abs(samples[0]-samples[1])>.0001f && std::abs(samples[0]-samples[2])>.0001f,"Timbres produce distinct audio");
 audioUI.reset();audioP->releaseResources();p->releaseResources();std::cout<<"Feature tests: presets/favourites, undo/redo, progression editing/export, timbres, note light and tempo sync passed\n";
}
