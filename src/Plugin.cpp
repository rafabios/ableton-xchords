#include <juce_audio_utils/juce_audio_utils.h>
#include "Theory.h"
#include "MidiExport.h"
#include "MusicalState.h"
#include "CamelotWheel.h"
#include "Theme.h"
#include "PresetLibrary.h"
#include "LibraryPanel.h"
#include "ProgressionPanel.h"
#include "Fretboard.h"
#include <array>
#include <atomic>
using namespace juce;
namespace {
String tr(bool pt,const char* en,const char* br) { return String::fromUTF8(pt?br:en); }
using xc::Settings;
using xc::makeEvents;
bool writeMidi(const File& file,const Settings& s) {
 return xc::writeMidi(file,makeEvents(s),s.bpm);
}
class Processor final : public AudioProcessor {
public:
 Processor():AudioProcessor(BusesProperties().withOutput("Output",AudioChannelSet::stereo(),true)) {}
 Settings settings;
 std::vector<xc::ChordStep> progression;
 int editorWidth=1180,editorHeight=800;
 const String getName() const override { return JucePlugin_Name; }
 bool acceptsMidi() const override { return true; }
 bool producesMidi() const override { return false; }
 double getTailLengthSeconds() const override { return .1; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const String getProgramName(int) override { return {}; }
 void changeProgramName(int,const String&) override {}
 bool isBusesLayoutSupported(const BusesLayout& b) const override { return b.getMainOutputChannelSet()==AudioChannelSet::stereo(); }
 void prepareToPlay(double sr,int) override { rate=sr; phase.fill(0); amp.fill(0); held.fill(0);age.fill(0);for(size_t n=0;n<128;++n){increment[n]=MathConstants<double>::twoPi*MidiMessage::getMidiNoteInHertz(static_cast<int>(n))/rate;lit[n]=false;}playing=false; previewing=false; }
 void releaseResources() override {}
 void preview(bool wholeProgression=false) {
  previewIsProgression=wholeProgression;
  const auto events=wholeProgression?xc::progressionEvents(progression):makeEvents(settings);
  const SpinLock::ScopedLockType guard(lock); pending.clear();pendingLength=wholeProgression?xc::progressionLength(progression):0;
  for(auto e:events) { pending.push_back({e.start,e.note,e.velocity/127.0f}); pending.push_back({e.start+e.duration,e.note,0});pendingLength=jmax(pendingLength,e.start+e.duration); }
  std::stable_sort(pending.begin(),pending.end(),[](auto a,auto b){return a.time<b.time;});
  stopRequested=false;loopEnabled=settings.loop;setSoundSettings();previewing=!events.empty();ready=true;
 }
 void stop() { const SpinLock::ScopedLockType guard(lock);ready=false;stopRequested=true;previewing=false; }
 bool isPreviewing() const {return previewing.load();}
 void setLoop(bool enabled) {settings.loop=enabled;loopEnabled=enabled;}
 void setSoundSettings() {manualTempo=settings.bpm;syncTempo=settings.syncTempo;sound=settings.timbre;}
 double effectiveBpm() const {return currentTempo.load();}
 bool hasHostTempo() const {return hostTempoAvailable.load();}
 bool noteIsPlaying(int n) const {return lit[static_cast<size_t>(n)].load();}
 bool previewIsProgression=false;
 std::unique_ptr<XmlElement> sceneXml() {MemoryBlock data;getStateInformation(data);return getXmlFromBinary(data.getData(),static_cast<int>(data.getSize()));}
 void restoreXml(const XmlElement& xml) {MemoryBlock data;copyXmlToBinary(xml,data);setStateInformation(data.getData(),static_cast<int>(data.getSize()));}
 void processBlock(AudioBuffer<float>& buffer,MidiBuffer& midi) override {
  ScopedNoDenormals noDenormals; buffer.clear();
  double tempo=manualTempo.load();bool foundTempo=false;
  if(syncTempo.load())if(auto* head=getPlayHead())if(auto position=head->getPosition())if(auto host=position->getBpm())if(std::isfinite(*host) && *host>0){tempo=jlimit(1.0,999.0,*host);foundTempo=true;}
  currentTempo=tempo;hostTempoAvailable=foundTempo;const double beatIncrement=tempo/(60*rate);
  const int voice=sound.load();const float attack=static_cast<float>(1-std::exp(-1/(rate*(voice==1?.035:.004))));const float release=static_cast<float>(1-std::exp(-1/(rate*.025)));
  auto gate=[this](int note,float level){auto n=static_cast<size_t>(note);if(level>0){age[n]=0;phase[n]=0;}held[n]=level;};
  { const SpinLock::ScopedTryLockType guard(lock); if(guard.isLocked() && ready) { active.swap(pending);length=pendingLength;ready=false; cursor=0; elapsed=0; held.fill(0); playing=!active.empty();previewing=playing; } }
  if(stopRequested.exchange(false)) { held.fill(0); playing=false; }
  auto it=midi.cbegin();
  for(int i=0;i<buffer.getNumSamples();++i) {
   while(it!=midi.cend() && (*it).samplePosition<=i) {
    auto m=(*it).getMessage(); if(m.isNoteOn())gate(m.getNoteNumber(),m.getFloatVelocity());
    if(m.isNoteOff()) held[static_cast<size_t>(m.getNoteNumber())]=0;
    if(m.isAllNotesOff() || m.isAllSoundOff()) held.fill(0); ++it;
   }
   while(playing && cursor<active.size() && active[cursor].time<=elapsed) { auto e=active[cursor++];gate(e.note,e.level); }
   if(playing && elapsed>=length) {
    if(loopEnabled.load() && length>0) {
     elapsed=std::fmod(elapsed,length);cursor=0;held.fill(0);
     while(cursor<active.size() && active[cursor].time<=elapsed) {auto e=active[cursor++];gate(e.note,e.level);}
    } else {playing=false;previewing=false;}
   }
   float value=0;
   for(size_t n=0;n<128;++n) {
    amp[n]+=(held[n]-amp[n])*(held[n]>amp[n]?attack:release);
    if(amp[n]<0.00001f) continue;
    phase[n]+=increment[n];age[n]+=1/rate;
    if(phase[n]>MathConstants<double>::twoPi) phase[n]-=MathConstants<double>::twoPi;
    double tone=std::sin(phase[n]);
    if(voice==0)tone=(tone+.22*std::sin(2*phase[n])+.08*std::sin(3*phase[n]))*(.45+.55*std::exp(-age[n]*2));
    if(voice==1)tone+=.11*std::sin(3*phase[n])+.04*std::sin(5*phase[n]);
    value+=static_cast<float>(tone)*amp[n]*0.065f;
   }
   for(int ch=0;ch<buffer.getNumChannels();++ch) buffer.setSample(ch,i,std::tanh(value));
   elapsed+=beatIncrement;
  }
  for(size_t n=0;n<128;++n)lit[n]=held[n]>0;
  midi.clear();
 }
 bool hasEditor() const override {return true;}
 AudioProcessorEditor* createEditor() override;
 void getStateInformation(MemoryBlock& data) override {
  XmlElement xml("XChords"); auto s=settings;
  xml.setAttribute("editorWidth",editorWidth);xml.setAttribute("editorHeight",editorHeight);
  xml.setAttribute("language",s.language); xml.setAttribute("scale",s.scale); xml.setAttribute("root",s.root); xml.setAttribute("formula",s.formula);
  xml.setAttribute("octave",s.octave); xml.setAttribute("convention",s.convention); xml.setAttribute("inversion",s.inversion); xml.setAttribute("pattern",s.pattern);
  xml.setAttribute("duration",s.duration); xml.setAttribute("velocity",s.velocity); xml.setAttribute("bpm",s.bpm);
  xml.setAttribute("custom",s.custom);xml.setAttribute("loop",s.loop);
  xml.setAttribute("instrument",s.instrument);xml.setAttribute("voicing",s.voicing);xml.setAttribute("bassPattern",s.bassPattern);xml.setAttribute("strum",s.strum);xml.setAttribute("nearVoices",s.nearVoices);StringArray fs;for(int f:s.frets)fs.add(String(f));xml.setAttribute("frets",fs.joinIntoString(","));
  xml.setAttribute("timbre",s.timbre);xml.setAttribute("syncTempo",s.syncTempo);
  auto* steps=xml.createNewChildElement("Progression");
  for(const auto& step:progression){auto* item=steps->createNewChildElement("Step");item->setAttribute("name",String::fromUTF8(step.name.c_str()));item->setAttribute("beats",step.beats);item->setAttribute("rest",step.rest);item->setAttribute("gate",step.gate);item->setAttribute("velocity",step.velocity);item->setAttribute("root",step.root);item->setAttribute("formula",step.formula);item->setAttribute("scale",step.scale);item->setAttribute("octave",step.octave);item->setAttribute("inversion",step.inversion);item->setAttribute("instrument",step.instrument);item->setAttribute("bassPattern",step.bassPattern);item->setAttribute("strum",step.strum);StringArray frets;for(int f:step.frets)frets.add(String(f));item->setAttribute("frets",frets.joinIntoString(","));StringArray ns;for(int n:step.notes)ns.add(String(n));item->setAttribute("notes",ns.joinIntoString(","));}
  StringArray pitches;for(auto n:s.customNotes)pitches.add(String(n));xml.setAttribute("notes",pitches.joinIntoString(","));copyXmlToBinary(xml,data);
 }
 void setStateInformation(const void* data,int size) override {
  auto xml=getXmlFromBinary(data,size); if(!xml || !xml->hasTagName("XChords")) return;
  auto& s=settings;
  editorWidth=jlimit(1020,1800,xml->getIntAttribute("editorWidth",1180));editorHeight=jlimit(720,1200,xml->getIntAttribute("editorHeight",800));
  s.language=jlimit(0,1,xml->getIntAttribute("language",1)); s.scale=jlimit(0,1,xml->getIntAttribute("scale"));
  s.root=jlimit(0,11,xml->getIntAttribute("root")); s.formula=jlimit(0,static_cast<int>(xc::catalog(s.scale!=0).size())-1,xml->getIntAttribute("formula"));
  s.octave=jlimit(2,6,xml->getIntAttribute("octave",5)); s.convention=jlimit(3,4,xml->getIntAttribute("convention",4));
  s.inversion=jlimit(0,4,xml->getIntAttribute("inversion")); s.pattern=jlimit(0,3,xml->getIntAttribute("pattern"));
  s.duration=jlimit(0,3,xml->getIntAttribute("duration",2)); s.velocity=jlimit(1,127,xml->getIntAttribute("velocity",90)); s.bpm=jlimit(40,240,xml->getIntAttribute("bpm",120));
  s.custom=xml->getBoolAttribute("custom",false);s.loop=xml->getBoolAttribute("loop",false);s.customNotes.clear();
  for(auto token:StringArray::fromTokens(xml->getStringAttribute("notes"),",","")) {
   if(token.isEmpty() || token.containsOnly("0123456789")==false)continue;
   auto n=token.getIntValue();if(n>=0 && n<=127)s.customNotes.push_back(n);
  }
  std::sort(s.customNotes.begin(),s.customNotes.end());s.customNotes.erase(std::unique(s.customNotes.begin(),s.customNotes.end()),s.customNotes.end());
  s.instrument=jlimit(0,3,xml->getIntAttribute("instrument"));s.voicing=jlimit(0,63,xml->getIntAttribute("voicing"));s.bassPattern=jlimit(0,2,xml->getIntAttribute("bassPattern"));s.strum=xml->getBoolAttribute("strum");s.nearVoices=xml->getBoolAttribute("nearVoices");s.frets.clear();for(auto token:StringArray::fromTokens(xml->getStringAttribute("frets"),",",""))s.frets.push_back(jlimit(-1,12,token.getIntValue()));if(s.frets.size()!=6)s.frets.clear();
  s.timbre=jlimit(0,2,xml->getIntAttribute("timbre"));s.syncTempo=xml->getBoolAttribute("syncTempo",false);
  progression.clear();if(auto* steps=xml->getChildByName("Progression"))for(auto* item:steps->getChildIterator()) {
   if(!item->hasTagName("Step") || progression.size()>=128)continue;
   xc::ChordStep step;step.rest=item->getBoolAttribute("rest");double gate=item->getDoubleAttribute("gate",1);step.gate=std::isfinite(gate)?jlimit(.05,1.0,gate):1;step.name=item->getStringAttribute("name").substring(0,100).toStdString();double beats=item->getDoubleAttribute("beats",4);step.beats=std::isfinite(beats)?jlimit(.25,64.0,beats):4;step.velocity=jlimit(1,127,item->getIntAttribute("velocity",90));
   step.inversion=jlimit(0,4,item->getIntAttribute("inversion"));step.instrument=jlimit(0,3,item->getIntAttribute("instrument"));step.bassPattern=jlimit(0,2,item->getIntAttribute("bassPattern"));step.strum=item->getBoolAttribute("strum");for(auto token:StringArray::fromTokens(item->getStringAttribute("frets"),",",""))step.frets.push_back(jlimit(-1,12,token.getIntValue()));if(step.frets.size()!=6)step.frets.clear();
   step.root=jlimit(0,11,item->getIntAttribute("root"));step.scale=jlimit(0,1,item->getIntAttribute("scale"));step.formula=jlimit(0,static_cast<int>(xc::catalog(step.scale!=0).size())-1,item->getIntAttribute("formula"));step.octave=jlimit(2,6,item->getIntAttribute("octave",5));
   for(auto token:StringArray::fromTokens(item->getStringAttribute("notes"),",",""))if(token.isNotEmpty() && token.containsOnly("0123456789")){int n=token.getIntValue();if(n>=0 && n<=127)step.notes.push_back(n);}
   std::sort(step.notes.begin(),step.notes.end());step.notes.erase(std::unique(step.notes.begin(),step.notes.end()),step.notes.end());if(!step.notes.empty() || step.rest)progression.push_back(std::move(step));
  }
  loopEnabled=s.loop;setSoundSettings();stop();
 }
private:
 struct Timed { double time; int note; float level; };
 SpinLock lock; std::vector<Timed> pending,active; bool ready=false,playing=false;
 std::atomic<bool> stopRequested{false},loopEnabled{false},previewing{false}; size_t cursor=0; double rate=44100,elapsed=0,length=0,pendingLength=0;
 std::array<double,128> phase{},increment{},age{}; std::array<float,128> held{},amp{};
 std::array<std::atomic<bool>,128> lit{};
 std::atomic<int> manualTempo{120},sound{0};std::atomic<double> currentTempo{120};std::atomic<bool> syncTempo{false},hostTempoAvailable{false};
};
class Keyboard final : public Component {
public:
 std::vector<int> notes; int start=48,convention=4;
 std::array<bool,128> sounding{};
 std::function<void(int)> onToggle;
 int noteAt(Point<float> p) const {
  const float w=getWidth()/21.0f;
  // Black keys must be hit-tested first, because they overlap the whites.
  for(bool blackPass:{true,false}) {
   int white=0;
   for(int i=0;i<36;++i) {
    const int pc=(start+i)%12;const bool black=pc==1||pc==3||pc==6||pc==8||pc==10;
    auto r=black?Rectangle<float>(white*w-w*.3f,0,w*.6f,getHeight()*.59f):Rectangle<float>(white*w,0,w,static_cast<float>(getHeight()));
    if(black==blackPass && r.contains(p))return start+i;
    if(!black)++white;
   }
  }
  return -1;
 }
 void mouseDown(const MouseEvent& e) override {auto n=noteAt(e.position);if(n>=0 && onToggle)onToggle(n);}
 void paint(Graphics& g) override {
  const int count=36; const float w=getWidth()/21.0f; int white=0;
  for(int i=0;i<count;++i) { int pc=(start+i)%12; bool black=pc==1||pc==3||pc==6||pc==8||pc==10; if(black) continue;
   draw(g,start+i,{white*w,0,w-1,static_cast<float>(getHeight())},false); ++white;
  }
  white=0;
  for(int i=0;i<count;++i) { int pc=(start+i)%12; bool black=pc==1||pc==3||pc==6||pc==8||pc==10;
   if(black) draw(g,start+i,{white*w-w*.3f,0,w*.6f,getHeight()*.59f},true); else ++white;
  }
 }
 void draw(Graphics& g,int n,Rectangle<float> r,bool black) {
  bool selected=std::find(notes.begin(),notes.end(),n)!=notes.end();
  auto base=sounding[static_cast<size_t>(n)]?Colour(0xffffc875):(selected?Colour(0xff70e2bf):(black?Colour(0xff202634):Colour(0xffe8ecf3)));
  g.setGradientFill(ColourGradient(base.brighter(.05f),r.getX(),0,base.darker(black?.4f:.1f),r.getX(),r.getBottom(),false));g.fillRoundedRectangle(r,3);
  g.setColour(black&&!selected?Colours::white:Colour(0xff142233)); g.setFont(11.0f);
  const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  g.drawText(String(names[n%12])+String(n/12-5+convention),r.removeFromBottom(23),Justification::centred);
 }
};
class DragButton final : public TextButton {
public: std::function<void()> drag; bool sent=false;
 void mouseDown(const MouseEvent& e) override { sent=false; TextButton::mouseDown(e); }
 void mouseDrag(const MouseEvent& e) override { if(!sent && e.getDistanceFromDragStart()>5) {sent=true;if(drag)drag();} }
};
class Editor final : public AudioProcessorEditor, private Timer {
public:
 explicit Editor(Processor& p):AudioProcessorEditor(p),processor(p),progressionPanel(p.progression) {
  const int initialWidth=p.editorWidth,initialHeight=p.editorHeight;
  setLookAndFeel(&theme);
  setResizable(true,true);setResizeLimits(1020,720,1800,1200);
  setSize(initialWidth,initialHeight);
  for(auto* c:std::vector<Component*>{&language,&mode,&root,&formula,&octave,&convention,&inversion,&pattern,&duration,&velocity,&bpm,&play,&stop,&random,&undo,&save,&drag,&lockRoot,&keyboard,&notes,&status,&loop,&reset,&wheel,&redo,&libraryOpen,&progressionOpen,&timbre,&sync,&tempoLabel,&instrument,&voicing,&bassStyle,&strum,&nearVoices,&fretboard}) addAndMakeVisible(c);
  addChildComponent(library);addChildComponent(progressionPanel);
  instrument.setComponentID("instrument");voicing.setComponentID("voicing");bassStyle.setComponentID("bass-style");strum.setComponentID("strum");nearVoices.setComponentID("near-voices");fretboard.setComponentID("fretboard");
  undo.setComponentID("undo");redo.setComponentID("redo");libraryOpen.setComponentID("library-open");progressionOpen.setComponentID("progression-open");timbre.setComponentID("timbre");sync.setComponentID("sync-tempo");tempoLabel.setComponentID("tempo-status");
  mode.setComponentID("mode");root.setComponentID("root");formula.setComponentID("formula");keyboard.setComponentID("keyboard");notes.setComponentID("notes");
  play.setComponentID("play");stop.setComponentID("stop");loop.setComponentID("loop");reset.setComponentID("reset");
  fretboard.onToggle=[this](int string,int fret){remember();xc::toggleFret(processor.settings,string,fret);update(true);};
  keyboard.setMouseCursor(MouseCursor::PointingHandCursor);
  keyboard.onToggle=[this](int n){remember();xc::toggleNote(processor.settings,n);update(true);};
  wheel.onSelect=[this](int n,bool minor){remember();auto& s=processor.settings;s.root=xc::camelotRoot(n,minor);s.formula=minor?1:0;s.inversion=0;xc::resetNotes(s);refresh(true);};
  notes.setMultiLine(true); notes.setReadOnly(true); notes.setScrollbarsShown(true); notes.setColour(TextEditor::backgroundColourId,Colour(0xff171e2a)); notes.setColour(TextEditor::textColourId,Colour(0xffe8ecf3));
  notes.setFont(16.0f); status.setColour(Label::textColourId,Colour(0xff63dbc3));
  notes.setFont(14.0f);notes.setColour(TextEditor::backgroundColourId,Colour(0xff141c28));notes.setColour(TextEditor::outlineColourId,Colours::transparentBlack);
  play.setColour(TextButton::buttonColourId,Colour(0xff70e2bf));play.setColour(TextButton::textColourOffId,Colour(0xff102c25));
  drag.setColour(TextButton::buttonColourId,Colour(0xff355d76));
  language.addItem("English",1);language.addItem(String::fromUTF8("Português"),2);
  language.setSelectedId(p.settings.language+1,dontSendNotification);
  language.onChange=[this]{processor.settings.language=language.getSelectedId()-1; refresh();};
  instrument.onChange=[this]{auto& s=processor.settings;s.instrument=instrument.getSelectedId()-1;xc::resetNotes(s);refresh(true);};
  voicing.onChange=[this]{auto& s=processor.settings;xc::resetNotes(s);s.voicing=voicing.getSelectedId()-1;update(true);};
  bassStyle.onChange=[this]{processor.settings.bassPattern=bassStyle.getSelectedId()-1;update(true);};
  strum.onClick=[this]{processor.settings.strum=strum.getToggleState();update(true);};
  nearVoices.onClick=[this]{processor.settings.nearVoices=nearVoices.getToggleState();if(processor.settings.nearVoices)xc::leadVoices(processor.progression);progressionPanel.refresh();update();if(processor.isPreviewing())processor.preview(processor.previewIsProgression);};
  mode.onChange=[this]{auto& s=processor.settings;s.scale=mode.getSelectedId()-1;s.formula=0;s.inversion=0;s.pattern=s.scale?1:0;xc::resetNotes(s);refresh(true);};
  root.onChange=[this]{processor.settings.root=root.getSelectedId()-1;xc::resetNotes(processor.settings);refresh(true);};
  formula.onChange=[this]{processor.settings.formula=formula.getSelectedId()-1;processor.settings.inversion=0;xc::resetNotes(processor.settings);refresh(true);};
  octave.onChange=[this]{auto& s=processor.settings;int newOct=octave.getSelectedId()+1;if(s.custom)for(auto& n:s.customNotes)n=jlimit(0,127,n+(newOct-s.octave)*12);s.octave=newOct;update(true);};
  convention.onChange=[this]{processor.settings.convention=convention.getSelectedId()+2;refresh();};
  inversion.onChange=[this]{processor.settings.inversion=inversion.getSelectedId()-1;xc::resetNotes(processor.settings);refresh(true);};
  pattern.onChange=[this]{processor.settings.pattern=pattern.getSelectedId()-1;update(true);};
  duration.onChange=[this]{processor.settings.duration=duration.getSelectedId()-1;update(true);};
  velocity.setRange(1,127,1); bpm.setRange(40,240,1);
  for(auto* slider:{&velocity,&bpm}) {slider->setSliderStyle(Slider::LinearHorizontal);slider->setTextBoxStyle(Slider::TextBoxRight,false,50,24);}
  velocity.onValueChange=[this]{processor.settings.velocity=static_cast<int>(velocity.getValue());update(true);};
  bpm.onValueChange=[this]{processor.settings.bpm=static_cast<int>(bpm.getValue());update(true);};
  play.onClick=[this]{processor.preview();};stop.onClick=[this]{processor.stop();};
  loop.onClick=[this]{processor.setLoop(loop.getToggleState());};
  reset.onClick=[this]{xc::resetNotes(processor.settings);refresh(true);};
  random.onClick=[this]{auto& s=processor.settings;auto& rng=Random::getSystemRandom();if(!lockRoot.getToggleState())s.root=rng.nextInt(12);s.formula=rng.nextInt(static_cast<int>(xc::catalog(s.scale!=0).size()));s.inversion=s.scale?0:rng.nextInt(static_cast<int>(xc::catalog(false)[static_cast<size_t>(s.formula)].intervals.size()));xc::resetNotes(s);refresh(true);};
  undo.onClick=[this]{travelHistory(false);};redo.onClick=[this]{travelHistory(true);};
  timbre.onChange=[this]{processor.settings.timbre=timbre.getSelectedId()-1;processor.setSoundSettings();};
  sync.onClick=[this]{processor.settings.syncTempo=sync.getToggleState();processor.setSoundSettings();};
  libraryOpen.onClick=[this]{library.language(processor.settings.language==1);library.setVisible(!library.isVisible());library.toFront(true);};library.close=[this]{library.setVisible(false);};
  library.capture=[this]{return processor.sceneXml();};
  library.restore=[this](const XmlElement& xml){remember();int lang=processor.settings.language,oct=processor.settings.convention,w=getWidth(),h=getHeight();processor.restoreXml(xml);processor.settings.language=lang;processor.settings.convention=oct;processor.editorWidth=w;processor.editorHeight=h;refresh();if(!processor.progression.empty()){progressionPanel.setVisible(true);progressionPanel.toFront(false);}library.setVisible(false);updateExportButtons();};
  progressionOpen.onClick=[this]{progressionPanel.setVisible(!progressionPanel.isVisible());progressionPanel.refresh();progressionPanel.toFront(false);if(library.isVisible())library.toFront(true);updateExportButtons();};progressionPanel.onClose=[this]{progressionPanel.setVisible(false);updateExportButtons();};
  progressionPanel.beforeEdit=[this]{remember();};progressionPanel.onChanged=[this]{updateExportButtons();if(processor.settings.nearVoices)xc::leadVoices(processor.progression);if(processor.isPreviewing() && processor.previewIsProgression)processor.preview(true);updateHistoryButtons();updateExportButtons();};
  progressionPanel.onAdd=[this]{auto pitches=xc::selectedNotes(processor.settings);if(pitches.empty() || processor.progression.size()>=128)return;remember();processor.progression.push_back(currentStep());if(processor.settings.nearVoices)xc::leadVoices(processor.progression);if(processor.isPreviewing() && processor.previewIsProgression)processor.preview(true);updateHistoryButtons();updateExportButtons();};
  progressionPanel.onReplace=[this](int index){if(xc::selectedNotes(processor.settings).empty())return;remember();auto length=processor.progression[static_cast<size_t>(index)].beats;auto gate=processor.progression[static_cast<size_t>(index)].gate;processor.progression[static_cast<size_t>(index)]=currentStep();processor.progression[static_cast<size_t>(index)].beats=length;processor.progression[static_cast<size_t>(index)].gate=gate;if(processor.settings.nearVoices)xc::leadVoices(processor.progression);if(processor.isPreviewing() && processor.previewIsProgression)processor.preview(true);updateHistoryButtons();updateExportButtons();};
  progressionPanel.onLoad=[this](int index){remember();auto& step=processor.progression[static_cast<size_t>(index)];auto& s=processor.settings;s.custom=true;s.customNotes=step.notes;s.velocity=step.velocity;s.octave=step.octave;s.root=step.root;s.formula=step.formula;s.scale=step.scale;s.inversion=step.inversion;s.instrument=step.instrument;s.bassPattern=step.bassPattern;s.strum=step.strum;s.frets=step.frets;progressionPanel.setVisible(false);refresh(true);};
  progressionPanel.onPlay=[this]{processor.preview(true);};progressionPanel.onExport=[this]{saveFile(true);};
  progressionPanel.onStop=[this]{processor.stop();};progressionPanel.onLoop=[this](bool enabled){remember();processor.setLoop(enabled);loop.setToggleState(enabled,dontSendNotification);};
  save.onClick=[this]{saveFile(progressionPanel.isVisible());};
  drag.drag=[this]{auto f=File::getSpecialLocation(File::tempDirectory).getChildFile("xchords");if(f.createDirectory().failed()){report(false);return;} auto target=f.getNonexistentChildFile("xchords",".mid"); bool whole=progressionPanel.isVisible();if(xc::writeMidi(target,whole?xc::progressionEvents(processor.progression):makeEvents(processor.settings),static_cast<int>(std::lround(processor.settings.syncTempo && processor.hasHostTempo()?processor.effectiveBpm():processor.settings.bpm)),whole?xc::progressionLength(processor.progression):0)) { bool ok=DragAndDropContainer::performExternalDragDropOfFiles({target.getFullPathName()},false,this); report(ok); } else report(false);};
  for(auto* c:{&mode,&root,&formula,&octave,&convention,&inversion,&pattern,&duration,&timbre,&instrument,&voicing,&bassStyle}){auto callback=c->onChange;c->onChange=[this,callback]{remember();if(callback)callback();updateHistoryButtons();};}
  for(auto* b:std::vector<Button*>{&random,&reset,&loop,&sync,&strum,&nearVoices}){auto callback=b->onClick;b->onClick=[this,callback]{remember();if(callback)callback();updateHistoryButtons();};}
  for(auto* slider:{&velocity,&bpm}){auto callback=slider->onValueChange;slider->onValueChange=[this,callback]{remember();if(callback)callback();updateHistoryButtons();};}
  refresh();startTimerHz(30);
 }
 ~Editor() override {stopTimer();setLookAndFeel(nullptr);}
 void paint(Graphics& g) override {
  bool pt=processor.settings.language==1;
  g.setGradientFill(ColourGradient(Colour(0xff17202d),0,0,Colour(0xff0c111a),0,static_cast<float>(getHeight()),false));g.fillAll();
  g.setColour(Colour(0xff70e2bf));g.fillRoundedRectangle(20,24,4,32,2);g.setFont(FontOptions(30.0f,Font::bold));g.drawText("xchords",36,15,180,43,Justification::centredLeft);
  g.setColour(Colour(0xff8799b0));g.setFont(12.0f);g.drawText("VEMCOMPY LABS",220,26,170,23,Justification::centredLeft);
  g.setColour(Colour(0xff2a3545));g.drawHorizontalLine(73,20.0f,static_cast<float>(getWidth()-20));
  auto panel=[&](Rectangle<int> r){g.setColour(Colour(0xff18212e));g.fillRoundedRectangle(r.toFloat(),12);g.setColour(Colour(0xff2a3647));g.drawRoundedRectangle(r.toFloat().reduced(.5f),12,1);};
  panel(harmonyPanel);panel(performancePanel);panel(notesPanel);panel(wheelPanel);panel(keyboardPanel);
  auto title=[&](const String& text,Rectangle<int> r){g.setColour(Colour(0xff8fa5bf));g.setFont(FontOptions(11.0f,Font::bold));g.drawText(text,r.getX()+16,r.getY()+8,r.getWidth()-32,20,Justification::centredLeft);};
  title(tr(pt,"01   HARMONY","01   HARMONIA"),harmonyPanel);title(tr(pt,"02   AUDITION","02   AUDIÇÃO"),performancePanel);
  title(tr(pt,"SELECTED NOTES","NOTAS SELECIONADAS"),notesPanel);title("CAMELOT",wheelPanel);

  const char* en[]={"Mode","Root","Chord / scale","Octave","Middle C","Inversion","Playback","Beats","Velocity","BPM"};
  const char* br[]={"Modo","Fundamental","Acorde / escala","Oitava","Dó central","Inversão","Execução","Tempos","Intensidade","BPM"};
  g.setColour(Colour(0xff9aacc2));g.setFont(12.0f);
  for(int i=0;i<10;++i) {auto r=fields[static_cast<size_t>(i)];g.drawText(tr(pt,en[i],br[i]),r.getX(),r.getY()-20,r.getWidth(),18,Justification::centredLeft);}
  const auto& s=processor.settings;auto code=String(xc::referenceCamelot(s));
  String info=tr(pt,"A = minor · B = major\nClick to choose a key.","A = menor · B = maior\nClique para escolher uma tonalidade.");
  if(s.custom)info+=tr(pt,"\nCustom notes · original: ","\nNotas personalizadas · origem: ")+(code.isEmpty()?String("-"):code);
  else if(code.isNotEmpty())info+=tr(pt,"\nTonal reference: ","\nReferência tonal: ")+code;
  else info+=tr(pt,"\nNo direct Camelot code for this formula.","\nSem código Camelot direto para esta fórmula.");
  g.drawFittedText(info,wheelPanel.getX()+12,wheel.getBottom()+8,wheelPanel.getWidth()-24,56,Justification::centred,3);
  g.setColour(Colour(0xff62758d));g.setFont(11.0f);g.drawText(String(getWidth())+" x "+String(getHeight()),getWidth()-124,getHeight()-24,90,18,Justification::centredRight);
 }
 void resized() override {
  const int w=getWidth(),h=getHeight(),gap=14;
  processor.editorWidth=w;processor.editorHeight=h;
  const int right=jlimit(310,420,w*28/100),left=w-right-54;
  language.setBounds(w-190,25,170,30);
  harmonyPanel={20,88,left,168};performancePanel={20,270,left,128};notesPanel={20,412,left,h-612};
  wheelPanel={34+left,88,right,h-288};keyboardPanel={20,h-186,w-40,154};
  int x=36,available=left-32;
  int modeW=available*22/100,rootW=available*34/100;
  fields[0]={x,140,modeW,32};fields[1]={x+modeW+gap,140,rootW,32};fields[2]={fields[1].getRight()+gap,140,available-modeW-rootW-2*gap,32};
  const std::array<float,5> ratios{.12f,.30f,.18f,.23f,.17f};int remaining=available-4*10;
  for(int i=0;i<5;++i){int width=i==4?36+available-x:static_cast<int>(remaining*ratios[static_cast<size_t>(i)]);fields[static_cast<size_t>(i+3)]={x,207,width,32};x+=width+10;}
  int half=(available-20)/2;fields[8]={36,320,half,24};fields[9]={56+half,320,available-half-20,24};
  std::array<Component*,10> comps{&mode,&root,&formula,&octave,&convention,&inversion,&pattern,&duration,&velocity,&bpm};
  for(size_t i=0;i<comps.size();++i)comps[i]->setBounds(fields[i]);
  play.setBounds(36,350,120,32);stop.setBounds(166,350,80,32);loop.setBounds(258,350,85,32);reset.setBounds(36+available-166,350,166,32);
  random.setBounds(36+available-118,96,118,26);lockRoot.setBounds(36+available-290,96,168,26);
  progressionOpen.setBounds(w-610,26,110,28);libraryOpen.setBounds(w-490,26,100,28);undo.setBounds(w-380,26,86,28);redo.setBounds(w-284,26,86,28);
  timbre.setBounds(160,276,140,25);sync.setBounds(310,276,125,25);tempoLabel.setBounds(440,276,left-430,25);
  progressionPanel.setBounds(20,88,left,h-288);library.setBounds(getLocalBounds().withSizeKeepingCentre(780,440));
  int diameter=jmin(right-32,wheelPanel.getHeight()-150);
  wheel.setBounds(wheelPanel.getCentreX()-diameter/2,126,diameter,diameter);
  int buttonWidth=(right-42)/2;save.setBounds(wheelPanel.getX()+16,wheelPanel.getBottom()-46,buttonWidth,30);drag.setBounds(save.getRight()+10,save.getY(),buttonWidth,30);
  notes.setBounds(notesPanel.reduced(16).withTrimmedTop(20));
  instrument.setBounds(36,h-181,180,25);voicing.setBounds(228,h-181,160,25);bassStyle.setBounds(228,h-181,230,25);strum.setBounds(400,h-181,100,25);nearVoices.setBounds(w-300,h-181,264,25);fretboard.setBounds(36,h-154,w-72,110);
  keyboard.setBounds(36,h-154,w-72,110);status.setBounds(20,h-27,w-160,22);
 }
private:
 Processor& processor;
 std::vector<MemoryBlock> undoStates,redoStates;
 XChordsTheme theme;
 TooltipWindow tooltips{this,500};
 Rectangle<int> harmonyPanel,performancePanel,notesPanel,wheelPanel,keyboardPanel;
 ComboBox language,mode,root,formula,octave,convention,inversion,pattern,duration,timbre,instrument,voicing,bassStyle;
 Slider velocity,bpm; TextButton play,stop,random,undo,redo,save,reset,libraryOpen,progressionOpen;DragButton drag;ToggleButton lockRoot,loop,sync,strum,nearVoices;Fretboard fretboard;Keyboard keyboard;CamelotWheel wheel;TextEditor notes;Label status,tempoLabel;
 LibraryPanel library;ProgressionPanel progressionPanel;
 std::array<Rectangle<int>,10> fields;std::unique_ptr<FileChooser> chooser;
 void fill(ComboBox& c,const StringArray& values,int selected) {c.clear(dontSendNotification);c.addItemList(values,1);c.setSelectedId(selected+1,dontSendNotification);}
 void refresh(bool audition=false) {
  auto& s=processor.settings;bool pt=s.language==1;
  language.setSelectedId(s.language+1,dontSendNotification);
  fill(instrument,{tr(pt,"Keyboard","Teclado"),tr(pt,"Guitar / acoustic","Guitarra / violão"),tr(pt,"Bass · 4 strings","Baixo · 4 cordas"),tr(pt,"Bass · 5 strings","Baixo · 5 cordas")},s.instrument);fill(bassStyle,{tr(pt,"Root","Fundamental"),tr(pt,"Root / fifth / octave","Fundamental / quinta / oitava"),tr(pt,"Selected tones","Notas selecionadas")},s.bassPattern);strum.setButtonText("Strum");strum.setToggleState(s.strum,dontSendNotification);nearVoices.setButtonText(tr(pt,"Close voices · progression","Aproximar vozes · progressão"));nearVoices.setToggleState(s.nearVoices,dontSendNotification);StringArray positions;if(s.instrument==1 && !s.scale){auto shapes=xc::guitarShapes(s.root,xc::catalog(false)[static_cast<size_t>(s.formula)],s.inversion);for(size_t i=0;i<shapes.size();++i)positions.add(tr(pt,"Position ","Posição ")+String(static_cast<int>(i+1)));}fill(voicing,positions,s.voicing);
  fill(mode,{tr(pt,"Chords","Acordes"),tr(pt,"Scales","Escalas")},s.scale);
  StringArray roots;for(int n=0;n<12;++n) {auto name=String::fromUTF8(xc::noteNames(60+n).c_str()); name=name.upToFirstOccurrenceOf(String::fromUTF8(" · MIDI"),false,false).removeCharacters("4");roots.add(name);}fill(root,roots,s.root);
  StringArray formulas;for(const auto& f:xc::catalog(s.scale!=0))formulas.add(String::fromUTF8((pt?f.pt:f.en).c_str()));fill(formula,formulas,s.formula);
  StringArray octaves;for(int i=2;i<=6;++i)octaves.add(String(i-5+s.convention));fill(octave,octaves,s.octave-2);
  fill(convention,{String::fromUTF8("MIDI 60 = C3 / Dó3"),String::fromUTF8("MIDI 60 = C4 / Dó4")},s.convention-3);
  StringArray inv;auto count=s.scale?1:static_cast<int>(xc::catalog(false)[static_cast<size_t>(s.formula)].intervals.size());for(int i=0;i<count;++i)inv.add(i==0?tr(pt,"Root position","Fundamental"):String(i));s.inversion=jmin(s.inversion,count-1);fill(inversion,inv,s.inversion);inversion.setEnabled(!s.scale);
  fill(pattern,{tr(pt,"Block","Em bloco"),tr(pt,"Ascending","Ascendente"),tr(pt,"Descending","Descendente"),tr(pt,"Up / down","Ida e volta")},s.pattern);
  fill(duration,{"1/4","1/2","1","2"},s.duration);
  velocity.setValue(s.velocity,dontSendNotification);bpm.setValue(s.bpm,dontSendNotification);
  play.setButtonText(tr(pt,"Play","Ouvir"));stop.setButtonText(tr(pt,"Stop","Parar"));random.setButtonText(tr(pt,"Random","Sortear"));undo.setButtonText(tr(pt,"Undo","Desfazer"));redo.setButtonText(tr(pt,"Redo","Refazer"));updateHistoryButtons();
  libraryOpen.setButtonText("Presets");progressionOpen.setButtonText(tr(pt,"Progression","Progressão"));
  fill(timbre,{tr(pt,"Soft keys","Teclas suaves"),tr(pt,"Warm pad","Pad suave"),tr(pt,"Sine","Senoidal")},s.timbre);sync.setButtonText(tr(pt,"DAW BPM","BPM da DAW"));sync.setToggleState(s.syncTempo,dontSendNotification);processor.setSoundSettings();progressionPanel.language(pt);progressionPanel.setLoop(s.loop);library.language(pt);
  save.setButtonText(tr(pt,"Save MIDI…","Salvar MIDI…"));drag.setButtonText(tr(pt,"Drag MIDI","Arrastar MIDI"));lockRoot.setButtonText(tr(pt,"Keep root","Manter fundamental"));
  loop.setButtonText("Loop");loop.setToggleState(s.loop,dontSendNotification);processor.setLoop(s.loop);reset.setButtonText(tr(pt,"Restore original","Restaurar original"));update(audition);
 }
 void update(bool audition=false) {
  auto& s=processor.settings;StringArray lines;keyboard.notes=xc::selectedNotes(s);auto displayNotes=keyboard.notes;if(s.instrument>=2){displayNotes.clear();for(auto e:makeEvents(s))displayNotes.push_back(e.note);displayNotes=xc::uniqueNotes(displayNotes);}for(auto n:displayNotes)lines.add(String::fromUTF8(xc::noteNames(n,s.convention).c_str()));
  bool empty=keyboard.notes.empty();play.setEnabled(!empty);save.setEnabled(!empty);drag.setEnabled(!empty);reset.setEnabled(s.custom);inversion.setEnabled(!s.scale && !s.custom && s.instrument<2);octave.setEnabled(s.instrument==0);pattern.setEnabled(s.instrument<2 && !(s.instrument==1 && s.strum));
  keyboard.setVisible(s.instrument==0);fretboard.setVisible(s.instrument!=0);voicing.setVisible(s.instrument==1 && !s.scale);voicing.setEnabled(!s.custom && voicing.getNumItems()>0);strum.setVisible(s.instrument==1 && !s.scale);bassStyle.setVisible(s.instrument>=2);fretboard.updateSettings(s);
  notes.setText(empty?tr(s.language==1,"No notes selected. Click the keyboard.","Nenhuma nota selecionada. Clique no teclado."):lines.joinIntoString("     |     "),false);
  keyboard.start=s.octave*12;keyboard.convention=s.convention;keyboard.repaint();
  wheel.select(s.custom?String{}:String(xc::referenceCamelot(s)));
  status.setText(s.custom?tr(s.language==1,"Custom · preview and MIDI use exactly the selected notes","Personalizada · prévia e MIDI usam exatamente as notas selecionadas"):tr(s.language==1,"Original selection · ready to edit","Seleção original · pronta para editar"),dontSendNotification);
  if(s.instrument>=2)status.setText(tr(s.language==1,"Bass line · preview and MIDI use the selected pattern","Linha de baixo · prévia e MIDI usam o padrão selecionado"),dontSendNotification);else if(s.instrument==1 && empty)status.setText(tr(s.language==1,"No suggested shape. Click the fretboard to build one.","Sem posição sugerida. Clique no braço para montar uma."),dontSendNotification);
  if(audition && processor.isPreviewing())processor.preview(progressionPanel.isVisible() && processor.previewIsProgression);updateHistoryButtons();updateExportButtons();repaint();
 }
 void updateExportButtons(){bool enabled=progressionPanel.isVisible()?!xc::progressionEvents(processor.progression).empty():!xc::selectedNotes(processor.settings).empty();save.setEnabled(enabled);drag.setEnabled(enabled);}
 void report(bool ok) {bool pt=processor.settings.language==1;status.setText(ok?tr(pt,"MIDI ready","MIDI pronto"):tr(pt,"Could not create or drag MIDI","Não foi possível criar ou arrastar MIDI"),dontSendNotification);}
 void saveFile(bool wholeProgression=false) {
  const auto snapshot=processor.settings;
  const auto events=wholeProgression?xc::progressionEvents(processor.progression):makeEvents(snapshot);
  const double length=wholeProgression?xc::progressionLength(processor.progression):0;
  const int tempo=static_cast<int>(std::lround(snapshot.syncTempo && processor.hasHostTempo()?processor.effectiveBpm():snapshot.bpm));
  chooser=std::make_unique<FileChooser>(tr(snapshot.language==1,"Save MIDI","Salvar MIDI"),File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(wholeProgression?"xchords-progression.mid":"xchords.mid"),"*.mid");
  Component::SafePointer<Editor> safe(this);
  chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[safe,events,tempo,length](const FileChooser& fc){if(safe && fc.getResult()!=File{})safe->report(xc::writeMidi(fc.getResult().withFileExtension("mid"),events,tempo,length));});
 }
 void remember(){MemoryBlock state;processor.getStateInformation(state);if(undoStates.empty() || undoStates.back()!=state){undoStates.push_back(std::move(state));if(undoStates.size()>128)undoStates.erase(undoStates.begin());}redoStates.clear();updateHistoryButtons();}
 void updateHistoryButtons(){undo.setEnabled(!undoStates.empty());redo.setEnabled(!redoStates.empty());}
 void travelHistory(bool forward){auto& from=forward?redoStates:undoStates;auto& to=forward?undoStates:redoStates;if(from.empty())return;bool wasPlaying=processor.isPreviewing(),whole=processor.previewIsProgression;MemoryBlock current;processor.getStateInformation(current);to.push_back(std::move(current));auto state=std::move(from.back());from.pop_back();int width=getWidth(),height=getHeight();processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));processor.editorWidth=width;processor.editorHeight=height;refresh();if(wasPlaying)processor.preview(whole);}
 xc::ChordStep currentStep()const {auto& s=processor.settings;xc::ChordStep step;step.notes=xc::selectedNotes(s);step.velocity=s.velocity;step.root=s.root;step.formula=s.formula;step.scale=s.scale;step.octave=s.octave;step.inversion=s.inversion;step.instrument=s.instrument;step.bassPattern=s.bassPattern;step.strum=s.strum;if(s.instrument==1)step.frets=xc::selectedFrets(s);String label=root.getText()+" "+formula.getText();if(s.custom)label+=tr(s.language==1," (custom)"," (personalizado)");if(s.instrument!=0)label+=" · "+instrument.getText();step.name=label.toStdString();return step;}
 void timerCallback() override {
  bool changed=false;for(int n=0;n<128;++n){bool playing=processor.noteIsPlaying(n);if(keyboard.sounding[static_cast<size_t>(n)]!=playing){keyboard.sounding[static_cast<size_t>(n)]=playing;changed=true;}}if(changed){keyboard.repaint();fretboard.sounding=keyboard.sounding;fretboard.repaint();}
  const auto& s=processor.settings;String label;if(s.syncTempo)label=processor.hasHostTempo()?"DAW: "+String(processor.effectiveBpm(),1):tr(s.language==1,"Manual (no host)","Manual (sem host)");tempoLabel.setText(label,dontSendNotification);bpm.setEnabled(!s.syncTempo || !processor.hasHostTempo());
 }
};
AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
