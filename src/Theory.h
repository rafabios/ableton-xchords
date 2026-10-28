#pragma once
#include <string>
#include <vector>
namespace xc {
struct Formula { std::string id, en, pt; std::vector<int> intervals; };
const std::vector<Formula>& catalog(bool scales);
// Camelot: A = minor, B = major. Number is 1..12, root is a pitch class.
int camelotRoot(int number, bool minor);
std::string camelotCode(int root, bool minor);
std::string noteNames(int midi, int middleCOctave = 4);
std::vector<int> pitches(int rootMidi, const Formula&, int inversion = 0);
struct Event { int note; double start, duration; int velocity; };
// pattern: 0 block, 1 ascending, 2 descending, 3 up/down.
std::vector<Event> sequence(std::vector<int> notes, int pattern, double step, int velocity);
}
