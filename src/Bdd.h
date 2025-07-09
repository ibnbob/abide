//
//      File     : Bdd.h
//      Abstract : External BDD interface.
//

#ifndef BDD_H
#define BDD_H

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

namespace abide {

//
// Forward class declarations.
//
class BddImpl;
class Bdd;
class BddFnSet;

// Internal representaion of a BDD node is a 32-bit unsigned int.
using BDD = uint32_t;
using BDDVec = std::vector<BDD>;

enum BddOp {
  AND,
  NAND,
  OR,
  NOR,
  XOR,
  XNOR,
  IMPL
};

using BddVar = uint32_t;
using BddLit = int32_t;
using BddIndex = uint32_t;
using BddVec = std::vector<Bdd>;
using BddVarVec = std::vector<BddVar>;
using BddIndexVec = std::vector<BddIndex>;

//      Class    : BddMgr
//      Abstract : Manager for BDD memory and operations.
class BddMgr {
 public:
  BddMgr();
  BddMgr(size_t numVars);
  BddMgr(size_t numVars, size_t maxNodes);
  BddMgr(size_t numVars, size_t maxNodes, size_t cacheSz);
  ~BddMgr();

  BddMgr(const BddMgr &) = delete; // Copy CTOR
  BddMgr &operator=(const BddMgr &)  = delete; // Copy assignment
  BddMgr(BddMgr &&) = delete; // Move CTOR
  BddMgr &operator=(BddMgr &&)  = delete; // Move assignment

  Bdd getOne() const;
  Bdd getZero() const;
  Bdd getLit(BddLit) const;
  Bdd getIthLit(BddIndex) const;

  Bdd andExists(const Bdd f, const Bdd g, const Bdd c) const;
  Bdd ite(const Bdd f, const Bdd g, const Bdd h) const;

  size_t countNodes(BDD f) const;
  size_t countNodes(const BddVec &bdds) const;

  Bdd supportCube(const BddVec &bdds) const;
  BddVarVec supportVec(const BddVec &bdds) const;

  void lockGC() const;
  void unlockGC() const;
  size_t gc(bool force = false, bool verbose = false) const;
  size_t reorder(bool verbose = false) const;
  const BddVarVec &getVarOrder() const;

  bool checkMem() const;
  size_t nodesAllocd() const;
  size_t varsCreated() const;
  void setMaxNodes(size_t maxNodes);

  void printStats();
 private:
  friend class Bdd;

  bool isOne(const Bdd &f) const;
  bool isZero(const Bdd &f) const;
  bool isConstant(const Bdd &f) const;
  bool isPosLit(const Bdd &f) const;
  bool isNegLit(const Bdd &f) const;
  bool isCube(const Bdd &f) const;

  Bdd invert(BDD f) const;
  Bdd abs(BDD f) const;
  Bdd apply(BDD f, BDD g, BddOp op) const;
  Bdd andExists(const BDD f, const BDD g, const BDD c) const;
  Bdd restrict(BDD f, BDD c) const;
  Bdd compose(BDD f, BddVar x, BDD g) const;
  bool covers(BDD f, BDD g) const;
  Bdd cubeFactor(BDD f) const;

  Bdd getIf(BDD f) const;
  Bdd getThen(BDD f) const;
  Bdd getElse(BDD f) const;

  BddVar getTopVar(BDD f) const;
  BddIndex getIndex(BDD f) const;

  size_t supportSize(BDD f) const;
  BddVarVec supportVec(BDD f) const;
  Bdd supportCube(BDD f) const;

  Bdd oneCube(BDD f ) const;

  void incRef(BDD f) const;
  void decRef(BDD f) const;
  size_t numRefs(BDD f) const;
  void print(BDD f) const;

  // Class Data
  std::unique_ptr<BddImpl> _impl;
}; // BddMgr


//      Class    : Bdd
//      Abstract : Bdd class. Used to represent a single boolean function.
class Bdd {
 public:
  Bdd() : _mgr(0), _me(0) {};
  ~Bdd() { decRef(); };

