//
//      File     : BddImpl.cc
//      Abstract : Implements basic interface functions.
//

#include "BddImpl.h"

namespace abide {

//      Function : BddImpl::BddImpl
//      Abstract : CTOR
BddImpl::BddImpl(unsigned int numVars,
                 unsigned long maxNodes,
                 unsigned long cacheSz) :
  _gcLock(0),
  _maxIndex(0),
  _curNodes(0),
  _maxNodes(maxNodes),
  _nodesAllocd(0),
  _maxAllocd(0),
  _nodesFree(0),
  _gcTrigger(std::min(1UL<<10, _maxNodes<<6)),
  _reordering(false),
  _freeList(0),
  _nullNode(0),
  _oneNode(0),
  _zeroNode(0),
  _uniqTbls(*this)
{
  initialize(numVars, cacheSz);

  _nullNode = allocateNode();
  assert(_nullNode == 0);
  _oneNode = allocateNode();
  _zeroNode = invert(_oneNode);

  BddNode &n(getNode(_oneNode));
  n.setIndex(BDD_MAX_INDEX);
} // BddImpl::BddImpl


//      Function : BddImpl::~BddImpl
//      Abstract : DTOR
BddImpl::~BddImpl()
{
#ifdef BANKEDMEM
  for (auto *b : _banks) {
    delete [] b;
  } // for
#else
  delete [] _nodes;
#endif
} // BddImpl::~BddImpl


//      Function : BddImpl::initialize
//      Abstract :
void
BddImpl::initialize(unsigned int numVars, unsigned long cacheSz)
{
  _var2Index.resize(numVars+1);
  _index2BddVar.resize(numVars+1);
  for (unsigned int idx = 1; idx <= numVars; ++idx) {
    _var2Index[idx] = _index2BddVar[idx] = idx;
  } // for

  _compCacheSz = 1;
  while (_compCacheSz < cacheSz) {
    _compCacheSz *= 2;
  } // while
  _compCacheMask = _compCacheSz - 1;

  _andTbl.resize(_compCacheSz);
  _xorTbl.resize(_compCacheSz);
  _restrictTbl.resize(_compCacheSz);
  _iteTbl.resize(_compCacheSz);
  _andExistTbl.resize(_compCacheSz);
} // BddImpl::initialize


//      Function : BddImpl::getLit
//      Abstract : Return the BDD of the given literal.
BDD
BddImpl::getLit(int lit)
{
  assert(lit != 0);

  unsigned int var = std::abs(lit);
  if (var >= _var2Index.size()) {
    unsigned int nuBddVar = _var2Index.size();
    while (nuBddVar <= var) {
      _var2Index.push_back(nuBddVar);
      _index2BddVar.push_back(nuBddVar);
      ++nuBddVar;
    } // while
  } // if need to extend var-index vectors.

  unsigned int index = _var2Index[var];

  BDD rtn(lit > 0 ?
          findOrAddUniqTbl(index, _oneNode, _zeroNode) :
          findOrAddUniqTbl(index, _zeroNode, _oneNode));

  return rtn;
} // BddImpl::getLit


//      Function : BddImpl::getIthLit
//      Abstract : Return Bdd if the literal with the given
//      index. Since BddIndex is unsigned, we always return the
//      positive literal.
BDD
BddImpl::getIthLit(BddIndex index)
{
  if (index >= _var2Index.size()) {
    unsigned int nuBddVar = _var2Index.size();
    while (nuBddVar <= index) {
      _var2Index.push_back(nuBddVar);
      _index2BddVar.push_back(nuBddVar);
      ++nuBddVar;
    } // while
  } // if need to extend var-index vectors.

  BDD rtn(findOrAddUniqTbl(index, _oneNode, _zeroNode));

  return rtn;
} // BddImpl::getIthLit


//      Function : BddImpl::countNodes
//      Abstract : Count the number of nodes in the rooted at the BDDs
//      in the vector.
unsigned int
BddImpl::countNodes(BDDVec &bdds) const
{
  unsigned int count = 0;

  for (const auto bdd : bdds) {
    count += countNodes(bdd);
  } // for

  for (const auto bdd : bdds) {
    unmarkNodes(bdd, 1);
  } // for

  return count;
} // BddImpl::countNodes

//      Function : BddImpl::print
//      Abstract : Pretty print the BDD.
void
BddImpl::print(BDD f) const
{
  printRec(f, 0);
  unmarkNodes(f, 1);
  std::cout << std::endl;
} // BddImpl::print



//      Function : BddImpl::printRec
//      Abstract : Recursive helper function for print().
void
BddImpl::printRec(const BDD f, const unsigned int level) const
{
  std::cout << std::string(2*level, ' ');
  if (isZero(f)) {
    std::cout << "[0]" << std::endl;
  } else if (isOne(f)) {
    std::cout << "[1]" << std::endl;
  } else {
    unsigned int addr = f >> 1;
    if (nodeMarked(f, 1)) {
      std::cout << "["
           << (isNegPhase(f) ? "~" : "")
           << addr
           << "]"
           << std::endl;
    } else {
      markNode(f, 1);
      std::cout << (isNegPhase(f) ? "~" : "");
      std::cout.width(4);
      std::cout.fill('0');
      std::cout << addr;
      std::cout.fill(' ');
      std::cout << ":" << getIndex(f)
           << std::endl;

      printRec(getHi(f), level+1);
      printRec(getLo(f), level+1);
    } // if
  } // if
} // BddImpl::printRec

} // namespace abide
