//
//      File     : Ticker.h
//	Abstract : Class to implement simple progress bar.
//

#ifndef TICKER_H
#define TICKER_H

#include <iostream>
#include <cmath>

//      Class    : Ticker
//      Abstract : Class to implement simple progress bar.
class Ticker {
public:
  Ticker(int total) :
    _total(total),
    _ticks(0),
    _tocks(0)
  {
    std::cout << '|' << std::flush;
  }; // CTOR
  ~Ticker(){
    tick(_total-_ticks);
  }; // DTOR

  Ticker(const Ticker &) = delete; // Copy CTOR
  Ticker &operator=(const Ticker &) = delete; // Copy assignment
  Ticker(Ticker &&) = delete; // Move CTOR
  Ticker &operator=(Ticker &&) = delete; // Move assignment

  void tick(int ticks = 1) {
    while (ticks) {
      --ticks;
      ++_ticks;

      double p = static_cast<double>(_ticks) / static_cast<double>(_total);
      int nextTock = std::floor(p * 50);
      while (_tocks < nextTock) {
	++_tocks;
	if (_tocks % 5) {
	  std::cout << '-' << std::flush;
	} else if (_tocks < 50) {
	  std::cout << _tocks / 5 << std::flush;
	} else {
	  std::cout << '|' << std::endl;
	} // if done
      } // while
    } // while
  };
private:
  int _total;
  int _ticks;
  int _tocks;
}; // Ticker

#endif // TICKER_H