  Bdd(const Bdd &f);
  Bdd& operator=(const Bdd &f);
  Bdd(Bdd &&); // Move CTOR
  Bdd& operator=(Bdd &&); // Move assignment

  const BddMgr *getMgr() const { return _mgr; };
  bool valid() const { return (_mgr != nullptr &&
                               getIndex() != 0); };

  // Operator overloads.
  Bdd operator~() const;
  Bdd& operator*=(const Bdd &rhs);
  friend Bdd operator*(Bdd lhs, const Bdd &rhs);
  Bdd& operator+=(const Bdd &rhs);
  friend Bdd operator+(Bdd lhs, const Bdd &rhs);
  Bdd& operator^=(const Bdd &rhs);
  friend Bdd operator^(Bdd lhs, const Bdd &rhs);
  Bdd& operator/=(const Bdd &rhs);
  friend Bdd operator/(Bdd lhs, const Bdd &rhs);

  // Named operations.
  // We use and2, or2 and xor2 because and, or and xor are reserved.
  // We use nand2, nor2, and xnor2 for consistency.
  Bdd inv() const;
  Bdd and2(const Bdd &f) const;
  Bdd nand2(const Bdd &f) const;
  Bdd or2(const Bdd &f) const;
  Bdd nor2(const Bdd &f) const;
  Bdd xor2(const Bdd &f) const;
  Bdd xnor2(const Bdd &f) const;
  Bdd implies(const Bdd &f) const;
  Bdd andExists(const Bdd &f, const Bdd &c) const;

  Bdd abs() const;
  Bdd restrict(const Bdd &f) const;
  Bdd compose(const BddVar x, const Bdd &g) const;

  // Comparison operations.
  bool covers(const Bdd &g) const;
  bool operator<=(const Bdd &rhs) const;
  bool operator==(const Bdd &rhs) const;
  bool operator!=(const Bdd &rhs) const;

  // Tests for trival functions.
  bool isOne() const;
  bool isZero() const;
  bool isConstant() const;
  bool isPosLit() const;
  bool isNegLit() const;
  bool isCube() const;

  // Cubes and support.
  Bdd cubeFactor() const;
  Bdd oneCube() const;

  size_t supportSize() const;
  BddVarVec supportVec() const;
  Bdd supportCube() const;

  // Data access.
  Bdd getIf() const;
  Bdd getThen() const;
  Bdd getElse() const;
  BddVar getTopVar() const;
  BddIndex getIndex() const;
  BDD getId() const;

  // Debug.
  size_t countNodes() const;
  size_t numRefs() const;
  void print() const;
 private:
  friend class BddMgr;
  friend class BddFnSet;

  Bdd(BDD f, const BddMgr *m) :
    _mgr(m), _me(f) { incRef(); };

  BDD id() const {
    return _me; };

  void incRef() const {
    if (_mgr) {
      _mgr->incRef(_me);
    } // if
  } // incRef

  void decRef() const {
    if (_mgr) {
      _mgr->decRef(_me);
    } // if
  } // decRef

