//
//      File     : BddImpl.h
//      Abstract :
//

#ifndef BDDIMPL_H
#define BDDIMPL_H

#include "Bdd.h"
#include "BddNode.h"
#include "CacheStats.h"

#include "Defines.h"
#include "UniqTbls.h"

#include <iomanip>
#include <map>
#include <unordered_map>
#include <vector>

namespace abide {

//
// Typedefs
//
namespace {
  const unsigned int BDD_MAX_INDEX = UINT32_MAX;

  const unsigned int BDD_VEC_LG_SZ = 14;
  const unsigned int BDD_VEC_SZ = (1<<BDD_VEC_LG_SZ);
  const unsigned int BDD_VEC_MASK = (BDD_VEC_SZ-1);

  const int DFLT_VAR_SZ = 128;
  const int DFLT_CACHE_SZ = (1<<20);
} // anonymous namespace


//      Class    : BddImpl
//      Abstract : Class for managing memory for BDDs.
class BddImpl {
public:
  // BddImpl.cc
  BddImpl(unsigned int numVars, unsigned int cacheSz);
  ~BddImpl();
  void initialize(unsigned int numVars, unsigned int cacheSz);

  BDD getLit(BddLit lit);
  BDD getIthLit(BddIndex index);
  unsigned int countNodes(BDDVec &bdds) const;
  void print(BDD f) const;

  // BddImplCalc.cc
  BDD apply(BDD f, BDD g, BddOp op);
  BDD restrict(BDD f, BDD c);
  BDD compose(BDD f, BddVar x, BDD g);
  BDD andExists(BDD f, BDD g, BDD c);
  bool covers(BDD f, BDD g);
  BDD cubeFactor(BDD f);
  BDD supportCube(BDD f);
  BDD oneCube(BDD f);

  BddVarVec supportVec(BDD f);

  // BddImplMem.cc
  unsigned int gc(bool force, bool verbose);
  unsigned int reorder(bool verbose);

  bool checkMem() const;
  unsigned nodesAllocd() const { return _nodesAllocd; };

  void incRef(BDD F) const;
  void decRef(BDD F) const;
  unsigned int numRefs(BDD f) const;
  unsigned int countFreeNodes() const;

  // Inline
  bool isOne(BDD f) const { return f == _oneNode; };
  bool isZero(BDD f) const { return f == _zeroNode; };
  bool isPosLit(BDD f) const { return (getXHi(f) == _oneNode) && (getXLo(f) == _zeroNode); }
  bool isNegLit(BDD f) const { return (getXHi(f) == _zeroNode) && (getXLo(f) == _oneNode); }
  bool isConstant(BDD f) const { return isOne(f) || isZero(f); };
  bool notConstant(BDD f) const { return ! isConstant(f); };

  BDD getThen(BDD f) const { return getXHi(f); };
  BDD getElse(BDD f) const { return getXLo(f); };

  BddVar getTopVar(BDD f) const { return getBddVar(f); };
  BddIndex getIndex(BDD f) const {
    BddNode &n = getNode(f);
    return n.getIndex();
  };

  void lockGC() { ++_gcLock; };
  void unlockGC() { if (_gcLock > 0) {--_gcLock;} };

  BDD getOne() const {return _oneNode; };
  BDD getZero() const {return _zeroNode; };
  BDD invert(BDD f) const { return f ^ 0x01; };
  BDD abs(BDD f) const { return f & ~0x01; };

  void printStats() { _cacheStats.print(); };
private:
  friend class UniqTbl;
  BDD apply2(BDD f, BDD g, BddOp op);

  enum Unateness {
    POS,
    NEG,
    BINATE
  };
  using FnSet = std::unordered_set<BDD>;
  BDD cubeFactor(BddIndexVec &indices, const FnSet &fns);
  Unateness getUnateness(BddIndex idx, const FnSet &fns);
  FnSet expandFnSet(BddIndex idx, const FnSet &fns);

  // Interface from type BDD type to type BddNode.
  bool isNegPhase(BDD f) const { return f & 0x01; };
  bool isPosPhase(BDD f) const { return !(f & 0x01); };
  unsigned int getBddVar(BDD f) const {
    BddNode &n = getNode(f);
    return _index2BddVar[n.getIndex()];
  };
  BDD getHi(BDD f) const {
    BddNode &n = getNode(f);
    return n.getHi();
  };
  BDD getLo(BDD f) const {
    BddNode &n = getNode(f);
    return n.getLo();
  };
  BDD getXHi(BDD f) const {
    BDD mask;
    BddNode &n = getNode(f, mask);
    return n.getHi() ^ mask;
  };
  BDD getXLo(BDD f) const {
    BDD mask;
    BddNode &n = getNode(f, mask);
    return n.getLo() ^ mask;
  };
  BDD getNext(BDD f) const {
    BddNode &n = getNode(f);
    return n.getNext();
  };
  void markNode(BDD f, unsigned int m) const {
    BddNode &n = getNode(f);
    n.setMark(m);
  };
  void unmarkNode(BDD f, unsigned int m) const {
    BddNode &n = getNode(f);
    n.clrMark(m);
  };
  bool nodeMarked(BDD f, unsigned int m) const {
    BddNode &n = getNode(f);
    return n.marked(m);
  };
  bool nodeUnmarked(BDD f, unsigned int m) const {
    BddNode &n = getNode(f);
    return ! n.marked(m);
  };

