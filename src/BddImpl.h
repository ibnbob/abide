//
//      File     : BddImpl.h
//      Abstract : Interface into BDD implementation.
//

#ifndef BDDIMPL_H
#define BDDIMPL_H

#include "Bdd.h"
#include "BddNode.h"
#include "CacheStats.h"

#include "Defines.h"
#include "UniqTbls.h"

#include <map>
#include <vector>

namespace abide {

//
// Typedefs
//
namespace {
const uint32_t BDD_MAX_INDEX = UINT32_MAX;

const size_t BDD_VEC_LG_SZ = 14;
const size_t BDD_VEC_SZ = (1<<BDD_VEC_LG_SZ);
const size_t BDD_VEC_MASK = (BDD_VEC_SZ-1);

const size_t DFLT_VAR_SZ = 0;
const size_t DFLT_NODE_SZ = UINT32_MAX;
const size_t DFLT_CACHE_SZ = (1<<20);

const double DFLT_REORDER_GROWTH_FACTOR = 1.25;
} // anonymous namespace


//      Class    : BddImpl
//      Abstract : Class for managing memory for BDDs.
class BddImpl {
 public:
  // BddImpl.cc
  BddImpl(size_t numVars,
          size_t maxNodes,
          size_t cacheSz);
  ~BddImpl();
  void initialize(size_t numVars, size_t cacheSz);

  BDD getLit(BddLit lit);
  BDD getIthLit(BddIndex index);
  size_t countNodes(BDDVec &bdds) const;
  void print(BDD f) const;

  // BddImplCalc.cc
  BDD apply(BDD f, BDD g, BddOp op);
  BDD restrict(BDD f, BDD c);
  BDD compose(BDD f, BddVar x, BDD g);
  BDD andExists(BDD f, BDD g, BDD c);
  bool covers(BDD f, BDD g);
  BDD cubeFactor(BDD f);
  BDD oneCube(BDD f);
  BDD ite(BDD f, BDD g, BDD h);

  size_t supportSize(BDD f);
  BDD supportCube(BDD f);
  BddVarVec supportVec(BDD f);

  // BddImplMem.cc
  size_t gc(bool force, bool verbose);
  size_t reorder(bool verbose);

  const BddVarVec &getVarOrder() const { return _index2BddVar; };

  bool checkMem() const;
  size_t nodesAllocd() const { return _nodesAllocd; };
  size_t varsCreated() const { return _maxIndex; };

  void incRef(BDD F) const;
  void decRef(BDD F) const;
  size_t numRefs(BDD f) const;
  size_t countFreeNodes() const;

  // Inline
  bool isOne(BDD f) const { return f == _oneNode; };
  bool isZero(BDD f) const { return f == _zeroNode; };
  bool isPosLit(BDD f) const { return (getXHi(f) == _oneNode) && (getXLo(f) == _zeroNode); }
  bool isNegLit(BDD f) const { return (getXHi(f) == _zeroNode) && (getXLo(f) == _oneNode); }
  bool isConstant(BDD f) const { return isOne(f) || isZero(f); };
  bool notConstant(BDD f) const { return ! isConstant(f); };
  bool isNull(BDD f) const {return f == _nullNode; };

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
  BDD invert(BDD f) const { return f ? f ^ 0x01 : f; };
  BDD abs(BDD f) const { return f & ~0x01; };

