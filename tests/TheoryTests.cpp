#include "Theory.h"
#include "MusicalState.h"
#include <iostream>
#include <stdexcept>
void require(bool ok) { if(!ok) throw std::runtime_error("Test failed"); }
int main() {
 require(xc::camelotCode(0,false)=="8B");require(xc::camelotCode(9,true)=="8A");
 require(xc::camelotRoot(1,true)==8);require(xc::camelotRoot(12,true)==1);
 const int major[]{11,6,1,8,3,10,5,0,7,2,9,4};const int minor[]{8,3,10,5,0,7,2,9,4,11,6,1};
 for(int n=1;n<=12;++n) {require(xc::camelotRoot(n,false)==major[n-1]);require(xc::camelotRoot(n,true)==minor[n-1]);require(xc::camelotCode(major[n-1],false)==std::to_string(n)+"B");require(xc::camelotCode(minor[n-1],true)==std::to_string(n)+"A");}
 xc::Settings s;s.scale=1;s.pattern=1;xc::toggleNote(s,64);xc::toggleNote(s,63);
 require(s.custom && xc::selectedNotes(s)==std::vector<int>({60,62,63,65,67,69,71,72}));
 auto edited=xc::makeEvents(s);require(edited.size()==8 && edited[2].note==63);
 for(auto n:xc::selectedNotes(s))xc::toggleNote(s,n);
 require(s.custom && xc::makeEvents(s).empty());xc::resetNotes(s);require(!s.custom && xc::selectedNotes(s).size()==8);
 require(xc::noteNames(33)=="A1 · Lá1 · MIDI 33");
 require(xc::noteNames(34)=="A#1 / Bb1 · Lá#1 / Si♭1 · MIDI 34");
 require(xc::noteNames(65)=="F4 · Fá4 · MIDI 65");
 require(xc::noteNames(60,3)=="C3 · Dó3 · MIDI 60");
 require(xc::pitches(60,xc::catalog(false)[0],1)==std::vector<int>({64,67,72}));
 require(xc::pitches(60,xc::catalog(true)[0])==std::vector<int>({60,62,64,65,67,69,71}));
 auto e=xc::sequence({60,64,67},3,0.5,90);
 require(e.size()==5 && e[3].note==64 && e[4].note==60 && e[4].start==2.0);
 require(xc::sequence({60,64,67},0,1,90)[2].start==0);
 for(bool scales:{false,true}) for(const auto& f:xc::catalog(scales)) for(int r=24;r<84;++r) {
  auto notes=xc::pitches(r,f); require(!notes.empty());
  for(auto n:notes) require(n>=0 && n<=127);
 }
 std::cout<<"Theory: notation, octaves, inversions, scales, sequences and ranges OK\n";
}
