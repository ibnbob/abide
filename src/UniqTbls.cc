//
//      File     : BddImplUniq.cc
//      Abstract : Implementation of unique table functions.
//

#include <BddImpl.h>

namespace abide {

namespace {
const size_t UNIQ_LG_SZ = 12;
const size_t UNIQ_INIT_SZ = 1 << UNIQ_LG_SZ;
const size_t UNIQ_LD_FACTOR = 1;
const size_t UNIQ_LG_GROWTH_FACTOR = 2;
} // anonymous namespace

//      Function : UniqTbl::UniqTbl
//      Abstract : Constructor.
UniqTbl::UniqTbl() :
  _tbl(nullptr),
  _size(UNIQ_INIT_SZ),
  _mask(_size-1),
  _numNodes(0),
  _processed(false)
{
  _tbl = new BDD[_size];
  for (size_t idx = 0; idx < _size; ++idx) {
    _tbl[idx] = 0;
  } // zero-out mem
} // UniqTbl::UniqTbl


//      Function : UniqTbl::findOrAdd
//      Abstract : Find or add a node in this table.
BDD
UniqTbl::findOrAdd(BddImpl &impl,
                   const int index,
                   const BDD hi,
                   const BDD lo)
{
  BDD rtn = 0;
  uint32_t hash = hash2(hi, lo) & _mask;
  BDD cur;
  BDD next;

  impl._cacheStats.incUniqAccess();

  for (cur = getHash(hash); cur; cur = next) {
    impl._cacheStats.incUniqChain();
    BddNode &n = impl.getNode(cur);
    // getHi() and getLo() should be OK as only uninverted nodes
    // should be stored in the the unique tables.
    if ((n.getHi()) == hi && (n.getLo() == lo)) {
      rtn = cur;
      impl._cacheStats.incUniqHit();
      break;
    } else {
      next = n.getNext();
    } // if found
  } // for nodes in entry

  if (rtn == 0) {
    impl._cacheStats.incUniqMiss();
    rtn = impl.allocateNode();

    if (rtn) {
      BddNode &n = impl.getNode(rtn);
      n.setIndex(index);
      n.setHi(hi);
      n.setLo(lo);
      putHash(impl, rtn, hash);

      if (_numNodes > UNIQ_LD_FACTOR * _size) {
        resize(impl);
      } // if load average > 2
    }  /* if (rtn) ... */
  } // if not already created.

  return rtn;
} // UniqTbl::findOrAdd


//      Function : UniqTbl::resize
//      Abstract : Resize the table to reduce the load average.
void
UniqTbl::resize(BddImpl &impl)
{
  //cout << "Resizing utable." << endl;
  BDD *oldTbl = _tbl;
  size_t oldSize = _size;

  _size = _size << UNIQ_LG_GROWTH_FACTOR;
  _mask = _size - 1;
  _tbl = new BDD[_size];

  for (size_t idx = 0; idx < _size; ++idx) {
    _tbl[idx] = 0;
  } // zero-out mem

  for (size_t idx = 0; idx < oldSize; ++idx) {
    BDD f = oldTbl[idx];
    BDD next;

    while (f) {
      next = impl.getNext(f);
      putHash(impl, f);
      f = next;
    } // while nodes in bin
  } // for each hash entry

  delete [] oldTbl;
} // UniqTbl::resize


//      Function : UniqTbl::getHash
//      Abstract : Get the first node in the collision chain for this
//      hash index.
BDD UniqTbl::getHash(const size_t hdx) const
{
  return _tbl[hdx];
}; // getHash


//      Function : UniqTbl::putHash
//      Abstract : Push this node onto the collision chain with this
//      hash index.
void
UniqTbl::putHash(BddImpl &impl,
                 const BDD f,
                 const size_t hdx)
{
  BddNode &node = impl.getNode(f);
  node.setNext(_tbl[hdx]);
  _tbl[hdx] = f;
  _numNodes++;
} // UniqTbl::putHash


//      Function : UniqTbl::putHash
//      Abstract : Push this node onto the collision chain with the
//      computes hash index.
void
UniqTbl::putHash(BddImpl &impl, const BDD f)
{
  BddNode &node = impl.getNode(f);
  uint32_t hdx = hash2(node.getHi(), node.getLo()) & _mask;
  node.setNext(_tbl[hdx]);
  _tbl[hdx] = f;
  _numNodes++;
} // UniqTbl::putHash


//      Function : UniqTbl::clear
//      Abstract : Clear out the table while placing all the nodes
//      in the given vector.
void
UniqTbl::clear(BddImpl &impl, BDDVec &nodes)
{
  nodes.reserve(_numNodes);
  for (size_t hdx = 0; hdx < _size; ++hdx) {
    BDD f = _tbl[hdx];
    while (f) {
      nodes.push_back(f);
      f = impl.getNext(f);
    } // while nodes to process
    _tbl[hdx] = 0;
  } // for each hash
  _numNodes = 0;
} // UniqTbl::clear


//      Function : UniqTbls::~UniqTbls
//      Abstract : DTOR

UniqTbls::~UniqTbls()
{
  for (auto &tbl : _tables) {
    tbl.freeTbl();
  } // for
} // UniqTbls::~UniqTbls

} // namespace abide
