#pragma once
#include "Theory.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
namespace xc {
inline std::vector<int> tuning(int instrument) {
 if(instrument==1)return {40,45,50,55,59,64};
 if(instrument==3)return {23,28,33,38,43};
 return {28,33,38,43};
}
inline std::vector<int> uniqueNotes(std::vector<int> notes){std::sort(notes.begin(),notes.end());notes.erase(std::unique(notes.begin(),notes.end()),notes.end());return notes;}
inline std::vector<int> shapeNotes(const std::vector<int>& frets){std::vector<int> result;auto strings=tuning(1);for(size_t i=0;i<strings.size() && i<frets.size();++i)if(frets[i]>=0 && frets[i]<=12)result.push_back(strings[i]+frets[i]);return uniqueNotes(result);}
struct GuitarShape {std::vector<int> frets;int score=0;};
// Enumerate pitch-class-complete, inversion-bass shapes on frets 0..12.
// Fretted notes span at most four frets. These are suggestions, not a guarantee
// of ergonomic fingering (barres and individual finger assignments are not modelled).
inline std::vector<GuitarShape> guitarShapes(int root,const Formula& formula,int inversion=0) {
 int bassClass=(root+formula.intervals[static_cast<size_t>(std::clamp(inversion,0,static_cast<int>(formula.intervals.size())-1))])%12;
 int mask=0;for(int n:formula.intervals)mask|=1<<((root+n)%12);
 if(formula.intervals.size()>6)return {};
 auto strings=tuning(1);std::vector<GuitarShape> result;std::vector<int> frets(6,-1);
 std::function<void(int,int,int,int,int,int)> visit=[&](int string,int used,int lo,int hi,int count,int bass){
  if(string==6){if(used!=mask || count<3 || bass%12!=bassClass)return;int sum=0;for(int f:frets)sum+=std::max(0,f);result.push_back({frets,(6-count)*5+sum+(hi>0?hi-lo:0)*3});return;}
  frets[static_cast<size_t>(string)]=-1;visit(string+1,used,lo,hi,count,bass);
  for(int f=0;f<=12;++f){int note=strings[static_cast<size_t>(string)]+f;if(!(mask&(1<<(note%12))))continue;
   int low=f?std::min(lo,f):lo,high=std::max(hi,f);if(high>0 && high-low>3)continue;
   frets[static_cast<size_t>(string)]=f;visit(string+1,used|(1<<(note%12)),low,high,count+1,count?std::min(bass,note):note);
  }
 };
 visit(0,0,99,0,0,0);
 std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.score==b.score?a.frets<b.frets:a.score<b.score;});
 if(result.size()>64)result.resize(64);return result;
}
inline int movement(const std::vector<int>& from,const std::vector<int>& to){if(from.empty() || to.empty())return 0;int sum=0;for(int n:to){int best=128;for(int p:from)best=std::min(best,std::abs(n-p));sum+=best;}for(int n:from){int best=128;for(int p:to)best=std::min(best,std::abs(n-p));sum+=best;}return sum;}
inline std::vector<int> closeVoicing(const std::vector<int>& previous,const std::vector<int>& notes,int low,int high){
 if(previous.empty())return notes;std::vector<int> best=notes;int score=movement(previous,notes);
 // Preserve voice count and pitch classes; search inversions/octave placement.
 auto candidate=uniqueNotes(notes);for(size_t inversion=0;inversion<candidate.size();++inversion){for(int shift=-48;shift<=48;shift+=12){auto moved=candidate;for(auto& n:moved)n+=shift;if(moved.empty() || moved.front()<low || moved.back()>high)continue;int cost=movement(previous,moved);if(cost<score){score=cost;best=moved;}}
  candidate.front()+=12;std::sort(candidate.begin(),candidate.end());
 }return best;
}
inline std::vector<Event> bassLine(const std::vector<int>& source,int root,int instrument,int style,double length,int velocity){
 if(source.empty() || length<=0)return {};auto strings=tuning(instrument);int low=strings.front(),high=strings.back()+12;
 std::vector<int> available;for(int n=low;n<=high;++n)if(std::any_of(source.begin(),source.end(),[&](int p){return p%12==n%12;}))available.push_back(n);
 if(available.empty())return {};int anchor=available.front(),distance=128;for(int n:available)if(n%12==root && std::abs(n-source.front())<distance){anchor=n;distance=std::abs(n-source.front());}
 std::vector<int> line{anchor};
 if(style==1){int fifth=anchor,best=128;for(int n:available)if(n%12==(root+7)%12){int cost=std::abs(n-(anchor+7));if(cost<best){best=cost;fifth=n;}}int octave=anchor+12<=high?anchor+12:anchor;line={anchor,fifth,octave,fifth};}
 if(style==2){line.clear();for(int n:available)if(n>=anchor && n<=anchor+12)line.push_back(n);if(line.empty())line={anchor};}
 std::vector<Event> events;for(size_t i=0;i<line.size();++i)events.push_back({line[i],length*i/line.size(),length/line.size(),velocity});return events;
}
}
