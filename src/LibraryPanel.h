#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetLibrary.h"
class LibraryPanel final : public juce::Component {
public:
 std::function<std::unique_ptr<juce::XmlElement>()> capture;
 std::function<void(const juce::XmlElement&)> restore;
 std::function<void()> close;
 explicit LibraryPanel(juce::File directory=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Vemcompy Labs/Xchords/Presets")):store(std::move(directory)) {
  for(auto* c:std::vector<juce::Component*>{&list,&name,&save,&load,&star,&filter,&dismiss,&status})addAndMakeVisible(c);
  name.setComponentID("preset-name");save.setComponentID("preset-save");list.setComponentID("preset-list");load.setComponentID("preset-load");star.setComponentID("preset-favourite");dismiss.setComponentID("preset-close");
  save.onClick=[this]{if(!capture)return;auto state=capture();auto file=state?store.save(name.getText(),*state):juce::File{};message(file!=juce::File{});if(file!=juce::File{}){filter.setToggleState(false,juce::dontSendNotification);refresh(file);}};
  load.onClick=[this]{int i=list.getSelectedId()-1;if(i<0 || i>=static_cast<int>(entries.size()))return;auto state=store.load(entries[static_cast<size_t>(i)].file);if(state && restore)restore(*state);message(state!=nullptr);};
  star.onClick=[this]{int i=list.getSelectedId()-1;if(i<0 || i>=static_cast<int>(entries.size()))return;auto e=entries[static_cast<size_t>(i)];message(store.favourite(e.file,!e.favourite));refresh(e.file);};
  filter.onClick=[this]{refresh();};dismiss.onClick=[this]{if(close)close();};list.onChange=[this]{updateButtons();};language(true);
 }
 void language(bool value){pt=value;save.setButtonText(t("Save new","Salvar novo"));load.setButtonText(t("Load","Carregar"));dismiss.setButtonText(t("Close","Fechar"));filter.setButtonText(t("Favourites only","Somente favoritos"));name.setTextToShowWhenEmpty(t("Name this preset…","Nome do preset…"),juce::Colours::grey);refresh();}
 void paint(juce::Graphics& g) override {
  g.setColour(juce::Colour(0xff202c3c));g.fillRoundedRectangle(getLocalBounds().toFloat(),12);g.setColour(juce::Colour(0xff70e2bf));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),12,2);g.setFont(22.0f);g.drawText(t("Preset library","Biblioteca de presets"),22,16,390,30,juce::Justification::centredLeft);
  g.setFont(13.0f);g.setColour(juce::Colour(0xffaabbd0));g.drawFittedText(t("Saves your notes, sound and full progression locally.","Salva suas notas, timbre e progressão completa neste computador."),22,52,getWidth()-44,35,juce::Justification::centredLeft,2);
 }
 void resized() override {
  name.setBounds(22,96,getWidth()-166,32);save.setBounds(getWidth()-132,96,110,32);
  list.setBounds(22,150,getWidth()-44,34);filter.setBounds(22,192,240,28);
  load.setBounds(22,238,120,32);star.setBounds(154,238,200,32);dismiss.setBounds(getWidth()-132,238,110,32);status.setBounds(22,282,getWidth()-44,40);
 }
private:
 PresetLibrary store;std::vector<PresetLibrary::Entry> entries;bool pt=true;
 juce::ComboBox list;juce::TextEditor name;juce::TextButton save,load,star,dismiss;juce::ToggleButton filter;juce::Label status;
 juce::String t(const char* en,const char* br)const{return juce::String::fromUTF8(pt?br:en);}
 void message(bool ok){status.setText(ok?t("Done.","Pronto."):t("Could not save/load. Check the name and folder access.","Não foi possível salvar/carregar. Confira o nome e o acesso à pasta."),juce::dontSendNotification);}
 void refresh(const juce::File& selected={}){entries=store.list(filter.getToggleState());list.clear(juce::dontSendNotification);for(size_t i=0;i<entries.size();++i){auto& e=entries[i];list.addItem((e.favourite?"* ":"")+e.name,static_cast<int>(i)+1);if(e.file==selected)list.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);}if(list.getSelectedId()==0 && !entries.empty())list.setSelectedId(1,juce::dontSendNotification);updateButtons();}
 void updateButtons(){int i=list.getSelectedId()-1;bool selected=i>=0 && i<static_cast<int>(entries.size());load.setEnabled(selected);star.setEnabled(selected);star.setButtonText(selected && entries[static_cast<size_t>(i)].favourite?t("Remove favourite","Remover favorito"):t("Mark favourite","Marcar favorito"));}
};
