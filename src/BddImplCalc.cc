//
//      File     : BddImplCalc.cc
//      Abstract : Implementation of BDD calclulations.
//

#include <BddImpl.h>
#include <cassert>
#include <algorithm>

namespace abide {

//      Function : BddImpl::apply
//      Abstract : Apply OP to F and G with retry after gc if null..
BDD
BddImpl::apply(BDD f, BDD g, BddOp op)
{
  BDD rtn = apply2(f, g, op);
  if (rtn == _nullNode && _gcLock == 0) {
    gc(true, false);
    rtn = apply2(f, g, op);
  } // if

  return rtn;
} // BddImpl::apply


//      Function : BddImpl::apply2
//      Abstract : Apply OP to F and G.
BDD
BddImpl::apply2(BDD f, BDD g, BddOp op)
{
  BDD r = _nullNode;
  switch (op) {
   case AND:
    r = and2(f, g);
    break;
   case NAND:
    r = nand2(f, g);
    break;
   case OR:
    r = or2(f, g);
    break;
   case NOR:
    r = nor2(f, g);
    break;
   case XOR:
    r = xor2(f, g);
    break;
   case XNOR:
    r = xnor2(f, g);
    break;
   case IMPL:
    r = impl2(f, g);
    break;
   default:
    assert(false);
  } // switch

  return r;
} // BddImpl::apply2

//      Function : BddImpl::restrict
//      Abstract : Computes the cofactor of f w.r.t. c.
BDD
BddImpl::restrict(BDD f, BDD c)
{
  BDD rtn = restrictRec(f, c);
  if (rtn == _nullNode && _gcLock == 0) {
    gc(true, false);
    rtn = restrictRec(f, c);
  } // if

  return rtn;
} // BddImpl::restrict


//      Function : BddImpl::compose
//      Abstract : Replace variable x with function g in function f.
BDD
BddImpl::compose(BDD f, BddVar x, BDD g)
{
  BDD rtn = _nullNode;

  lockGC();
  BDD poslit = getLit(x);
  BDD neglit = getLit(-x);
  if (BDD f1 = restrict(f, poslit);
      f1) {
    if (BDD f0 = restrict(f, neglit);
        f0) {
      rtn = ite(g, f1, f0);
    } // if f0;
  } // if f1
  unlockGC();

  if (rtn == _nullNode && _gcLock == 0) {
    gc(true, false);
    lockGC();
    if (BDD f1 = restrict(f, poslit);
        f1) {
      if (BDD f0 = restrict(f, neglit);
          f0) {
        rtn = ite(g, f1, f0);
      } // if f0;
    } // if f1
    unlockGC();
  } // if

  return rtn;
} // BddImpl::compose


//      Function : BddImpl::andExists
//      Abstract : Compute the relational product of f and g w.r.t. c.
BDD
BddImpl::andExists(const BDD f, const BDD g, const BDD c)
{
  lockGC();
  BDD rtn = andExists2(f, g, c);
  unlockGC();

  if (rtn == _nullNode && _gcLock == 0) {
    gc(true, false);
    rtn = andExists2(f, g, c);
  } // if

  return rtn;
} // BddImpl::andExists


//      Function : BddImpl::andExists2
//      Abstract : Recursive step.
BDD
BddImpl::andExists2(BDD f, BDD g, BDD c)
{
  orderOps(f, g);

  BDD rtn = andExistsTerminal(f, g, c);

  if (rtn) {
    return rtn;
  } // if

  rtn = getAndExistsCache(f, g, c);
  if (!rtn) {
    _cacheStats.incCompMiss();
    unsigned int index = minIndex(f, g);
    unsigned int cdx = getIndex(c);
    while (cdx < index) {
      c = getHi(c);
      cdx = getIndex(c);
    } // while

    if (BDD lo = andExists2(restrict0(f, index),
                            restrict0(g, index),
                            restrict1(c, index));
        lo) {
      if (index == cdx && isOne(lo)) {
        rtn = _oneNode;
      } else if (BDD hi = andExists2(restrict1(f, index),
                                     restrict1(g, index),
                                     restrict1(c, index));
                 hi) {
        rtn = (index == cdx)
          ? or2(lo, hi)
          : makeNode(index, hi, lo);
        insertAndExistsCache(f, g, c, rtn);
      } // if lo is one
    } // if lo computed
  } else {
    _cacheStats.incCompHit();
  } // if

  return rtn;
} // BddImpl::andExists2


//      Function : BddImpl::andExistsTerminal
//      Abstract : Check for terminating condition.
BDD
BddImpl::andExistsTerminal(BDD f, BDD g, BDD c)
{
  if (isOne(c)) {
    return and2(f, g);
  } else if (isZero(f)) {
    return _zeroNode;
  } else if (f == invert(g)) {
    return _zeroNode;
  } // if

  return _nullNode;
} // BddImpl::andExistsTerminal


//      Function : BddImpl::covers
//      Abstract : Return true if f covers g.
bool
BddImpl::covers(BDD f, BDD g)
{
  BDD val = andConstant(invert(f), g);
  return isZero(val);
} // BddImpl::covers


//      Function : BddImpl::cubeFactor
//      Abstract : Computes the cube factor with the most
//      literals. N.B.: This function does not have an automatic retry
//      if the result is the null BDD.
BDD
BddImpl::cubeFactor(BDD f)
{
  if (isConstant(f)) {
    return f;
  } // if

  FnSet fns{f};
  BddIndexVec indices;
  for (const auto &var : supportVec(f)) {
    indices.push_back(_var2Index[var]);
  } // for
  std::reverse(indices.begin(), indices.end());

  BDD rtn = cubeFactor(indices, fns);

  return rtn;
} // BddImpl::cubeFactor


//      Function : BddImpl::cubeFactor
//      Abstract : Recursive step.
BDD
BddImpl::cubeFactor(BddIndexVec &indices, const FnSet &fns)
{
  BDD rtn = _oneNode;

  if (indices.size()) {
    BddIndex index = indices.back();
    indices.pop_back();
    Unateness unateness = getUnateness(index, fns);
    FnSet nuSet = expandFnSet(index, fns);
    rtn = cubeFactor(indices, nuSet);

    switch (unateness) {
      case POS:
        rtn = makeNode(index, rtn, _zeroNode);
        break;
      case NEG:
        rtn = makeNode(index, _zeroNode, rtn);
      break;
      case BINATE:
        break;
    } // switch
  } // if

  return rtn;
} // BddImpl::cubeFactor


//      Function : BddImpl::getUnateness
//      Abstract : Return the unateness of the function set w.r.t. the
//      variable. The variable is always the top variable of the set.
BddImpl::Unateness
BddImpl::getUnateness(const BddIndex idx, const FnSet &fns)
{
  bool isPos = true;
  bool isNeg = true;

  for (const auto &f : fns) {
    if (isOne(f)) {
      return BINATE;
    } else if (! isZero(f)) {
      if (getIndex(f) != idx) {
        return BINATE;
      } // if
      if (! isZero(getXLo(f))) {
        isPos = false;
      } // if
      if (! isZero(getXHi(f))) {
        isNeg = false;
      } // if
    } // if
  } // for

  assert(!(isPos && isNeg));

  return (isPos ? POS :
          isNeg ? NEG :
          BINATE);
} // BddImpl::getUnateness


//      Function : BddImpl::expandFnSet
//      Abstract : Expand the function set with all cofactors w.r.t. index.
BddImpl::FnSet
BddImpl::expandFnSet(const BddIndex index, const FnSet &fns)
{
  FnSet rtn;
  for (const auto &f : fns) {
    if (index == getIndex(f)) {
      rtn.insert(getXHi(f));
      rtn.insert(getXLo(f));
    } else {
      rtn.insert(f);
    } // if
  } // for

  return rtn;
} // BddImpl::expandFnSet


//      Function : BddImpl::supportVec
//      Abstract : Return the support of f as a vector of variables.
BddVarVec
BddImpl::supportVec(BDD f)
{
  BDD s = supportCubeRec(f);
  unmarkNodes(f, 1);
  BddVarVec rtn;
  while (! isConstant(s)) {
    rtn.push_back(getBddVar(s));
    s = getHi(s);
  } // while lits left
  return rtn;
} // BddImpl::supportVec


//      Function : BddImpl::supportCube
//      Abstract : Return the support of f as a vector of
//      variables. N.B.: This function does not have an automatic
//      retry if the result is the null BDD.
BDD
BddImpl::supportCube(BDD f)
{
  BDD rtn = supportCubeRec(f);
  unmarkNodes(f, 1);
  return rtn;
} // BddImpl::supportCube


//      Function : BddImpl::oneCube
//      Abstract : Return a satisfying cube if one exists. zero otherwise.
BDD
BddImpl::oneCube(BDD f)
{
  BDD rtn = _nullNode;
  if (isConstant(f)) {
    return f;
  } else {
    BddVar x = getBddVar(f);
    BDD hi = oneCube(getXHi(f));
    if (isZero(hi)) {
      BDD lo = oneCube(getXLo(f));
      rtn = makeNode(x, _zeroNode, lo);
    } else {
      rtn = makeNode(x, hi, _zeroNode);
    } // if
  } // if

  return rtn;
} // BddImpl::oneCube


//      Function : BddImpl::and2
//      Abstract : Computes f*g.
BDD
BddImpl::and2(BDD f, BDD g)
{
  orderOps(f, g);

  // Terminal cases.
  if (isOne(f)) {
    return g;
  } else if (isZero(f)) {
    return _zeroNode;
  } else if (f == g) {
    return f;
  } else if (f == invert(g)) {
    return _zeroNode;
  } // if

  BDD rtn = getAndCache(f, g);
  if (!rtn) {
    _cacheStats.incCompMiss();
    unsigned int index = minIndex(f, g);
    if (BDD hi = and2(restrict1(f, index),
                      restrict1(g, index));
        hi) {
      if (BDD lo = and2(restrict0(f, index),
                        restrict0(g, index));
          lo) {
        rtn = makeNode(index, hi, lo);
        insertAndCache(f, g, rtn);
      } // if lo
    } // if hi
  } else {
    _cacheStats.incCompHit();
  } // if

  return rtn;
} // BddImpl::and2


//      Function : BddImpl::xor2
//      Abstract : Computes f^g.
BDD
BddImpl::xor2(BDD f, BDD g)
{
  orderOps(f, g);

  // Terminal cases.
  if (isOne(f)) {
    return invert(g);
  } else if (isZero(f)) {
    return g;
  } else if (f == g) {
    return _zeroNode;
  } else if (f == invert(g)) {
    return _oneNode;
  } // if

  BDD rtn = getXorCache(f, g);
  if (!rtn) {
    _cacheStats.incCompMiss();
    unsigned int index = minIndex(f, g);
    if (BDD hi = xor2(restrict1(f, index),
                      restrict1(g, index));
        hi) {
      if (BDD lo = xor2(restrict0(f, index),
                        restrict0(g, index));
          lo) {
        rtn = makeNode(index, hi, lo);
        insertXorCache(f, g, rtn);
      } // if lo
    } // if hi
  } else {
    _cacheStats.incCompHit();
  } // if

  return rtn;
} // BddImpl::xor2


//      Function : BddImpl::andConstant
//      Abstract : Like and2(), but only returns constant 1, 0 or null
//      node if not constant.
BDD
BddImpl::andConstant(BDD f, BDD g)
{
  orderOps(f, g);

  BDD rtn;

  if (andConstantTerminal(f, g, rtn)) {
    return rtn;
  } // if

  rtn = getAndCache(f, g);
  if (!rtn) {
    _cacheStats.incCompMiss();
    unsigned int index = minIndex(f, g);
    if (BDD hi = andConstant(restrict1(f, index),
                             restrict1(g, index));
        isConstant(hi)) {
      if (BDD lo = andConstant(restrict0(f, index),
                               restrict0(g, index));
          isConstant(lo)) {
        if (hi == lo) {
          insertAndCache(f, g, hi);
          return hi;
        } else {
          return _nullNode;
        } // if
      } // if lo
    } // if hi
  } else {
    _cacheStats.incCompHit();
    rtn = isConstant(rtn) ? rtn : _nullNode;
  } // if

  return rtn;
} // BddImpl::andConstant


//      Function : BddImpl::andConstantTerminal
//      Abstract :
bool
BddImpl::andConstantTerminal(BDD f, BDD g, BDD &rtn)
{
  // Terminal cases.
  if (isZero(f) || isZero(g)) {
    rtn = _zeroNode;
    return true;
  } else if (isOne(f)) {
    rtn = (isOne(g)) ? _oneNode : _nullNode;
    return true;
  } else if (f == invert(g)) {
    rtn = _zeroNode;
    return true;
  } else if (f == g) {
    rtn = _nullNode;
    return true;
  } // if

  return false;
} // BddImpl::andConstantTerminal


//      Function : BddImpl::ite
//      Abstract : Computes if f then g else h.
BDD
BddImpl::ite(BDD f, BDD g, BDD h)
{
  assert(f >= 2);
  assert(g >= 2);
  assert(h >= 2);
  bool inv = stdTrip(f, g, h);
  BDD rtn = _nullNode;

  if (isOne(f)) {
    rtn = g;
  } else if (g == h) {
    rtn = g;
  } else if (isOne(g) && isZero(h)) {
    rtn = f;
  } else {
    rtn = getIteCache(f, g, h);
    if (! rtn) {
      _cacheStats.incCompMiss();
      unsigned int index = minIndex(f, g, h);
      BDD hi = ite(restrict1(f, index),
                   restrict1(g, index),
                   restrict1(h, index));
      if (hi) {
        BDD lo = ite(restrict0(f, index),
                     restrict0(g, index),
                     restrict0(h, index));
        if (lo) {
          rtn = makeNode(index, hi, lo);
          insertIteCache(f, g, h, rtn);
        } // if valid else node
      } // if valid then node
    } else {
      _cacheStats.incCompHit();
    } // if not in cache
  } // test for terminal cases

  if (rtn && inv) {
    rtn = invert(rtn);
  } // invert?

  return rtn;
} // BddImpl::ite


//      Function : BddImpl::stdTrip
//      Abstract : Standardize the ite triple among equivalent
//      forms. Returns true if the standardized for produces the
//      inverse.
bool
BddImpl::stdTrip(BDD &f, BDD &g, BDD &h)
{
  reduceThenElse(f, g, h);
  swapArgs(f, g, h);
  return stdNegation(f, g, h);
} // BddImpl::stdTrip


//      Function : BddImpl::reduceThenElse
//      Abstract : Reduce g and h to constants if possible.
void
BddImpl::reduceThenElse(BDD &f, BDD &g, BDD &h)
{
  if (f == g) {
    g = _oneNode;
  } else if (f == invert(g)) {
    g = _zeroNode;
  } else if (f == h) {
    h = _zeroNode;
  } else if (f == invert(h)) {
    h = _oneNode;
  } // if g or h can be a constant
} // BddImpl::reduceThenElse


//      Function : BddImpl::swapArgs
//      Abstract : Perform swaps if possible.

void
BddImpl::swapArgs(BDD &f, BDD &g, BDD &h)
{
  if (isOne(g)) {
    condSwap(f, h);
  } else if (isZero(h)) {
    condSwap(f, g);
  } else if (isOne(h)) {
    condSwapNeg(f, g);
  } else if (isZero(g)) {
    condSwapNeg(f, h);
  } else if (g == invert(h)) {
    if (index(f) > index(g)) {
      std::swap(f, g);
      h = invert(g);
    } // if
  } // if
} // BddImpl::swapArgs

void BddImpl::condSwap(BDD &f, BDD &g){
  if (index(f) > index(g)) {
    std::swap(f, g);
  } // if
}

void BddImpl::condSwapNeg(BDD &f, BDD &g){
  if (index(f) > index(g)) {
    std::swap(f, g);
    f = invert(f);
    g = invert(g);
  } // if
}

//      Function : BddImpl::stdNegation
//      Abstract : Standardize negations.
bool
BddImpl::stdNegation(BDD &f, BDD &g, BDD &h)
{
  bool inv = false;
  if (isNegPhase(f)) {
    if (isNegPhase(h)) {
      f = invert(f);
      g = invert(g);
      h = invert(h);
      std::swap(g, h);
      inv = true;
    } else {
      f = invert(f);
      std::swap(g, h);
    } // if h inverted
  } else {
    if (isNegPhase(g)) {
      g = invert(g);
      h = invert(h);
      inv = true;
    } // if
  } // if f inverted

  return inv;
} // BddImpl::stdNegation


//      Function : BddImpl::restrictRec
//      Abstract : Recursive worker function for restrict().
BDD
BddImpl::restrictRec(BDD f, BDD c)
{
  BDD rtn = restrictTerminal(f, c);

  if (rtn) {
    return rtn;
  } // if

  rtn = getRestrictCache(f, c);
  if (! rtn) {
    _cacheStats.incCompMiss();
    unsigned int fdx = getIndex(f);
    c = reduce(c, fdx);
    BDD c1 = restrict1(c, fdx);
    BDD c0 = restrict0(c, fdx);

    if (isZero(c1)) {
      rtn = restrictRec(getXLo(f), c0);
    } else if (isZero(c0)) {
      rtn = restrictRec(getXHi(f), c1);
    } else {
      if (BDD r1 = restrictRec(getXHi(f), c);
          r1) {
        if (BDD r0 = restrictRec(getXLo(f), c);
            r0) {
          rtn = makeNode(fdx, r1, r0);
        } // if r0
      } // if r1
    } // if cube literal
    insertRestrictCache(f, c, rtn);
  } else {
    _cacheStats.incCompHit();
  } //if

  return rtn;
} // BddImpl::restrictRec


//      Function : BddImpl::restrictTerminal
//      Abstract : Return the result if this is a terminal case.
BDD
BddImpl::restrictTerminal(BDD f, BDD c)
{
  BDD rtn = _nullNode;
  if (isOne(c) || isConstant(f)) {
    rtn = f;
  } else if (f == c) {
    rtn = _oneNode;
  } else if (f == invert(c)) {
    rtn = _zeroNode;
  } // if

  return rtn;
} // BddImpl::restrictTerminal


//      Function : BddImpl::reduce
//      Abstract : While the top variable of c is greater than tgt,
//      perform or-smoothing on it.
BDD
BddImpl::reduce(BDD f, unsigned int tgt)
{
  unsigned int idx = getIndex(f);
  while (idx < tgt) {
    unsigned int f1 = getXHi(f);
    unsigned int f0 = getXLo(f);
    f = apply2(f1, f0, OR);
    idx = getIndex(f);
  } // while

  return f;
} // BddImpl::reduce


//      Function : BddImpl::supportCubeRec
//      Abstract : Recursive function for finding function
//      support as a cube.
BDD
BddImpl::supportCubeRec(const BDD f)
{
  BDD rtn = _nullNode;
  if (isConstant(f) || nodeMarked(f, 1)) {
    rtn = _oneNode;
  } else {
    markNode(f, 1);
    BDD s1 = supportCube(getHi(f));
    if (s1) {
      BDD s0 = supportCube(getLo(f));
      if (s0) {
        rtn = apply(s1, s0, AND);
        rtn = makeNode(getIndex(f), rtn, _zeroNode);
      } // if s0
    } // if s1
  } // if terminal or not

  return rtn;
} // BddImpl::supportCubeRec


//      Function : BddImpl::countNodes
//      Abstract : Count the number of nodes rooted at this node. Uses
//      mark 1 to record previousy visited nodes.
unsigned int
BddImpl::countNodes(const BDD f) const
{
  unsigned int count = 0;
  if (nodeUnmarked(f, 1)) {
    markNode(f, 1);
    if (! isConstant(f)) {
      count = (countNodes(getHi(f)) +
               countNodes(getLo(f)) + 1);
    } else {
      count = 1;
    } // if not constant
  } // if unmarked

  return count;
} // BddImpl::countNodes

//      Function : BddImpl::index
//      Abstract : Get the index of this BDD.
unsigned int
BddImpl::index(const BDD f) const
{
  BddNode &n = getNode(f);
  return n.getIndex();
} // BddImpl::index


//      Function : BddImpl::getAndCache
//      Abstract : Retrieves an entry from the AND cache if it is there.
BDD
BddImpl::getAndCache(BDD f, BDD g)
{
  BDD rtn = _nullNode;
  auto hash = hash2(f, g) & _compCacheMask;
  CacheData2 &c = _andTbl[hash];
  if (c._f == f && c._g == g) {
    rtn = c._r;
  } // if

  return rtn;
} // BddImpl::getAndCache


//      Function : BddImpl::insertAndCache
//      Abstract : Inserts a result into the AND cache.
void
BddImpl::insertAndCache(BDD f, BDD g, BDD r)
{
  if (r) {
    auto hash = hash2(f, g) & _compCacheMask;
    CacheData2 &c = _andTbl[hash];
    c._f = f;
    c._g = g;
    c._r = r;
  } // if
} // BddImpl::insertAndCache


//      Function : BddImpl::getXorCache
//      Abstract : Retrieves an entry from the XOR cache if it is there.
BDD
BddImpl::getXorCache(BDD f, BDD g)
{
  BDD rtn = _nullNode;
  auto hash = hash2(f, g) & _compCacheMask;
  CacheData2 &c = _xorTbl[hash];
  if (c._f == f && c._g == g) {
    rtn = c._r;
  } // if

  return rtn;
} // BddImpl::getXorCache


//      Function : BddImpl::insertXorCache
//      Abstract : Inserts a result into the XOR cache.
void
BddImpl::insertXorCache(BDD f, BDD g, BDD r)
{
  if (r) {
    auto hash = hash2(f, g) & _compCacheMask;
    CacheData2 &c = _xorTbl[hash];
    c._f = f;
    c._g = g;
    c._r = r;
  } // if
} // BddImpl::insertXorCache


//      Function : BddImpl::getRestrictCache
//      Abstract : Retrieves an entry from the restrict cache if it is
//      there.
BDD
BddImpl::getRestrictCache(BDD f, BDD g)
{
  BDD rtn = _nullNode;
  auto hash = hash2(f, g) & _compCacheMask;
  CacheData2 &c = _restrictTbl[hash];
  if (c._f == f && c._g == g) {
    rtn = c._r;
  } // if

  return rtn;
} // BddImpl::getRestrictCache


//      Function : BddImpl::insertRestrictCache
//      Abstract : Inserts a result into the restrict cache.
void
BddImpl::insertRestrictCache(BDD f, BDD g, BDD r)
{
  if (r) {
    auto hash = hash2(f, g) & _compCacheMask;
    CacheData2 &c = _restrictTbl[hash];
    c._f = f;
    c._g = g;
    c._r = r;
  } // if
} // BddImpl::insertRestrictCache


//      Function : BddImpl::getIteCache
//      Abstract : Looks in the computed cache to see if the
//      ite(f,g,h) has been saved.
BDD
BddImpl::getIteCache(const BDD f,
                     const BDD g,
                     const BDD h)
{
  BDD rtn = _nullNode;
  auto hash = hash3(f, g, h) & _compCacheMask;
  CacheData3 &c = _iteTbl[hash];
  if (c._f == f && c._g == g && c._h == h) {
    rtn = c._r;
  } // if

  return rtn;
} // BddImpl::getIteCache


//      Function : BddImpl::insertIteCache
//      Abstract : Inserts r into the computed table as being the
//      result of ite(f,g,h).
void
BddImpl::insertIteCache(const BDD f,
                        const BDD g,
                        const BDD h,
                        const BDD r)
{
  if (r) {
    auto hash = hash3(f, g, h) & _compCacheMask;
    CacheData3 &c = _iteTbl[hash];
    c._f = f;
    c._g = g;
    c._h = h;
    c._r = r;
  } // if r
} // BddImpl::insertIteCache


//      Function : BddImpl::getAndExistsCache
//      Abstract : Looks in the computed cache to see if the
//      andExists(f,g,h) has been saved.
BDD
BddImpl::getAndExistsCache(const BDD f,
                           const BDD g,
                           const BDD h)
{
  BDD rtn = _nullNode;
  auto hash = hash3(f, g, h) & _compCacheMask;
  CacheData3 &c = _andExistTbl[hash];
  if (c._f == f && c._g == g && c._h == h) {
    rtn = c._r;
  } // if

  return rtn;
} // BddImpl::getAndExistsCache


//      Function : BddImpl::insertAndExistsCache
//      Abstract : Inserts r into the computed table as being the
//      result of ite(f,g,h).
void
BddImpl::insertAndExistsCache(const BDD f,
                              const BDD g,
                              const BDD h,
                              const BDD r)
{
  if (r) {
    auto hash = hash3(f, g, h) & _compCacheMask;
    CacheData3 &c = _andExistTbl[hash];
    c._f = f;
    c._g = g;
    c._h = h;
    c._r = r;
  } // if r
} // BddImpl::insertAndExistsCache


//      Function : BddImpl::cleanCaches
//      Abstract : Remove cache entries with unreferenced nodes. Used
//      after garbage collection and reordering.
void
BddImpl::cleanCaches(const bool force)
{
  cleanCache(_andTbl, force);
  cleanCache(_xorTbl, force);
  cleanCache(_restrictTbl, force);
  cleanCache(_iteTbl, force);
  cleanCache(_andExistTbl, force);
} // BddImpl::cleanCaches


//      Function : BddImpl::cleanCache
//      Abstract : Clean a computed cache.
void
BddImpl::cleanCache(ComputedTbl2 &table, bool force)
{
  for (auto &data : table) {
    bool umF = nodeUnmarked(data._f, 0);
    bool umG = nodeUnmarked(data._g, 0);
    bool umR = nodeUnmarked(data._r, 0);
    if (force || umF || umG || umR) {
      data._f = _nullNode;
      data._g = _nullNode;
      data._r = _nullNode;
    } // if
  } // for
} // BddImpl::cleanCache


//      Function : BddImpl::cleanCache
//      Abstract : Clean a computed cache.
void
BddImpl::cleanCache(ComputedTbl3 &table, bool force)
{
  for (auto &data : table) {
    bool umF = nodeUnmarked(data._f, 0);
    bool umG = nodeUnmarked(data._g, 0);
    bool umH = nodeUnmarked(data._h, 0);
    bool umR = nodeUnmarked(data._r, 0);
    if (force || umF || umG || umH || umR) {
      data._f = _nullNode;
      data._g = _nullNode;
      data._h = _nullNode;
      data._r = _nullNode;
    } // if
  } // for
} // BddImpl::cleanCache

} // namespace abide
