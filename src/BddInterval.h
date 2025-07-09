//
//      File     : BddInterval.h
//      Abstract : Class for manipulating intervals. An interval,
//      F = [f0, f1] where f0 <= f1, is a representation of an
//      incompletely-specified function. A function f is a valid
//      implementation of F if f0 <= f <= f1. If F is specified as an
//      on-set and don't-care set, F = (f, d), then the equivalent
//      interval representation is F = [f*~d, f+d]. Conversely, if F
//      is specified as an interval, the don't-care set is d = f1*~f0
//      and the on-set is any f' in F. Consequently, F has infinitely
//      many representations using the (f, d) representation, while
//      there is exactly one interval representation.

#ifndef BDDINTERVAL_H
#define BDDINTERVAL_H

#include <Bdd.h>

namespace abide {

//      Class    : BddInterval
//      Abstract : Class for representing and manipulating intervals.
class BddInterval {
public:
  BddInterval() :
    _min(Bdd()), _max(Bdd()) {};
  BddInterval(const Bdd &f0, const Bdd &f1) :
    _min(f0), _max(f1) { assert(f0 <= f1); };
  BddInterval(const Bdd &f) :
    _min(f), _max(f) { };
  // Weird constructor of constant X.
  BddInterval(const BddMgr &mgr) :
    _min(mgr.getZero()),
    _max(mgr.getOne()) { };
  ~BddInterval() = default; // DTOR

  // Construction and assignment.
  BddInterval(const BddInterval &f); // Copy CTOR
  BddInterval &operator=(const BddInterval &f); // Copy assignment
  BddInterval(BddInterval &&f); // Move CTOR
  BddInterval &operator=(BddInterval &&f); // Move assignment

  // Operator overloads.
  BddInterval operator~() const;
  BddInterval& operator*=(const BddInterval &rhs);
  friend BddInterval operator*(BddInterval lhs, const BddInterval &rhs);
  BddInterval& operator+=(const BddInterval &rhs);
  friend BddInterval operator+(BddInterval lhs, const BddInterval &rhs);
  BddInterval& operator^=(const BddInterval &rhs);
  friend BddInterval operator^(BddInterval lhs, const BddInterval &rhs);

  // Comparison operations.
  bool operator<=(const BddInterval &rhs) const;
  bool operator==(const BddInterval &rhs) const;
  bool operator!=(const BddInterval &rhs) const;
  friend bool operator<=(const Bdd &lhs, const BddInterval &rhs);

  // Forces to interval [0,1], i.e. constant X.
  void toX(const BddMgr &mgr);

  // Member access.
  Bdd min() { return _min; };
  Bdd max() { return _max; };
  Bdd getTopVar() {
    BddVar x = (_min.getIndex() < _max.getIndex()
                ? _min.getTopVar()
                : _max.getTopVar());
    return _min.getMgr()->getLit(x);
  }; // getTopVar

  bool isZero() { return _max.isZero(); } ;
  bool isOne() { return _min.isOne(); } ;
  bool isX() { return _min.isZero() && _max.isOne(); } ;
  bool valid() { return _min.valid() && _max.valid(); };

 private:
  Bdd _min;
  Bdd _max;
}; // BddInterval

// Construction and assignment.

inline BddInterval::BddInterval(const BddInterval &f) :
  _min(f._min),
  _max(f._max)
{ } // Copy CTOR

inline BddInterval& BddInterval::operator=(const BddInterval &f)
{
  _min = f._min;
  _max = f._max;
  return *this;
} // Copy assignment.

inline BddInterval::BddInterval(BddInterval &&f) :
  _min(std::move(f._min)),
  _max(std::move(f._max))
{
  f._min = Bdd();
  f._max = Bdd();
} // Move CTOR

inline BddInterval& BddInterval::operator=(BddInterval &&f) {
  _min = std::move(f._min);
  _max = std::move(f._max);
  f._min = Bdd();
  f._max = Bdd();
  return *this;
} // Move assignment

// Operator overloads.

inline BddInterval BddInterval::operator~() const {
  BddInterval rtn(~_max, ~_min);
  return rtn;
} // BddInterval::operator~

inline BddInterval& BddInterval::operator*=(const BddInterval &rhs) {
  _min = _min * rhs._min;
  _max = _max * rhs._max;
  assert(_min <= _max);
  return *this;
} // BddInterval::operator*=

inline BddInterval operator*(BddInterval lhs, const BddInterval &rhs) {
  lhs *= rhs;
  return lhs;
} // operator*

inline BddInterval& BddInterval::operator+=(const BddInterval &rhs) {
  _min = _min + rhs._min;
  _max = _max + rhs._max;
  assert(_min <= _max);
  return *this;
} // BddInterval::operator+=

inline BddInterval operator+(BddInterval lhs, const BddInterval &rhs) {
  lhs += rhs;
  return lhs;
} // operator+

inline BddInterval& BddInterval::operator^=(const BddInterval &rhs) {
  Bdd nuMin = _min*~rhs._max + ~_max*rhs._min;
  Bdd nuMax = _max*~rhs._min + ~_min*rhs._max;
  _min = nuMin;
  _max = nuMax;
  assert(_min <= _max);
  return *this;
} // BddInterval::operator^=

inline BddInterval operator^(BddInterval lhs, const BddInterval &rhs) {
  lhs ^= rhs;
  return lhs;
} // operator^

// Comparison operations.

inline bool BddInterval::operator<=(const BddInterval &f) const {
  return (f._min <= _min) && (_max <= f._max);
} // BddInterval::operator<=

inline bool BddInterval::operator==(const BddInterval &f) const {
  return (f._min == _min) && (_max == f._max);
} // BddInterval::operator==

inline bool BddInterval::operator!=(const BddInterval &f) const {
  return (f._min != _min) || (_max != f._max);
} // BddInterval::operator==

inline bool operator<=(const Bdd &lhs, const BddInterval &rhs) {
  return (rhs._min <= lhs) && (lhs <= rhs._max);
} // operator<=

// Miscellaneous.
inline void BddInterval::toX(const BddMgr &mgr) {
  _min = mgr.getZero();
  _max = mgr.getOne();
} // BddInterval::toX

} // namespace abide

#endif // BDDINTERVAL_H
