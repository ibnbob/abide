//
//      File     : BddImpl.cc
//      Abstract : Implements basic interface functions.
//

#include "BddImpl.h"

//      Function : BddImpl::BddImpl
//      Abstract : CTOR
BddImpl::BddImpl() :
  _gcLock(0),
  _maxIndex(0),
  _curNodes(0),
  _maxNodes(UINT32_MAX),
  _nodesAllocd(0),
  _maxAllocd(0),
  _nodesFree(0),
  _gcTrigger(1<<20),
  _freeList(0),
  _nullNode(0),
  _oneNode(0),
  _zeroNode(0),
  _uniqTbls(*this)
{
  _andTbl.resize(COMP_CACHE_SZ);
  _xorTbl.resize(COMP_CACHE_SZ);
  _iteTbl.resize(COMP_CACHE_SZ);

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
  cout << endl;
} // BddImpl::print



//      Function : BddImpl::printRec
//      Abstract : Recursive helper function for print().
void
BddImpl::printRec(const BDD f, const unsigned int level) const
{
  cout << string(2*level, ' ');
  if (isZero(f)) {
    cout << "[0]" << endl;
  } else if (isOne(f)) {
    cout << "[1]" << endl;
  } else {
    unsigned int addr = f >> 1;
    if (nodeMarked(f, 1)) {
      cout << "["
           << (isNegPhase(f) ? "~" : "")
           << addr
           << "]"
           << endl;
    } else {
      markNode(f, 1);
      cout << (isNegPhase(f) ? "~" : "");
      cout.width(4);
      cout.fill('0');
      cout << addr;
      cout.fill(' ');
      cout << ":" << getIndex(f)
           << endl;

      printRec(getHi(f), level+1);
      printRec(getLo(f), level+1);
    } // if
  } // if
} // BddImpl::printRec
