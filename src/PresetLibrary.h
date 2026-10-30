#pragma once
#include <juce_core/juce_core.h>
#include "FactoryData.h"
// One atomic XML file per preset. Metadata wraps the existing versioned session
// state, so the same restore path handles custom notes and whole progressions.
class PresetLibrary {
public:
 struct Entry {juce::File file;juce::String name;bool favourite=false;juce::String instrument,role,genre,description;bool factory=false;};
 explicit PresetLibrary(juce::File directory,bool includeFactory=false):folder(std::move(directory)),withFactory(includeFactory){}
 static const juce::XmlElement* factoryBank(){static auto xml=juce::XmlDocument::parse(juce::String::fromUTF8(FactoryData::factorypresets_xml,FactoryData::factorypresets_xmlSize));return xml.get();}
 std::vector<Entry> list(bool favouritesOnly=false) const {
  std::vector<Entry> entries;
  if(withFactory)if(auto* bank=factoryBank()){auto favourites=juce::XmlDocument::parse(folder.getChildFile("factory-favourites.xml"));for(auto* preset:bank->getChildIterator()){auto id=preset->getStringAttribute("id");bool favourite=favourites && favourites->getBoolAttribute("p"+id);if(!favouritesOnly || favourite)entries.push_back({folder.getChildFile("factory-"+id+".xchords"),preset->getStringAttribute("name"),favourite,preset->getStringAttribute("instrument"),preset->getStringAttribute("role"),preset->getStringAttribute("genre"),preset->getStringAttribute("description"),true});}}
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
  if(auto* preset=factoryPreset(file))if(auto* state=preset->getChildByName("XChords"))return std::make_unique<juce::XmlElement>(*state);
  auto xml=juce::XmlDocument::parse(file);if(!xml || !xml->hasTagName("XChordsPreset"))return {};
  if(auto* state=xml->getChildByName("XChords"))return std::make_unique<juce::XmlElement>(*state);return {};
 }
 bool favourite(const juce::File& file,bool value) const {
  if(file.getParentDirectory()!=folder)return false;
  if(auto* preset=factoryPreset(file)){if(folder.createDirectory().failed())return false;auto target=folder.getChildFile("factory-favourites.xml");auto xml=juce::XmlDocument::parse(target);if(!xml || !xml->hasTagName("FactoryFavourites"))xml=std::make_unique<juce::XmlElement>("FactoryFavourites");xml->setAttribute("p"+preset->getStringAttribute("id"),value);return write(target,*xml);}
  auto xml=juce::XmlDocument::parse(file);if(!xml || !xml->hasTagName("XChordsPreset"))return false;xml->setAttribute("favourite",value);return write(file,*xml);
 }
private:
 juce::File folder;bool withFactory=false;
 const juce::XmlElement* factoryPreset(const juce::File& file)const{if(!withFactory || !file.getFileName().startsWith("factory-"))return nullptr;auto id=file.getFileNameWithoutExtension().substring(8);if(auto* bank=factoryBank())for(auto* preset:bank->getChildIterator())if(preset->getStringAttribute("id")==id)return preset;return nullptr;}
 static bool write(const juce::File& file,const juce::XmlElement& xml) {
  juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(xml.toString()) && tmp.overwriteTargetFileWithTemporary();
 }
};