  void setMaxNodes(size_t maxNodes) {_maxNodes = std::max(_nodesAllocd,maxNodes);};
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
  BddVar getBddVar(BDD f) const {
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
  void markNode(BDD f, uint32_t m) const {
    BddNode &n = getNode(f);
    n.setMark(m);
  };
  void unmarkNode(BDD f, uint32_t m) const {
    BddNode &n = getNode(f);
    n.clrMark(m);
  };
  bool nodeMarked(BDD f, uint32_t m) const {
    BddNode &n = getNode(f);
    return n.marked(m);
  };
  bool nodeUnmarked(BDD f, uint32_t m) const {
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
  void allocateMoreNodes();
  void freeNode(BDD f);
  BDD findOrAddUniqTbl(BddIndex index,
                       BDD hi,
                       BDD lo);

  void markReferencedNodes();
  // Avoid using m=0 unless gc is locked.
  void markNodes(BDD f, uint32_t m) const;
  void unmarkNodes(BDD f, uint32_t m) const;

  // Reordering.
  using bddCntMap = std::map<BDD, size_t>;
  BddIndex getNextBddVar();
  size_t maxSize(size_t startSz) {
    size_t maxSz = startSz * DFLT_REORDER_GROWTH_FACTOR;
    return std::min(maxSz, _maxNodes);
    //return std::min(startSz + (startSz>>1), _maxNodes);
  } // maxSize
  void sift_udu(BddIndex index);
  void sift_dud(BddIndex index);
  long exchange(BddIndex index);

  void demote(const BDDVec &nodes, BddIndex index);
  void swapCofactors(const BDDVec &nodes, BddIndex index);
  void promote(const BDDVec &nodes, BddIndex index);

  void saveXRefs(bddCntMap &refs);
  void calcTRefs(const bddCntMap &refs);
  void restoreXRefs(bddCntMap &refs);
  void calcTRefs(BDD f);
  void decTRefs(BDD f);
  void incTRefs(BDD f);
  void setRefs(BDD f, uint32_t r) const;

  BDD and2(BDD f, BDD g);

  BDD xor2(BDD f, BDD g);
  BDD andConstant(BDD f, BDD g);
  bool andConstantTerminal(BDD f, BDD g, BDD &rtn);
  BDD andExists2(BDD f, BDD g, BDD c);
  BDD andExistsTerminal(BDD f, BDD g, BDD c);
  void orderOps(BDD &f, BDD &g) {
    if (f > g) { std::swap(f, g); }
  }; // orderOps

  BDD nand2(BDD f, BDD g) { return invert(and2(f, g)); };
  BDD or2(BDD f, BDD g) { return invert(and2(invert(f), invert(g))); };
  BDD nor2(BDD f, BDD g) { return and2(invert(f), invert(g)); } ;
  BDD xnor2(BDD f, BDD g) { return invert(xor2(f, g)); };
  BDD impl2(BDD f, BDD g) { return invert(and2(f, invert(g))); };

  //ite
  bool stdTrip(BDD &f, BDD &g, BDD &h);
  void reduceThenElse(BDD &f, BDD &g, BDD &h);
  void swapArgs(BDD &f, BDD &g, BDD &h);
  void condSwap(BDD &f, BDD &g);
  void condSwapNeg(BDD &f, BDD &g);
  bool stdNegation(BDD &f, BDD &g, BDD &h);

  BDD makeNode(BddIndex index, BDD hi, BDD lo);

  BDD restrictRec(BDD f, BDD c);
  bool restrictTerminal(BDD f, BDD c, BDD &rtn);
  BDD reduce(BDD f, BddIndex tgt);

  using BitVec = std::vector<bool>;
  void fillSupportVec(BDD f, BitVec &suppVec);
  size_t countNodes(BDD f) const;

  // Computed caches.
  struct CacheData2 {
    BDD _f;
    BDD _g;
    BDD _r;
  }; // CacheData2
  using ComputedTbl2 = std::vector<CacheData2>;

  struct CacheData3 {
    BDD _f;
    BDD _g;
    BDD _h;
    BDD _r;
  }; // CacheData3
  using ComputedTbl3 = std::vector<CacheData3>;


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
  void cleanCache(ComputedTbl2 &table, bool force);
  void cleanCache(ComputedTbl3 &table, bool force);

  void cleanAndCache(bool force);
  void cleanXorCache(bool force);
  void cleanRestrictCache(bool force);
  void cleanIteCache(bool force);
  void cleanAndExistsCache(bool force);

  BddIndex index(BDD f) const;
  BddIndex minIndex(BDD f, BDD g) const;
  BddIndex minIndex(BDD f, BDD g, BDD h) const;
  BDD restrict1(BDD f, BddIndex index) const;
  BDD restrict0(BDD f, BddIndex index) const;

  void printRec(BDD f, size_t level) const;

  //
  // Data Members.
  //

  // BddVariable-index correlation.
  std::map<BddVar, BddIndex> _var2Index;
  BddVarVec _index2BddVar;

  // Counts
  size_t _gcLock;
  size_t _maxIndex;
  size_t _curNodes;
  size_t _maxNodes;
  size_t _nodesAllocd;
  size_t _maxAllocd;
  size_t _nodesFree;
  size_t _gcTrigger;

  bool _reordering;

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
  size_t _compCacheSz;
  size_t _compCacheMask;

  ComputedTbl2 _andTbl;
  ComputedTbl2 _xorTbl;
  ComputedTbl2 _restrictTbl;

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
  size_t bdx = i >> BDD_VEC_LG_SZ;
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
  size_t bdx = i >> BDD_VEC_LG_SZ;
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
BddImpl::makeNode(BddIndex index, BDD hi, BDD lo)
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
inline BddIndex
BddImpl::minIndex(const BDD f, const BDD g) const
{
  return std::min(index(f), index(g));
} // BddImpl::minIndex


//      Function : BddImpl::minIndex
//      Abstract : Return the minimum index of f, g and h.
inline BddIndex
BddImpl::minIndex(const BDD f, const BDD g, const BDD h) const
{
  return std::min(index(f), std::min(index(g), index(h)));
} // BddImpl::minIndex


//      Function : BddImpl::restrict1
//      Abstract : Restrict f to idx. Assumes index(f) >= idx.
inline BDD
BddImpl::restrict1(const BDD f, const BddIndex idx) const
{
  BddIndex myIdx = index(f);
  assert(myIdx >= idx);
  return (myIdx != idx) ? f : getXHi(f);
} // restrict1


//      Function : BddImpl::restrict0
//      Abstract : Restrict f to !idx. Assumes index(f) >= idx.
inline BDD
BddImpl::restrict0(BDD f, BddIndex idx) const
{
  BddIndex myIdx = index(f);
  assert(myIdx >= idx);
  return (myIdx != idx) ? f : getXLo(f);
} // restrict0


///////////////////////////////////////////////////////////////////////////////////////
// Simple hash functions.
//
inline uint32_t hash2(uint32_t a,
                      uint32_t b) {
  uint32_t rtn = ((a >> 1) ^ b ^ (a << 9) ^ (b << 5));
  return rtn;
} // hash2

inline uint32_t hash3(uint32_t a,
                      uint32_t b,
                      uint32_t c) {
  uint32_t rtn = ((a >> 5) ^
                  (b >> 2) ^
                  (c << 1) ^
                  (a << 4) ^
                  (b << 7) ^
                  (c << 10));

  return rtn;
} // hash3

} // namespace abide

#endif // BDDIMPL_H
