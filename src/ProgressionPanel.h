#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MusicalState.h"
class ProgressionPanel final : public juce::Component, private juce::ListBoxModel {
public:
 std::function<void()> beforeEdit,onChanged,onAdd,onPlay,onStop,onExport,onClose;
 std::function<void(bool)> onLoop;
 std::function<void(int)> onLoad,onReplace;
 explicit ProgressionPanel(std::vector<xc::ChordStep>& data):steps(data),list("Progression",this) {
  for(auto* c:std::vector<juce::Component*>{&list,&add,&load,&replace,&remove,&up,&down,&beats,&play,&save,&close,&stop,&loop,&gate})addAndMakeVisible(c);
  add.setComponentID("step-add");load.setComponentID("step-load");replace.setComponentID("step-replace");remove.setComponentID("step-remove");up.setComponentID("step-up");down.setComponentID("step-down");beats.setComponentID("step-beats");play.setComponentID("progression-play");save.setComponentID("progression-export");list.setComponentID("progression-list");close.setComponentID("progression-close");
  list.setRowHeight(38);list.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xff111a26));
  gate.setComponentID("step-gate");gate.setRange(5,100,1);gate.setSliderStyle(juce::Slider::LinearHorizontal);gate.setTextBoxStyle(juce::Slider::TextBoxLeft,false,48,24);gate.setTextValueSuffix("%");gate.onValueChange=[this]{if(!valid())return;remember();steps[static_cast<size_t>(list.getSelectedRow())].gate=gate.getValue()/100;changed();};
  beats.setRange(.25,32,0);beats.setNumDecimalPlacesToDisplay(2);beats.setSliderStyle(juce::Slider::IncDecButtons);beats.setTextBoxStyle(juce::Slider::TextBoxLeft,false,65,28);
  add.onClick=[this]{if(onAdd)onAdd();refresh(static_cast<int>(steps.size())-1);};
  load.onClick=[this]{if(valid() && onLoad)onLoad(list.getSelectedRow());};replace.onClick=[this]{if(valid() && onReplace)onReplace(list.getSelectedRow());refresh(list.getSelectedRow());};
  remove.onClick=[this]{if(!valid())return;int i=list.getSelectedRow();remember();steps.erase(steps.begin()+i);changed();refresh(juce::jmin(i,static_cast<int>(steps.size())-1));};
  up.onClick=[this]{move(-1);};down.onClick=[this]{move(1);};
  beats.onValueChange=[this]{if(!valid())return;remember();steps[static_cast<size_t>(list.getSelectedRow())].beats=beats.getValue();changed();list.repaint();};
  play.onClick=[this]{if(onPlay)onPlay();};stop.onClick=[this]{if(onStop)onStop();};loop.onClick=[this]{if(onLoop)onLoop(loop.getToggleState());};loop.setButtonText("Loop");save.onClick=[this]{if(onExport)onExport();};close.onClick=[this]{if(onClose)onClose();};language(true);
 }
 void setLoop(bool value){loop.setToggleState(value,juce::dontSendNotification);}
 void language(bool value){pt=value;add.setButtonText(t("+ Add selection","+ Adicionar seleção"));load.setButtonText(t("Edit notes","Editar notas"));replace.setButtonText(t("Replace","Substituir"));remove.setButtonText(t("Remove","Remover"));up.setButtonText(t("Earlier","Antes"));down.setButtonText(t("Later","Depois"));play.setButtonText(t("Play sequence","Ouvir sequência"));stop.setButtonText(t("Stop","Parar"));save.setButtonText(t("Export MIDI","Exportar MIDI"));close.setButtonText(t("Back","Voltar"));refresh(list.getSelectedRow());}
 void refresh(int selected=-1){if(selected<0)selected=list.getSelectedRow();list.updateContent();if(selected>=0 && selected<static_cast<int>(steps.size()))list.selectRow(selected);else if(!steps.empty())list.selectRow(0);else list.deselectAllRows();selection();repaint();}
 void paint(juce::Graphics& g) override {
  g.setColour(juce::Colour(0xff18212e));g.fillRoundedRectangle(getLocalBounds().toFloat(),12);g.setColour(juce::Colour(0xff8fa5bf));g.setFont(14.0f);g.drawText(t("PROGRESSION","PROGRESSÃO"),16,10,getWidth()-330,24,juce::Justification::centredLeft);
  g.setFont(12.0f);g.drawText("Gate",170,getHeight()-84,48,20,juce::Justification::centredLeft);g.drawText(t("Step length","Duração do passo"),16,getHeight()-84,160,20,juce::Justification::centredLeft);
 }
 void resized() override {
  loop.setBounds(getWidth()-300,10,80,26);stop.setBounds(getWidth()-210,10,100,26);close.setBounds(getWidth()-100,10,84,26);int col=(getWidth()-62)/4;
  add.setBounds(16,48,col,30);load.setBounds(26+col,48,col,30);replace.setBounds(36+2*col,48,col,30);remove.setBounds(46+3*col,48,col,30);
  list.setBounds(16,90,getWidth()-32,getHeight()-190);
  gate.setBounds(220,getHeight()-86,112,24);beats.setBounds(16,getHeight()-58,140,30);up.setBounds(170,getHeight()-58,76,30);down.setBounds(256,getHeight()-58,76,30);
  play.setBounds(getWidth()-296,getHeight()-58,135,30);save.setBounds(getWidth()-151,getHeight()-58,135,30);
 }
