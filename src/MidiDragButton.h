#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
class DragButton final : public juce::TextButton {
public:
 std::function<juce::File()> prepareMidi;
 std::function<void()> drag;
 bool sent=false;
 void mouseDown(const juce::MouseEvent& e) override {sent=false;TextButton::mouseDown(e);}
 void mouseDrag(const juce::MouseEvent& e) override {if(!sent && e.getDistanceFromDragStart()>5){sent=true;if(drag)drag();}}
};
