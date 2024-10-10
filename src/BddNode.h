//
//      File     : BddNode.h
//      Abstract : BddNode class.
//

#ifndef BDDNODE_H
#define BDDNODE_H

#include <Bdd.h>
#include <cassert>

namespace abide {

//      Class    : BddNode
//      Abstract : A Bdd node.
class BddNode {
public:
  BddNode() = default;
  ~BddNode() = default;

#ifdef BANKEDMEM
  BddNode(const BddNode &) = delete; // Copy CTOR
  BddNode &operator=(const BddNode &) = delete; // Copy assignment
  BddNode(BddNode &&) = delete; // Move CTOR
  BddNode &operator=(BddNode &&) = delete; // Move assignment
#else
  BddNode(const BddNode &) = default; // Copy CTOR
  BddNode &operator=(const BddNode &) = default; // Copy assignment
  BddNode(BddNode &&) = default; // Move CTOR
  BddNode &operator=(BddNode &&) = default; // Move assignment
#endif

  void setIndex(unsigned int i) { _index = i; };
  unsigned int getIndex() const { return _index; };

  void setHi(BDD n) { _hi = n;};
  BDD getHi() const { return _hi; };
  void setLo(BDD n) { _lo = n;};
  BDD getLo() const { return _lo; };

  void setNext(BDD n) { _next = n;};
  BDD getNext() const { return _next; };

  // Avoid using n=0 unless gc is locked.
  void setMark(unsigned int n) {
    assert(n < 8);
    _marks |= (1<<n); };
  void clrMark(unsigned int n) {
    assert(n < 8);
    _marks &= ~(1<<n); };
  bool marked(unsigned int n) {
    assert(n < 8);
    return _marks & (1<<n); };

  void clear() {
    _hi = _lo = _next = 0;
    _index = _xrefs = 0;
    _marks = 0;
  };

  void incRef() { ++_xrefs; };
  void decRef() { --_xrefs; };
  unsigned int numRefs() const { return _xrefs; };
  void setRefs(unsigned int r) { _xrefs = r; };
private:
  BDD _hi;
  BDD _lo;
  BDD _next;
  BddIndex _index;
  uint32_t _xrefs:24;
  uint32_t _marks:8;
}; // BddNode

} // namespace abide

#endif // BDDNODE_H