private:
 std::vector<xc::ChordStep>& steps;bool pt=true;juce::ListBox list;juce::TextButton add,load,replace,remove,up,down,play,stop,save,close;juce::ToggleButton loop;juce::Slider beats,gate;
 juce::String t(const char* en,const char* br)const{return juce::String::fromUTF8(pt?br:en);}
 int getNumRows()override{return static_cast<int>(steps.size());}
 void paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected)override {
  if(row<0 || row>=static_cast<int>(steps.size()))return;auto& step=steps[static_cast<size_t>(row)];if(selected){g.setColour(juce::Colour(0xff304c51));g.fillRect(0,0,width,height);}g.setColour(juce::Colour(0xffe2edf7));g.setFont(14.0f);g.drawText(juce::String(row+1).paddedLeft('0',2)+"   "+juce::String::fromUTF8(step.name.c_str()),12,0,width-135,height,juce::Justification::centredLeft);g.setColour(juce::Colour(0xff70e2bf));g.drawText(juce::String(step.beats,2)+t(" beats"," tempos"),width-125,0,112,height,juce::Justification::centredRight);
 }
 void selectedRowsChanged(int)override{selection();}
 bool valid()const{return list.getSelectedRow()>=0 && list.getSelectedRow()<static_cast<int>(steps.size());}
 void selection(){bool ok=valid();for(auto* b:{&load,&replace,&remove})b->setEnabled(ok);up.setEnabled(ok && list.getSelectedRow()>0);down.setEnabled(ok && list.getSelectedRow()+1<static_cast<int>(steps.size()));gate.setEnabled(ok && !steps[static_cast<size_t>(list.getSelectedRow())].rest);if(ok)gate.setValue(steps[static_cast<size_t>(list.getSelectedRow())].gate*100,juce::dontSendNotification);beats.setEnabled(ok);if(ok)beats.setValue(steps[static_cast<size_t>(list.getSelectedRow())].beats,juce::dontSendNotification);bool audible=std::any_of(steps.begin(),steps.end(),[](const auto& step){return !step.rest && !step.notes.empty();});play.setEnabled(audible);save.setEnabled(audible);add.setEnabled(steps.size()<128);}
 void remember(){if(beforeEdit)beforeEdit();}void changed(){if(onChanged)onChanged();}
 void move(int direction){if(!valid())return;int old=list.getSelectedRow(),next=old+direction;if(next<0 || next>=static_cast<int>(steps.size()))return;remember();std::swap(steps[static_cast<size_t>(old)],steps[static_cast<size_t>(next)]);changed();refresh(next);}
};
