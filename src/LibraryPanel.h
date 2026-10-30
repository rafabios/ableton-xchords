#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetLibrary.h"
class LibraryPanel final : public juce::Component {
public:
 std::function<std::unique_ptr<juce::XmlElement>()> capture;
 std::function<void(const juce::XmlElement&)> restore;
 std::function<void()> close;
 static juce::File defaultFolder(){return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Vemcompy Labs/Xchords/Presets");}
 explicit LibraryPanel(juce::File directory=defaultFolder(),bool includeFactory=false):store(directory,includeFactory || directory==defaultFolder()) {
  for(auto* c:std::vector<juce::Component*>{&list,&name,&save,&load,&star,&filter,&dismiss,&status,&search,&instrument,&role,&genre,&details,&count})addAndMakeVisible(c);
  name.setComponentID("preset-name");save.setComponentID("preset-save");list.setComponentID("preset-list");load.setComponentID("preset-load");star.setComponentID("preset-favourite");dismiss.setComponentID("preset-close");search.setComponentID("preset-search");instrument.setComponentID("preset-instrument");role.setComponentID("preset-role");genre.setComponentID("preset-genre");count.setComponentID("preset-count");
  save.onClick=[this]{if(!capture)return;auto state=capture();auto file=state?store.save(name.getText(),*state):juce::File{};message(file!=juce::File{});if(file!=juce::File{}){filter.setToggleState(false,juce::dontSendNotification);search.clear();for(auto* c:{&instrument,&role,&genre})c->setSelectedId(1,juce::dontSendNotification);refresh(file);}};
  load.onClick=[this]{int i=list.getSelectedId()-1;if(i<0 || i>=static_cast<int>(entries.size()))return;auto state=store.load(entries[static_cast<size_t>(i)].file);if(state && restore)restore(*state);message(state!=nullptr);};
  star.onClick=[this]{int i=list.getSelectedId()-1;if(i<0 || i>=static_cast<int>(entries.size()))return;auto e=entries[static_cast<size_t>(i)];message(store.favourite(e.file,!e.favourite));refresh(e.file);};
  filter.onClick=[this]{refresh();};dismiss.onClick=[this]{if(close)close();};list.onChange=[this]{updateButtons();};
  search.onTextChange=[this]{applyFilters();};for(auto* c:{&instrument,&role,&genre})c->onChange=[this]{applyFilters();};language(true);
 }
 void language(bool value){pt=value;save.setButtonText(t("Save new","Salvar novo"));load.setButtonText(t("Load sequence","Carregar sequência"));dismiss.setButtonText(t("Close","Fechar"));filter.setButtonText(t("Favourites only","Somente favoritos"));name.setTextToShowWhenEmpty(t("Name your own preset…","Nome do seu preset…"),juce::Colours::grey);search.setTextToShowWhenEmpty(t("Search: psy bass, forest, house…","Buscar: psy bass, forest, house…"),juce::Colours::grey);refresh();}
 void paint(juce::Graphics& g) override {
  g.setColour(juce::Colour(0xff202c3c));g.fillRoundedRectangle(getLocalBounds().toFloat(),12);g.setColour(juce::Colour(0xff70e2bf));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),12,2);g.setFont(22.0f);g.drawText(t("Preset library","Biblioteca de presets"),22,16,390,30,juce::Justification::centredLeft);
  g.setFont(13.0f);g.setColour(juce::Colour(0xffaabbd0));g.drawFittedText(t("INSTRUMENT - ROLE - GENRE - IDEA · MIDI sequences, not synth patches.","INSTRUMENTO - FUNÇÃO - GÊNERO - IDEIA · Sequências MIDI, não timbres."),22,52,getWidth()-44,32,juce::Justification::centredLeft,2);
 }
 void resized() override {
  const int w=getWidth(),col=(w-64)/3;
  search.setBounds(22,94,w-272,30);filter.setBounds(w-240,94,218,30);
  instrument.setBounds(22,136,col,30);role.setBounds(32+col,136,col,30);genre.setBounds(42+col*2,136,col,30);
  count.setBounds(22,174,w-44,22);list.setBounds(22,202,w-44,34);details.setBounds(22,244,w-44,26);
  load.setBounds(22,282,160,32);star.setBounds(194,282,200,32);dismiss.setBounds(w-132,282,110,32);
  name.setBounds(22,336,w-166,32);save.setBounds(w-132,336,110,32);status.setBounds(22,380,w-44,40);
 }