  // Find a node in memory.
  BddNode &getNode(BDD i) const;
  inline BddNode &getNode(BDD i, BDD &mask) const {
    mask = i & 0x01;
    return getNode(i);
  }; // getNode
  BddNode *getNodePtr(BDD i) const;

  // Allocating and freeing nodes.
  BDD allocateNode();
  void freeNode(BDD f);
  BDD findOrAddUniqTbl(unsigned int index,
                       BDD hi,
                       BDD lo);

  void markReferencedNodes();
  // Avoid using m=0 unless gc is locked.
  void markNodes(BDD f, unsigned int m) const;
  void unmarkNodes(BDD f, unsigned int m) const;

  // Reordering.
  using bddCntMap = std::map<BDD, unsigned int>;
  unsigned int getNextBddVar();
  unsigned long maxSize(unsigned long startSz) {
    return std::min(startSz + (startSz>>1), _maxNodes);
  } // maxSize
  void sift_udu(unsigned int index);
  void sift_dud(unsigned int index);
  long exchange(unsigned int index);

  void demote(const BDDVec &nodes, unsigned int index);
  void swapCofactors(const BDDVec &nodes, unsigned int index);
  void promote(const BDDVec &nodes, unsigned int index);

  void saveXRefs(bddCntMap &refs);
  void calcTRefs(const bddCntMap &refs);
  void restoreXRefs(bddCntMap &refs);
  void calcTRefs(BDD f);
  void decTRefs(BDD f);
  void incTRefs(BDD f);
  void setRefs(BDD f, unsigned int r) const;

  BDD and2(BDD f, BDD g);
  BDD xor2(BDD f, BDD g);
  BDD andConstant(BDD f, BDD g);
  BDD andExists2(BDD f, BDD g, BDD c);
  BDD andExistsTerminal(BDD f, BDD g, BDD c);
  void orderOps(BDD &f, BDD &g) {
    if (f > g) { std::swap(f, g); }
  }; // orderOps

  BDD ite(BDD f, BDD g, BDD h);
  bool stdTrip(BDD &f, BDD &g, BDD &h);
  void reduceThenElse(BDD &f, BDD &g, BDD &h);
  void swapArgs(BDD &f, BDD &g, BDD &h);
  void condSwap(BDD &f, BDD &g);
  void condSwapNeg(BDD &f, BDD &g);
  bool stdNegation(BDD &f, BDD &g, BDD &h);

  BDD makeNode(unsigned int index, BDD hi, BDD lo);

  BDD restrictRec(BDD f, BDD c);
  BDD restrictTerminal(BDD f, BDD c);
  BDD reduce(BDD f, unsigned int tgt);

  BDD supportCubeRec(const BDD f);
  unsigned int countNodes(BDD f) const;

  // Computed caches.
  BDD getAndCache(BDD f, BDD g);
  void insertAndCache(BDD f, BDD g, BDD r);
  BDD getXorCache(BDD f, BDD g);
  void insertXorCache(BDD f, BDD g, BDD r);
  BDD getRestrictCache(BDD f, BDD g);
  void insertRestrictCache(BDD f, BDD g, BDD r);
  BDD getIteCache(BDD f, BDD g, BDD h);
  void insertIteCache(BDD f,
                      BDD g,
                      BDD h,
                      BDD r);
  BDD getAndExistsCache(BDD f, BDD g, BDD h);
  void insertAndExistsCache(BDD f,
                            BDD g,
                            BDD c,
                            BDD r);

  void cleanCaches(bool force);
  void cleanAndCache(bool force);
  void cleanXorCache(bool force);
  void cleanRestrictCache(bool force);
  void cleanIteCache(bool force);
  void cleanAndExistsCache(bool force);

  unsigned int index(BDD f) const;
  unsigned int minIndex(BDD f, BDD g) const;
  unsigned int minIndex(BDD f, BDD g, BDD h) const;
  BDD restrict1(BDD f, unsigned int index) const;
  BDD restrict0(BDD f, unsigned int index) const;

  void printRec(BDD f, unsigned int level) const;

  //
  // Data Members.
  //

  // BddVariable-index correlation.
  std::vector<unsigned> _var2Index;
  std::vector<unsigned> _index2BddVar;

