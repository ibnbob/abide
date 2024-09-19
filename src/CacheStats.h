//
//      File     : CacheStats.h
//      Abstract : Struct for keeping cache access statistics.
//

#ifndef CACHESTATS_H
#define CACHESTATS_H

#include "Defines.h"
#include <iomanip>

struct CacheStats {
#ifdef CACHESTATS
  void incUniqAccess() { ++_uniqAccess; };
  void incUniqChain()  { ++_uniqChain;  };
  void incUniqHit()    { ++_uniqHit;    };
  void incUniqMiss()   { ++_uniqMiss;   };
  void incCompHit()    { ++_compHit;    };
  void incCompMiss()   { ++_compMiss;   };
  void print() {
    double hitRate = (_uniqAccess > 0 ?
                      100.0 * static_cast<double>(_uniqHit) /
                      static_cast<double>(_uniqAccess) :
                      0.0);
    cout << "Cache Statistics\n"
         << "----------------\n"
         << "Unique Access: " << _uniqAccess << endl
         << "Unique Chain : " << _uniqChain  << endl
         << "Unique Hit   : " << _uniqHit    << endl
         << "Unique Miss  : " << _uniqMiss   << endl
         << "Hit Rate     : " << std::setprecision(4) << hitRate << "%" << endl;
    hitRate = ((_compMiss + _compHit > 0) ?
               100.0 * static_cast<double>(_compHit) /
               static_cast<double>(_compHit + _compMiss):
               0.0);
    cout << "Compute Hit  : " << _compHit << endl
         << "Compute Miss : " << _compMiss <<endl
         << "HitRate      : " << std::setprecision(4) << hitRate << "%" << endl;
  } // print
#else
  void incUniqAccess() {};
  void incUniqChain()  {};
  void incUniqHit() {};
  void incUniqMiss() {};
  void incCompHit() {};
  void incCompMiss() {};
  void print() {};
#endif

private:
  unsigned long _uniqAccess = 0;
  unsigned long _uniqChain = 0;
  unsigned long _uniqHit = 0;
  unsigned long _uniqMiss = 0;
  unsigned long _compHit = 0;
  unsigned long _compMiss = 0;
}; // struct CacheStats

#endif // CACHESTATS_H