  const BddMgr *_mgr;
  BDD _me;
}; // class Bdd

// Inline function definitions for class Bdd

// Construction and assignment.

inline Bdd::Bdd(const Bdd &f) :
  _mgr(f._mgr), _me(f._me)
{
  incRef();
} // Copy CTOR

inline Bdd& Bdd::operator=(const Bdd &f) {
  decRef();
  _mgr = f._mgr;
  _me = f._me;
  incRef();

  return *this;
} // Copy assignment.

inline Bdd::Bdd(Bdd &&f) :
  _mgr(std::move(f._mgr)),
  _me(std::move(f._me))
{
  f._mgr = nullptr;
} // Move CTOR

inline Bdd& Bdd::operator=(Bdd &&f) {
  decRef();
  _mgr = std::move(f._mgr);
  _me = std::move(f._me);
  f._mgr = nullptr;
  return *this;
} // Move assignment

// Operator overloads.

// NOT
inline Bdd Bdd::operator~() const {
  assert(_mgr);

  Bdd rtn;
  rtn = _mgr->invert(_me);
  return rtn;
} // operator~

// AND
inline Bdd& Bdd::operator*=(const Bdd &rhs) {
  assert(_mgr);
  assert(_mgr == rhs._mgr);

  *this = _mgr->apply(_me, rhs._me, AND);
  return *this;
} // Bdd::operator*=

inline Bdd operator*(Bdd lhs, const Bdd &rhs) {
  lhs *= rhs;
  return lhs;
} // operator*

// OR
inline Bdd& Bdd::operator+=(const Bdd &rhs) {
  assert(_mgr);
  assert(_mgr == rhs._mgr);

  *this = _mgr->apply(_me, rhs._me, OR);
  return *this;
} // Bdd::operator+=

inline Bdd operator+(Bdd lhs, const Bdd &rhs) {
  lhs += rhs;
  return lhs;
} // operator+

// XOR
inline Bdd& Bdd::operator^=(const Bdd &rhs) {
  assert(_mgr);
  assert(_mgr == rhs._mgr);

  *this = _mgr->apply(_me, rhs._me, XOR);
  return *this;
} // Bdd::operator^=

inline Bdd operator^(Bdd lhs, const Bdd &rhs) {
  lhs ^= rhs;
  return lhs;
} // operator^

// Restrict
inline Bdd& Bdd::operator/=(const Bdd &rhs) {
  assert(_mgr);
  assert(_mgr == rhs._mgr);

  *this = _mgr->restrict(_me, rhs._me);
  return *this;
} // Bdd::operator/=

inline Bdd operator/(Bdd lhs, const Bdd &rhs) {
  lhs /= rhs;
  return lhs;
} // operator/

// Named operations.

inline Bdd Bdd::inv() const {
  assert(_mgr);

  return _mgr->invert(_me);
} // Bdd::inv

inline Bdd Bdd::and2(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, AND);
} // Bdd::and2

inline Bdd Bdd::nand2(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, NAND);
} // Bdd::nand2

inline Bdd Bdd::or2(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, OR);
} // Bdd::or2

inline Bdd Bdd::nor2(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, NOR);
} // Bdd::nor2

inline Bdd Bdd::xor2(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, XOR);
} // Bdd::xor2

inline Bdd Bdd::xnor2(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, XNOR);
} // Bdd::xnor2

inline Bdd Bdd::implies(const Bdd &f) const {
  assert(_mgr);

  return _mgr->apply(_me, f._me, IMPL);
} // Bdd::implies

inline Bdd Bdd::andExists(const Bdd &f, const Bdd &c) const {
  assert(_mgr);

  return _mgr->andExists(_me, f._me, c._me);
} // Bdd::andExists

inline Bdd Bdd::abs() const {
  assert(_mgr);

  return _mgr->abs(_me);
} // Bdd::abs

inline Bdd Bdd::restrict(const Bdd &f) const {
  assert(_mgr);

  return _mgr->restrict(_me, f._me);
} // Bdd::restrict

inline Bdd Bdd::compose(const BddVar x, const Bdd &g) const {
  assert(_mgr);
  assert(_mgr == g._mgr);

  return _mgr->compose(_me, x, g._me);
} // Bdd::compose

// Comparison operations.

inline bool Bdd::covers(const Bdd &g) const {
  assert(_mgr);
  assert(_mgr == g._mgr);

  return _mgr->covers(_me, g._me);
} // Bdd::covers

inline bool Bdd::operator<=(const Bdd &f) const {
  assert(_mgr);
  assert(_mgr == f._mgr);

  return _mgr->covers(f._me, _me);
} // Bdd::operator<=

inline bool Bdd::operator==(const Bdd &f) const {
  return _mgr == f._mgr && _me == f._me;
} // Bdd::operator&

inline bool Bdd::operator!=(const Bdd &f) const {
  return _mgr != f._mgr || _me != f._me;
} // Bdd::operator&

// Tests for trival functions.

inline bool Bdd::isOne() const {
  assert(_mgr);

  return _mgr->isOne(*this);
} // Bdd::isOne

inline bool Bdd::isZero() const {
  assert(_mgr);

  return _mgr->isZero(*this);
} // Bdd::isZero