  // Counts
  unsigned int _gcLock;
  unsigned int _maxIndex;
  unsigned long _curNodes;
  unsigned long _maxNodes;
  unsigned long _nodesAllocd;
  unsigned long _maxAllocd;
  unsigned long _nodesFree;
  unsigned long _gcTrigger;

  // Managed node memory.
#ifdef BANKEDMEM
  using BddBank = BddNode *;
  using BddBanks = std::vector<BddBank>;
  BddBanks _banks;
#else
  BddNode *_nodes;
#endif

  // List of free nodes.
  BDD _freeList;

  // Constant nodes.
  BDD _nullNode;
  BDD _oneNode;
  BDD _zeroNode;

  // Unique tables.
  UniqTbls _uniqTbls;

  // Computed tables.
  unsigned int _compCacheSz;
  unsigned int _compCacheMask;

  struct CacheData2 {
    BDD _f;
    BDD _g;
    BDD _r;
  }; // CacheData2
  using ComputedTbl2 = std::vector<CacheData2>;
  ComputedTbl2 _andTbl;
  ComputedTbl2 _xorTbl;
  ComputedTbl2 _restrictTbl;

  struct CacheData3 {
    BDD _f;
    BDD _g;
    BDD _h;
    BDD _r;
  }; // CacheData3
  using ComputedTbl3 = std::vector<CacheData3>;
  ComputedTbl3 _iteTbl;
  ComputedTbl3 _andExistTbl;

  // Stats
  CacheStats _cacheStats;
}; // BddImpl


//      Function : BddImpl::getNode
//      Abstract : Decode the BDD address and return a reference.
inline BddNode &
BddImpl::getNode(BDD i) const
{
#ifdef BANKEDMEM
  i = i>>1;
  unsigned bdx = i >> BDD_VEC_LG_SZ;
  i &= BDD_VEC_MASK;
  BddBank bank = _banks[bdx];
  return bank[i];
#else
  i = i>>1;
  return _nodes[i];
#endif
} // BddImpl::getNode


//      Function : BddImpl::getNode
//      Abstract : Decode the BDD address and return a pointer.
inline BddNode *
BddImpl::getNodePtr(BDD i) const
{
#ifdef BANKEDMEM
  i = i>>1;
  unsigned bdx = i >> BDD_VEC_LG_SZ;
  i &= BDD_VEC_MASK;
  BddBank bank = _banks[bdx];
  return bank+i;
#else
  i = i>>1;
  return _nodes+i;
#endif
} // BddImpl::getNode


//      Function : BddImpl::makeNode
//      Abstract : Make a new BDD node if necessary.
inline BDD
BddImpl::makeNode(unsigned int index, BDD hi, BDD lo)
{
  BDD rtn = _nullNode;
  if (hi != lo) {
    rtn = findOrAddUniqTbl(index, hi, lo);
  } else {
    rtn = hi;
  } // if

  return rtn;
} // BddImpl::makeNode


//      Function : BddImpl::minIndex
//      Abstract : Return the minimum index of f, g and h.
inline unsigned int
BddImpl::minIndex(const BDD f, const BDD g) const
{
  return std::min(index(f), index(g));
} // BddImpl::minIndex


//      Function : BddImpl::minIndex
//      Abstract : Return the minimum index of f, g and h.
inline unsigned int
BddImpl::minIndex(const BDD f, const BDD g, const BDD h) const
{
  return std::min(index(f), std::min(index(g), index(h)));
} // BddImpl::minIndex


//      Function : BddImpl::restrict1
//      Abstract : Restrict f to idx. Assumes index(f) >= idx.
inline BDD
BddImpl::restrict1(const BDD f, const unsigned int idx) const
{
  unsigned int myIdx = index(f);
  assert(myIdx >= idx);
  return (myIdx != idx) ? f : getXHi(f);
} // restrict1


//      Function : BddImpl::restrict0
//      Abstract : Restrict f to !idx. Assumes index(f) >= idx.
inline BDD
BddImpl::restrict0(BDD f, unsigned int idx) const
{
  unsigned int myIdx = index(f);
  assert(myIdx >= idx);
  return (myIdx != idx) ? f : getXLo(f);
} // restrict0


///////////////////////////////////////////////////////////////////////////////////////
// Simple hash functions.
//
inline unsigned int hash2(unsigned int a,
                          unsigned int b) {
  unsigned int rtn = ((a >> 1) ^ b ^ (a << 9) ^ (b << 5));
  return rtn;
} // hash2

inline unsigned int hash3(unsigned int a,
                          unsigned int b,
                          unsigned int c) {
  unsigned int rtn = ((a >> 5) ^
                      (b >> 2) ^
                      (c << 1) ^
                      (a << 4) ^
                      (b << 7) ^
                      (c << 10));

  return rtn;
} // hash3

} // namespace abide

#endif // BDDIMPL_H
