//
//      File     : BddImplMem.cc
//      Abstract : Memory management for BddImpl.
//

#include <BddImpl.h>
#include <cassert>
#include <cstring>
#include <iostream>

namespace abide {

//////////////////////////////////////////////////////////////////////////////
//
// Public Methods
//
//////////////////////////////////////////////////////////////////////////////

//      Function : BddImpl::gc
//      Abstract : Possibly perform a garbage collection. Return the number
//      collected nodes. Will not run if lock is set. Otherwise, runs
//      if force is set or the number of allocated nodes id greater
//      than the current trigger.
unsigned int
BddImpl::gc(bool force, bool verbose)
{
  unsigned int nodesFreed = 0;
  static int numGCs = 0;

  if (_gcLock > 0) {
    return 0;
  } // if

  if (force || _nodesAllocd > _gcTrigger) {
    ++numGCs;
    markReferencedNodes();
    cleanCaches(false);

    for (auto &tbl : _uniqTbls) {
      BDDVec nodes;
      tbl.clear(*this, nodes);
      for (auto f : nodes) {
        if (nodeMarked(f, 0)) {
          unmarkNode(f, 0);
          tbl.putHash(*this, f);
        } else {
          freeNode(f);
          ++nodesFreed;
        } // if marked or not
      } // for
    } // for each tbl
    if (_nodesAllocd > _gcTrigger) {
      _gcTrigger *= 2;
    } // increase trigger?

    assert(_nodesAllocd + _nodesFree == _curNodes);
    // assert(_nodesFree == countFreeNodes());

    if (verbose) {
      std::cout << "Garbage Collection #" << numGCs << ": "
           << _nodesAllocd << " : " << nodesFreed
           << std::endl;
    } // if
  } // if unlocked for gc

  return nodesFreed;
} // BddImpl::gc


//      Function : BddImpl::reorder
//      Abstract : Reorder variables using Rick Rudell's sifting
//      algorithm.
unsigned int
BddImpl::reorder(bool verbose)
{
  gc(true, false);
  lockGC();
  _reordering = true;

  bddCntMap refs;
  auto startSize = _nodesAllocd;

  if (verbose) {
    std::cout << "BDD REORDER: start size  = " << startSize << std::endl;
  } // if

  saveXRefs(refs);
  calcTRefs(refs);
  for (auto &tbl : _uniqTbls) {
    tbl.setProcessed(false);
  } // for

  for (auto index = getNextBddVar(); index > 0; index = getNextBddVar()) {
    UniqTbl &tbl = _uniqTbls[index];
    tbl.setProcessed(true);
    if (index < _maxIndex >> 1) {
      sift_udu(index);
    } else {
      sift_dud(index);
    } // choose shorter starting direction
    assert(_nodesAllocd <= startSize);
  } // for each var

  restoreXRefs(refs);

  _reordering = false;
  unlockGC();
  cleanCaches(true);

  if (verbose) {
    int saved = startSize - int(_nodesAllocd);
    std::cout << "BDD REORDER: end size    = " << _nodesAllocd << std::endl;
    std::cout << "BDD REORDER: saved nodes = " << saved << std::endl;
    assert(saved >= 0);
  } // if

  return startSize - _nodesAllocd;
} // reorder


//////////////////////////////////////////////////////////////////////////////
//
// Private Methods
//
//////////////////////////////////////////////////////////////////////////////


//      Function : BddImpl::allocateNode
//      Abstract : Allocate a BDD node if possible.
BDD
BddImpl::allocateNode()
{
  BDD rtn = _nullNode;

  if (_nodesAllocd >= _maxNodes &&
      !_reordering) {
    return rtn;
  } // if

  if (_nodesFree == 0) {
    assert(_freeList == 0);

    allocateMoreNodes();
  } // if no node free

  if (_nodesFree) {
    rtn = _freeList;
    BddNode &node(getNode(_freeList));
    _freeList = node.getNext();
    node.clear();
    ++_nodesAllocd;
    --_nodesFree;
    _maxAllocd = std::max(_maxAllocd, _nodesAllocd);
    assert(rtn/2 < _curNodes);
  } // if nodes free

  assert(_nodesAllocd + _nodesFree == _curNodes);
  // assert(_nodesFree == countFreeNodes());

  return rtn;
} // BddImpl::allocateNode


//      Function : BddImpl::allocateMoreNodes
//      Abstract : Free list is empty. Allocate more nodes and
//      add them to the free list.
void
BddImpl::allocateMoreNodes()
{
#ifdef BANKEDMEM
  if (_curNodes < _maxNodes) {
    unsigned int bdx = _banks.size();
    BddBank nuBank = new BddNode[BDD_VEC_SZ];
    if (nuBank) {
      _banks.push_back(nuBank);
      _freeList = bdx << (BDD_VEC_LG_SZ + 1);

      for (unsigned int idx = 1; idx < BDD_VEC_SZ; ++idx) {
        BddNode &node(nuBank[idx-1]);
        node.setNext(_freeList + 2*idx);
      } // for

      nuBank[BDD_VEC_SZ-1].setNext(0);

      _nodesFree = BDD_VEC_SZ;
      _curNodes += BDD_VEC_SZ;
      // assert(_nodesFree == countFreeNodes() || _nodesAllocd == 0);
    } // if allocated new bank of nodes
  } // if less than max
#else
  if (_curNodes < _maxNodes) {
    auto tgtSize = 2 * _curNodes;
    tgtSize = std::max(tgtSize, 1UL<<16);
    BddNode *nuNodes =
      static_cast<BddNode *>(realloc(_nodes, tgtSize * sizeof(BddNode)));
    if (nuNodes) {
      _nodes = nuNodes;
      _freeList = _curNodes<<1;
      BddNode *node = _nodes + _curNodes;
      for (auto idx = _curNodes+1; idx < tgtSize; ++idx) {
        node->clear();
        node->setNext(idx<<1);
        node = _nodes + idx;
      } // for
      node->clear();
      _nodesFree = tgtSize - _curNodes;
      _curNodes = tgtSize;
    } // if
  } // if less than max
#endif
} // BddImpl::allocateMoreNodes


//      Function : BddImpl::freeNode
//      Abstract : Put a node on the free list
void
BddImpl::freeNode(const BDD f)
{
  BddNode &node = getNode(f);
  node.clear();
  node.setNext(_freeList);
  _freeList = f;
  --_nodesAllocd;
  ++_nodesFree;
  // assert(_nodesFree == countFreeNodes());
} // BddImpl::freeNode


//      Function : BddImpl::findOrAddUniqTbl
//      Abstract : Abstract : Find a node in or add a node to the unique table.
BDD
BddImpl::findOrAddUniqTbl(const unsigned int index,
                          BDD hi,
                          BDD lo)
{
  assert(index != 0);
  assert(getIndex(hi) > index);
  assert(getIndex(lo) > index);
  BDD rtn = _nullNode;

  assert(index <= _maxIndex);

  bool inv = isNegPhase(hi);
  if (inv) {
    hi = invert(hi);
    lo = invert(lo);
  } // if need inverted node.

  UniqTbl &tbl = _uniqTbls[index];
  rtn = tbl.findOrAdd(*this, index, hi, lo);
  rtn = inv ? invert(rtn) : rtn ;

  return rtn;
} // BddImpl::findOrAddUniqTbl


//      Function : BddImpl::markReferencedNodes
//      Abstract : Mark all nodes that have a reference either
//      directly or indirectly.
void
BddImpl::markReferencedNodes()
{
  for (const auto &tbl : _uniqTbls) {
    for (unsigned int hdx = 0; hdx < tbl.size(); ++hdx) {
      BDD f = tbl.getHash(hdx);
      while (f) {
        if (numRefs(f) > 0) {
          markNodes(f, 0);
        } // if refs
        f = getNext(f);
      } // while nodes ro process
    } // for each hash
  } // for each tbl
} // BddImpl::markReferencedNodes


//      Function : BddImpl::markNodes
//      Abstract : Recursively mark nodes rooted at this node.
void
BddImpl::markNodes(const BDD f, unsigned int m) const
{
  if (f > 3) {
    BddNode &node = getNode(f);
    if (!node.marked(m)) {
      node.setMark(m);
      markNodes(node.getHi(), m);
      markNodes(node.getLo(), m);
    } // if unmarked
  } // if f is not the null, 1- or 0-node.
} // BddImpl::markNodes


//      Function : BddImpl::unmarkNodes
//      Abstract : Recursively unmark nodes rooted at this node.
void
BddImpl::unmarkNodes(const BDD f, unsigned int m) const
{
  BddNode &node = getNode(f);
  if (node.marked(m)) {
    node.clrMark(m);
    if (f > 3) {
      unmarkNodes(node.getHi(), m);
      unmarkNodes(node.getLo(), m);
    } // if f is not the null, 1- or 0-node.
  } // if marked
} // BddImpl::unmarkNodes


//      Function : BddImpl::getNextBddVar
//      Abstract : Find the unprocessed level with the most nodes.
unsigned int
BddImpl::getNextBddVar()
{
  unsigned int rtn = 0;
  unsigned int worst = 0;
  unsigned int idx = 0;
  for (const auto &tbl : _uniqTbls) {
    if (!tbl.processed() && tbl.numNodes() > worst) {
      worst = tbl.numNodes();
      rtn = idx;
    } // if
    ++idx;
  } // for

  return rtn;
} // BddImpl::getNextBddVar


//      Function : BddImpl::sift_udu
//      Abstract : Find the optimal place for this index by sifting
//      to the top, then to the bottom and back up to the minimal
//      position.
void
BddImpl::sift_udu(const unsigned int index)
{
  unsigned long startSz = _nodesAllocd;
  unsigned long maxSz = maxSize(startSz);
  unsigned int jdx = index;
  while (jdx > 1 && _nodesAllocd < maxSz) {
    exchange(--jdx);
  } // while not at top

  // Always do first exchange. The last one above may have blown the
  // maxSz. best and bestIndex depend on if this is a negative
  // (good) delta or not.
  long delta = exchange(jdx++);
  long best = delta < 0 ? delta : 0;
  unsigned int bestIndex = delta < 0 ? jdx : jdx-1;

  // Move to the bottom and record best position. The best position
  // is the one with the mode negative delta.
  while (jdx < _maxIndex && _nodesAllocd < maxSz) {
    delta += exchange(jdx++);
    if (delta < best) {
      best = delta;
      bestIndex = jdx;
    } // if
  } // while not at bottom and not too big
  assert(best <= 0);
  // Go to best position from here.
  while (bestIndex < jdx) {
    exchange(--jdx);
  } // while

  // Update_var2index.
  _var2Index.clear();
  for (unsigned int idx = 0; idx < _index2BddVar.size(); ++idx) {
    _var2Index[_index2BddVar[idx]] = idx;
  } // for
} // BddImpl::sift_udu


//      Function : BddImpl::sift_dud
//      Abstract : Find the optimal place for this index by sifting
//      to the top, then to the bottom and back up to the minimal
//      position.
void
BddImpl::sift_dud(const unsigned int index)
{
  unsigned long startSz = _nodesAllocd;
  unsigned long maxSz = maxSize(startSz);
  unsigned int jdx = index;
  while (jdx < _maxIndex && _nodesAllocd < maxSz) {
    exchange(jdx++);
  } // while not at top

  // Always do first exchange. The last one above may have blown the
  // maxSz. best and bestIndex depend on if this is a negative
  // (good) delta or not.
  long delta = exchange(--jdx);
  long best = delta < 0 ? delta : 0;
  unsigned int bestIndex = delta < 0 ? jdx : jdx+1;

  // Move to the top and record best position. The best position
  // is the one with the mode negative delta.
  while (jdx > 1 && _nodesAllocd < maxSz) {
    delta += exchange(--jdx);
    if (delta <= best) {
      best = delta;
      bestIndex = jdx;
    } // if
  } // while not at top and not too big
  assert(best <= 0);
  // Go to best position from here.
  while (bestIndex > jdx) {
    exchange(jdx++);
  } // while

  // Update_var2index/
  for (unsigned int idx = 0; idx < _var2Index.size(); ++idx) {
    _var2Index[_index2BddVar[idx]] = idx;
  } // for
} // BddImpl::sift_dud


//      Function : BddImpl::exchange
//      Abstract : Exchange index with index+1. Return the delta in nodes.
long
BddImpl::exchange(const unsigned int index)
{
  std::swap(_index2BddVar[index], _index2BddVar[index+1]);
  UniqTbl &tbl1 = _uniqTbls[index];
  UniqTbl &tbl2 = _uniqTbls[index+1];
  long startSz = tbl1.numNodes() + tbl2.numNodes();

  BDDVec x1, x2;
  tbl1.clear(*this, x1);
  tbl2.clear(*this, x2);

  bool tmp = tbl1.processed();
  tbl1.setProcessed(tbl2.processed());
  tbl2.setProcessed(tmp);

  // To understand why demote, swap and promote must be in this
  // order, work out the example of f = a*c+b*d starting with the
  // order (a,b,c,d).
  demote(x1, index);
  swapCofactors(x1, index);
  promote(x2, index);

  long endSz = tbl1.numNodes() + tbl2.numNodes();
  return endSz - startSz;
} // BddImpl::exchange


//      Function : BddImpl::demote
//      Abstract : If both children of a node in the node vector
//      have indices greater than idx+1, then make the node's index
//      idx+1.
void
BddImpl::demote(const BDDVec &nodes, const unsigned int idx)
{
  UniqTbl &tbl = _uniqTbls[idx+1];
  for (const auto f : nodes) {
    BddNode &node = getNode(f);
    BDD f1 = node.getHi();
    if (getIndex(f1) > idx+1) {
      BDD f0 = node.getLo();
      if (getIndex(f0) > idx+1) {
        node.setIndex(idx+1);
        unsigned int h = hash2(f1, f0) & tbl.getMask();
        tbl.putHash(*this, f, h);
      } // if f0
    } // if f1
  } // for each node in level
} // BddImpl::demote


//      Function : BddImpl::swapCofactors
//      Abstract : Swap the f10 and f01 cofactors of each node int
//      the vector. This causes the variables at the current index
//      and the next index to be swapped. Also, decrement the ref
//      counts of f1 and f0. If they become zero, then they will be
//      freed by the promote function.
void
BddImpl::swapCofactors(const BDDVec &nodes, const unsigned int idx)
{
  UniqTbl &tbl1 = _uniqTbls[idx];
  for (const auto f : nodes) {
    BddNode *node = getNodePtr(f);
    // if node was not demoted
    if (node->getIndex() == idx) {
      BDD f1 = node->getHi();
      BDD f0 = node->getLo();
      decTRefs(f1);
      decTRefs(f0);
      BDD f11, f10, f01, f00;
      if (getIndex(f1) == idx+1) {
        f11 = getHi(f1);
        f10 = getLo(f1);
      } else {
        f11 = f10 = f1;
      } // if

      if (getIndex(f0) == idx+1) {
        f01 = getXHi(f0);
        f00 = getXLo(f0);
      } else {
        f01 = f00 = f0;
      } // if

      if (f11 != f01) {
        f1 = findOrAddUniqTbl(idx+1, f11, f01);
#ifndef BANKEDMEM
        // There may have been a realloc during findOrAddUniqTbl().
        node = getNodePtr(f);
#endif
      } else {
        f1 = f11;
      } // if
      incTRefs(f1);
      node->setHi(f1);

      if (f10 != f00) {
        f0 = findOrAddUniqTbl(idx+1, f10, f00);
#ifndef BANKEDMEM
        // There may have been a realloc during findOrAddUniqTbl().
        node = getNodePtr(f);
#endif
      } else {
        f0 = f00;
      } // if
      incTRefs(f0);
      node->setLo(f0);

      tbl1.putHash(*this, f);
    } // if not demoted
  } // for each node
} // BddImpl::swapCofactors


//      Function : BddImpl::promote
//      Abstract : For each node int the vector, if it has
//      references, then change its index from i to i+1.  Otw, free
//      the node.
void
BddImpl::promote(const BDDVec &nodes, const unsigned int idx)
{
  UniqTbl &tbl = _uniqTbls[idx];
  for (const auto f : nodes) {
    if (numRefs(f)) {
      BddNode &node = getNode(f);
      node.setIndex(idx);
      tbl.putHash(*this, f);
    } else {
      freeNode(f);
    } // if
  } // for each node in level
} // BddImpl::promote


//      Function : BddImpl::saveXRefs
//      Abstract : Walks all nodes and record the ones with external
//      references.
void
BddImpl::saveXRefs(bddCntMap &refs)
{
  for (const auto &tbl : _uniqTbls) {
    for (unsigned int hdx = 0; hdx < tbl.size(); ++hdx) {
      BDD f = tbl.getHash(hdx);
      while (f) {
        if (numRefs(f) > 0) {
          refs[f] = numRefs(f);
          setRefs(f, 0);
        } // if refs
        f = getNext(f);
      } // while nodes ro process
    } // for each hash
  } // for each tbl
} // BddImpl::saveXRefs


//      Function : BddImpl::calcTRefs
//      Abstract : Use the (cleared) external refs field to store
//      the total number of refereneces to each node. (This is not a
//      true total as all external references are counted as one for
//      each node.
void
BddImpl::calcTRefs(const bddCntMap &refs)
{
  for (const auto &it : refs) {
    BDD f = it.first;
    calcTRefs(f);
  } //  for each stored external reference
} // BddImpl::calcTRefs


//      Function : BddImpl::restoreXRefs
//      Abstract : Restore the original number external references
//      of nodes.
void
BddImpl::restoreXRefs(bddCntMap &refs)
{
  for (const auto &tbl : _uniqTbls) {
    for (unsigned int hdx = 0; hdx < tbl.size(); ++hdx) {
      BDD f = tbl.getHash(hdx);
      while (f) {
        setRefs(f, (refs.count(f) ? refs[f] : 0));
        f = getNext(f);
      } // while nodes ro process
    } // for each hash
  } // for each tbl
} // restoreXRefs


//      Function : BddImpl::calcTRefs
//      Abstract : Recursively calculate trefs from this node.
void
BddImpl::calcTRefs(const BDD f)
{
  if (f > 3) {
    BddNode &node = getNode(f);
    if (node.numRefs() == 0) {
      calcTRefs(node.getHi());
      calcTRefs(node.getLo());
    } // if first visit
    node.incRef();
  } // if non-constant
} // BddImpl::calcTRefs


//      Function : BddImpl::decTRefs
//      Abstract : Recursively decrement trefs from this node.
void
BddImpl::decTRefs(const BDD f)
{
  if (f > 3) {
    BddNode &node = getNode(f);
    node.decRef();
    if (node.numRefs() == 0) {
      decTRefs(node.getHi());
      decTRefs(node.getLo());
    } // if first visit
  } // if non-constant
} // BddImpl::decTRefs


//      Function : BddImpl::incTRefs
//      Abstract : Recursively increment trefs from this node.
void
BddImpl::incTRefs(const BDD f)
{
  if (f > 3) {
    BddNode &node = getNode(f);
    if (node.numRefs() == 0) {
      incTRefs(node.getHi());
      incTRefs(node.getLo());
    } // if first visit
    node.incRef();
  } // if non-constant
} // BddImpl::incTRefs


//      Function : BddImpl::checkMem
//      Abstract : Print out some basic checks.
bool
BddImpl::checkMem() const
{
  std::printf("\t------------------------\n");
  std::printf("\tmax allocated = %ld\n", _maxAllocd);
  std::printf("\tnodes allocated = %ld\n", _nodesAllocd);
  std::printf("\tnodes free = %ld\n", _nodesFree);
  std::printf("\tnodes in free list = %d\n", countFreeNodes());
  std::printf("\tnodes in mem = %ld\n", _curNodes);

  return ((_nodesFree + _nodesAllocd) == _curNodes);
} // checkMem


//      Function : BddImpl::setRefs
//      Abstract : Set the number of external references of f.
void
BddImpl::setRefs(const BDD f, const unsigned int n) const
{
  BddNode &node = getNode(f);
  node.setRefs(n);
} // numRefs


//      Function : BddImpl::incRef
//      Abstract : Increment the ref count of f.
void
BddImpl::incRef(BDD f) const
{
  if (f && notConstant(f)) {
    BddNode &n = getNode(f);
    n.incRef();
  } // if
} // BddImpl::incRef

//      Function : BddImpl::decRef
//      Abstract : Decrement the ref count of f.
void
BddImpl::decRef(BDD f) const
{
  if (f && notConstant(f)) {
    BddNode &n = getNode(f);
    n.decRef();
  } //if
} // decRef


//      Function : BddImpl::numRefs
//      Abstract : Get the number of references to this node.
unsigned int
BddImpl::numRefs(BDD f) const
{
  BddNode &n = getNode(f);
  return n.numRefs();
} // BddImpl::numRefs


//      Function : BddImpl::countFreeNodes
//      Abstract : Count the nodes in the free list.
unsigned int
BddImpl::countFreeNodes() const
{
  unsigned int cnt = 0;
  BDD b = _freeList;
  while (b) {
    ++cnt;
    b = getNext(b);
  } // while
  return cnt;
} // BddImpl::countFreeNodes

} // namespace abide