inline bool Bdd::isConstant() const {
  assert(_mgr);

  return _mgr->isConstant(*this);
} // Bdd::isConstant

inline bool Bdd::isPosLit() const {
  assert(_mgr);

  return _mgr->isPosLit(*this);
} // Bdd::isPosLit

inline bool Bdd::isNegLit() const {
  assert(_mgr);

  return _mgr->isNegLit(*this);
} // Bdd::isNegLit

// Cubes and support.

inline bool Bdd::isCube() const {
  assert(_mgr);

  return _mgr->isCube(*this);
} // Bdd::isCube

// Cubes and support.

inline Bdd Bdd::cubeFactor() const {
  assert(_mgr);

  return _mgr->cubeFactor(_me);
} // Bdd::cubeFactor

inline Bdd Bdd::oneCube() const {
  assert(_mgr);

  return _mgr->oneCube(_me);
} // Bdd::oneCube

inline size_t Bdd::supportSize() const {
  assert(_mgr);

  return _mgr->supportSize(_me);
} // Bdd::supportSize

inline BddVarVec Bdd::supportVec() const {
  assert(_mgr);

  return _mgr->supportVec(_me);
} // Bdd::supportVec

inline Bdd Bdd::supportCube() const {
  assert(_mgr);

  return _mgr->supportCube(_me);
} // Bdd::supportCube

// Data access.

inline Bdd Bdd::getIf() const{
  assert(_mgr);

  return _mgr->getIf(_me);
} // Bdd::getIf

inline Bdd Bdd::getThen() const{
  assert(_mgr);

  return _mgr->getThen(_me);
} // Bdd::getThen

inline Bdd Bdd::getElse() const{
  assert(_mgr);

  return _mgr->getElse(_me);
} // Bdd::getElse

inline BddVar Bdd::getTopVar() const {
  assert(_mgr);

  return _mgr->getTopVar(_me);
} // Bdd::getTopVar

inline BddVar Bdd::getIndex() const {
  assert(_mgr);

  return _mgr->getIndex(_me);
} // Bdd::getIndex

inline BDD Bdd::getId() const {
  assert(_mgr);

  return _me;
} // Bdd::getId();

// Debug.

inline size_t Bdd::countNodes() const {
  assert(_mgr);

  return _mgr->countNodes(_me);
} // Bdd::countNodes

inline size_t Bdd::numRefs() const
{
  assert(_mgr);

  return _mgr->numRefs(_me);
} // Bdd::numRefs


//      Class    : BddFnSet
//      Abstract : Maintains a set of unigue Bdd functions. Note that
//      two Bdd objects may refer to the same logic function. If both
//      are inserted into a BddFnSet, only the first insertion will
//      succeed.
class BddFnSet {
 public:
  BddFnSet() : _mgr(nullptr) {}; // CTOR
  ~BddFnSet() {}; // DTOR

  BddFnSet(const BddFnSet &) = default; // Copy CTOR
  BddFnSet &operator=(const BddFnSet &) = default; // Copy assignment
  BddFnSet(BddFnSet &&) = default; // Move CTOR
  BddFnSet &operator=(BddFnSet &&) = default; // Move assignment

  bool insert(Bdd f);
  bool erase(Bdd f);
  void clear() {_bddSet.clear();};
  int size();

  Bdd getTop();
  BddFnSet restrict(const Bdd lit) const;
  BddFnSet eliminate(const Bdd lit) const;

  struct BddHash {
    std::size_t operator()(const Bdd &f) const noexcept {
      return std::hash<BDD>{}(f.id());
    } // operator
  }; // BddHash

  using Set = std::unordered_set<Bdd, BddHash>;

  Set &getSet() {return _bddSet;}
  const Set &getSet() const {return _bddSet;}
  auto begin() { return _bddSet.begin(); };
  auto end() { return _bddSet.end(); };
 private:
  friend class BddMgr;
  friend Bdd findProduct(BddFnSet &fnSet);

  Set _bddSet;
  const BddMgr *_mgr;
}; // BddFnSet

} // namespace abide

#endif // BDD_H
