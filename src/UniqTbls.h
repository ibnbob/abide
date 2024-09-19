//
//      File     : UniqTbls.h
//      Abstract : Classes for mainting unique tables for Bdd nodes.
//

#ifndef UNIQTBLS_H
#define UNIQTBLS_H

#include "BddImpl.h"
#include "BddNode.h"
#include "CacheStats.h"
#include "Defines.h"

//      Class    : UniqTbls
//      Abstract : Unique table for nodes with the same index (level).
class UniqTbl {
public:
  UniqTbl();
  ~UniqTbl() {
    // (_tbl) delete [] _tbl;
  };

  unsigned int size() const { return _size; };
  unsigned int numNodes() const { return _numNodes; };

  BDD findOrAdd(BddImpl &impl,
                int index,
                BDD hi,
                BDD lo);
  void resize(BddImpl &impl);
  BDD getHash(unsigned int hdx) const;
  void putHash(BddImpl &impl,
               BDD f,
               unsigned int hdx);
  void putHash(BddImpl &impl,
               BDD f);
  BDD clrHash(unsigned int hdx);

  // unsigned int hash2(BDD f, BDD g) const;
  unsigned int getMask() { return _mask; };

  void clear(BddImpl &impl, BDDVec &nodes);
  void setProcessed(bool b) { _processed = b; };
  bool processed() const { return _processed; };

  void freeTbl() {
    delete [] _tbl;
    _tbl = nullptr;
  }; // freeTbl

private:
  BDD *_tbl;
  unsigned int _size;
  unsigned int _mask;
  unsigned int _numNodes;
  bool _processed:1;
}; // UniqTbl

using UniqTblVec = std::vector<UniqTbl>;

//      Class    : UniqTbls
//      Abstract : All unique tables.
class UniqTbls {
public:
  UniqTbls(BddImpl &impl) : _impl(impl) {}; // CTOR
  ~UniqTbls(); // DTOR

  UniqTbls(const UniqTbls &) = delete; // Copy CTOR
  UniqTbls &operator=(const UniqTbls &) = delete; // Copy assignment
  UniqTbls(UniqTbls &&) = delete; // Move CTOR
  UniqTbls &operator=(UniqTbls &&) = delete; // Move assignment

  void resize(unsigned int nuSize) { _tables.resize(nuSize); };
  UniqTbl & operator[](unsigned int idx) { return _tables[idx]; };
  auto begin() { return _tables.begin(); };
  auto end() { return _tables.end(); };
private:
  BddImpl &_impl;
  UniqTblVec _tables;
}; // UniqTbls

#endif // UNIQTBLS_H
