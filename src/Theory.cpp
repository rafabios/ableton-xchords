#include "Theory.h"
#include <algorithm>
#include <stdexcept>
namespace xc {
int camelotRoot(int number, bool minor) {
 if(number<1 || number>12) throw std::out_of_range("Camelot number");
 static constexpr int majorRoots[]{11,6,1,8,3,10,5,0,7,2,9,4};
 const int root=majorRoots[number-1];
 return minor ? (root+9)%12 : root;
}
std::string camelotCode(int root, bool minor) {
 if(root<0 || root>11) throw std::out_of_range("Pitch class");
 for(int n=1;n<=12;++n) if(camelotRoot(n,minor)==root) return std::to_string(n)+(minor?"A":"B");
 return {};
}
const std::vector<Formula>& catalog(bool scales) {
 static const std::vector<Formula> chords {
  {"maj","Major","Maior",{0,4,7}}, {"min","Minor","Menor",{0,3,7}},
  {"dim","Diminished","Diminuto",{0,3,6}}, {"aug","Augmented","Aumentado",{0,4,8}},
  {"sus2","Suspended 2","Suspenso 2",{0,2,7}}, {"sus4","Suspended 4","Suspenso 4",{0,5,7}},
  {"7","Dominant 7","Sétima dominante",{0,4,7,10}}, {"maj7","Major 7","Maior com sétima maior",{0,4,7,11}},
  {"min7","Minor 7","Menor com sétima",{0,3,7,10}}, {"m7b5","Half diminished","Meio diminuto",{0,3,6,10}},
  {"dim7","Diminished 7","Sétima diminuta",{0,3,6,9}}, {"6","Major 6","Maior com sexta",{0,4,7,9}},
  {"m6","Minor 6","Menor com sexta",{0,3,7,9}}, {"add9","Add 9","Maior com nona adicionada",{0,4,7,14}},
  {"9","Dominant 9","Nona dominante",{0,4,7,10,14}}
 };
 static const std::vector<Formula> modes {
  {"major","Major / Ionian","Maior / Jônio",{0,2,4,5,7,9,11}},
  {"minor","Natural minor / Aeolian","Menor natural / Eólio",{0,2,3,5,7,8,10}},
  {"harmonic","Harmonic minor","Menor harmônica",{0,2,3,5,7,8,11}},
  {"melodic","Melodic minor (ascending form)","Menor melódica (forma ascendente)",{0,2,3,5,7,9,11}},
  {"pentmaj","Major pentatonic","Pentatônica maior",{0,2,4,7,9}},
  {"pentmin","Minor pentatonic","Pentatônica menor",{0,3,5,7,10}},
  {"blues","Minor blues","Blues menor",{0,3,5,6,7,10}},
  {"dorian","Dorian","Dórico",{0,2,3,5,7,9,10}},
  {"phrygian","Phrygian","Frígio",{0,1,3,5,7,8,10}},
  {"lydian","Lydian","Lídio",{0,2,4,6,7,9,11}},
  {"mixolydian","Mixolydian","Mixolídio",{0,2,4,5,7,9,10}},
  {"locrian","Locrian","Lócrio",{0,1,3,5,6,8,10}},
  {"whole","Whole tone","Tons inteiros",{0,2,4,6,8,10}},
  {"chromatic","Chromatic","Cromática",{0,1,2,3,4,5,6,7,8,9,10,11}}
 };
 return scales ? modes : chords;
}
std::string noteNames(int midi, int middleCOctave) {
 if(midi < 0 || midi > 127) throw std::out_of_range("MIDI pitch");
 const char* letters[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 const char* flats[] = {"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
 const char* sol[] = {"Dó","Dó#","Ré","Ré#","Mi","Fá","Fá#","Sol","Sol#","Lá","Lá#","Si"};
 const char* solFlat[] = {"Dó","Ré♭","Ré","Mi♭","Mi","Fá","Sol♭","Sol","Lá♭","Lá","Si♭","Si"};
 const int pc=midi%12; auto oct=std::to_string(midi/12-5+middleCOctave);
 std::string result=std::string(letters[pc])+oct;
 if(std::string(letters[pc])!=flats[pc]) result+=" / "+std::string(flats[pc])+oct;
 result+=" · "+std::string(sol[pc])+oct;
 if(std::string(sol[pc])!=solFlat[pc]) result+=" / "+std::string(solFlat[pc])+oct;
 return result+=" · MIDI "+std::to_string(midi);
}
std::vector<int> pitches(int root, const Formula& f, int inversion) {
 std::vector<int> result;
 for(auto n:f.intervals) result.push_back(root+n);
 if(result.empty()) return result;
 inversion=std::clamp(inversion,0,static_cast<int>(result.size())-1);
 for(int i=0;i<inversion;++i) { auto n=result.front()+12; result.erase(result.begin()); result.push_back(n); }
 std::sort(result.begin(),result.end());
 if(result.front()<0 || result.back()>127) throw std::out_of_range("MIDI range");
 return result;
}
std::vector<Event> sequence(std::vector<int> notes,int pattern,double step,int velocity) {
 if(step<=0 || velocity<1 || velocity>127 || pattern<0 || pattern>3) throw std::invalid_argument("Sequence settings");
 if(pattern==2) std::reverse(notes.begin(),notes.end());
 if(pattern==3 && notes.size()>1) for(int i=static_cast<int>(notes.size())-2;i>=0;--i) notes.push_back(notes[static_cast<size_t>(i)]);
 std::vector<Event> events;
 for(size_t i=0;i<notes.size();++i) events.push_back({notes[i],pattern==0?0.0:i*step,step,velocity});
 return events;
}
}
