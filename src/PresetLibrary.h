#pragma once
#include <juce_core/juce_core.h>
// One atomic XML file per preset. Metadata wraps the existing versioned session
// state, so the same restore path handles custom notes and whole progressions.
class PresetLibrary {
public:
 struct Entry {juce::File file;juce::String name;bool favourite=false;};
 explicit PresetLibrary(juce::File directory):folder(std::move(directory)){}
 std::vector<Entry> list(bool favouritesOnly=false) const {
  std::vector<Entry> entries;
  for(const auto& f:folder.findChildFiles(juce::File::findFiles,false,"*.xchords")) {
   auto xml=juce::XmlDocument::parse(f);if(!xml || !xml->hasTagName("XChordsPreset") || !xml->getChildByName("XChords"))continue;
   bool favourite=xml->getBoolAttribute("favourite");if(!favouritesOnly || favourite)entries.push_back({f,xml->getStringAttribute("name",f.getFileNameWithoutExtension()),favourite});
  }
  std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.name.compareIgnoreCase(b.name)<0;});return entries;
 }
 juce::File save(const juce::String& name,const juce::XmlElement& state) const {
  if(name.trim().isEmpty() || folder.createDirectory().failed() || !state.hasTagName("XChords"))return {};
  juce::XmlElement xml("XChordsPreset");xml.setAttribute("version",1);xml.setAttribute("name",name.trim().substring(0,100));xml.setAttribute("favourite",false);xml.addChildElement(new juce::XmlElement(state));
  auto file=folder.getChildFile(juce::Uuid().toString()+".xchords");return write(file,xml)?file:juce::File{};
 }
 std::unique_ptr<juce::XmlElement> load(const juce::File& file) const {
  if(file.getParentDirectory()!=folder)return {};
  auto xml=juce::XmlDocument::parse(file);if(!xml || !xml->hasTagName("XChordsPreset"))return {};
  if(auto* state=xml->getChildByName("XChords"))return std::make_unique<juce::XmlElement>(*state);return {};
 }
 bool favourite(const juce::File& file,bool value) const {
  if(file.getParentDirectory()!=folder)return false;
  auto xml=juce::XmlDocument::parse(file);if(!xml || !xml->hasTagName("XChordsPreset"))return false;xml->setAttribute("favourite",value);return write(file,*xml);
 }
private:
 juce::File folder;
 static bool write(const juce::File& file,const juce::XmlElement& xml) {
  juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(xml.toString()) && tmp.overwriteTargetFileWithTemporary();
 }
};