private:
 PresetLibrary store;std::vector<PresetLibrary::Entry> all,entries;bool pt=true;
 juce::ComboBox list,instrument,role,genre;juce::TextEditor name,search;juce::TextButton save,load,star,dismiss;juce::ToggleButton filter;juce::Label status,details,count;
 juce::String t(const char* en,const char* br)const{return juce::String::fromUTF8(pt?br:en);}
 void message(bool ok){status.setText(ok?t("Done.","Pronto."):t("Could not save/load. Check the name and folder access.","Não foi possível salvar/carregar. Confira o nome e o acesso à pasta."),juce::dontSendNotification);}
 void populate(juce::ComboBox& combo,juce::StringArray values,const juce::String& title){auto selected=combo.getSelectedId()>1?combo.getText():juce::String{};values.removeEmptyStrings();values.removeDuplicates(false);values.sort(true);combo.clear(juce::dontSendNotification);combo.addItem(title,1);combo.addItemList(values,2);combo.setSelectedId(selected.isEmpty()?1:values.indexOf(selected)+2,juce::dontSendNotification);if(combo.getSelectedId()==0)combo.setSelectedId(1,juce::dontSendNotification);}
 void refresh(const juce::File& selected={}){all=store.list();juce::StringArray instruments,roles,genres;for(auto& e:all){instruments.add(e.instrument);roles.add(e.role);genres.add(e.genre);}populate(instrument,instruments,t("All instruments","Todos os instrumentos"));populate(role,roles,t("All roles","Todas as funções"));populate(genre,genres,t("All genres","Todos os gêneros"));applyFilters(selected);}
 void applyFilters(const juce::File& selected={}){auto previous=list.getSelectedId()>0 && list.getSelectedId()<=static_cast<int>(entries.size())?entries[static_cast<size_t>(list.getSelectedId()-1)].file:juce::File{};auto desired=selected==juce::File{}?previous:selected;entries.clear();auto tokens=juce::StringArray::fromTokens(search.getText()," \t","");list.clear(juce::dontSendNotification);for(auto& e:all){if(filter.getToggleState() && !e.favourite)continue;if(instrument.getSelectedId()>1 && instrument.getText()!=e.instrument)continue;if(role.getSelectedId()>1 && role.getText()!=e.role)continue;if(genre.getSelectedId()>1 && genre.getText()!=e.genre)continue;bool matches=true;for(auto token:tokens)if(!e.name.containsIgnoreCase(token))matches=false;if(!matches)continue;entries.push_back(e);int id=static_cast<int>(entries.size());list.addItem((e.favourite?"* ":"")+e.name,id);if(e.file==desired)list.setSelectedId(id,juce::dontSendNotification);}if(list.getSelectedId()==0 && !entries.empty())list.setSelectedId(1,juce::dontSendNotification);count.setText(juce::String(static_cast<int>(entries.size()))+t(" presets shown / "," presets encontrados / ")+juce::String(static_cast<int>(all.size()))+t(" available"," disponíveis"),juce::dontSendNotification);updateButtons();}
 void updateButtons(){int i=list.getSelectedId()-1;bool selected=i>=0 && i<static_cast<int>(entries.size());load.setEnabled(selected);star.setEnabled(selected);star.setButtonText(selected && entries[static_cast<size_t>(i)].favourite?t("Remove favourite","Remover favorito"):t("Mark favourite","Marcar favorito"));details.setText(selected?(entries[static_cast<size_t>(i)].factory?entries[static_cast<size_t>(i)].description:t("User preset","Preset do usuário")):t("No matching presets","Nenhum preset encontrado"),juce::dontSendNotification);}
};
