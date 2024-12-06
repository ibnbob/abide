//      Class    : Timer
//      Abstract : Compute CPU time used by a scope.

#include <ctime>
#include <iostream>
#include <string>

class Timer {
public:
  Timer(std::string name) :
    _name(name),
    _start(std::clock()) {}; // CTOR
  ~Timer(){
    double time = std::clock() - _start;
    time /= static_cast<double>(CLOCKS_PER_SEC);
    std::cout << "\
================================================================"
              << std::endl;
    std::printf("%-12s: %.2f\n", _name.c_str(), time);
    std::cout << "\
================================================================"
              << std::endl;
  }; // DTOR

  Timer(const Timer &) = delete; // Copy CTOR
  Timer &operator=(const Timer &) = delete; // Copy assignment
  Timer(Timer &&) = delete; // Move CTOR
  Timer &operator=(Timer &&) = delete; // Move assignment
private:
  std::string _name;
  clock_t _start;
}; // Timer

