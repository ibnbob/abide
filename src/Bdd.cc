//
//      File     : Bdd.cc
//      Abstract : Public API functions for BddMgr and BddFnSet.
//

#include <Bdd.h>
#include <BddImpl.h>
#include <climits>

namespace abide {

// BddMgr


//      Function : BddMgr::BddMgr
//      Abstract : Constructor.
BddMgr::BddMgr() : BddMgr(DFLT_VAR_SZ, DFLT_NODE_SZ, DFLT_CACHE_SZ)
{
} // BddMgr::BddMgr


//      Function : BddMgr::BddMgr
//      Abstract : Constructor.
BddMgr::BddMgr(size_t numVars) : BddMgr(numVars, DFLT_NODE_SZ, DFLT_CACHE_SZ)
{
} // BddMgr::BddMgr


//      Function : BddMgr::BddMgr
//      Abstract : Constructor.
BddMgr::BddMgr(size_t numVars,
               size_t maxNodes) : BddMgr(numVars, maxNodes, DFLT_CACHE_SZ)
{
} // BddMgr::BddMgr


//      Function : BddMgr::BddMgr
//      Abstract : Constructor. numVars is the initial maximum
//      variable index. This is meant to be an estimate and will grow
//      if more variables are requested. maxNodes is the maximum
//      number of nodes allowed. This number may be exceeded during
//      variable reordering but will be below at completion. All other
//      operations should respect this. cacheSz is the size the size
//      of the various computed caches. If it is not a power of 2,
//      then it will be increased to the next power of two.
BddMgr::BddMgr(size_t numVars,
               size_t maxNodes,
               size_t cacheSz)
{
  maxNodes = maxNodes ? maxNodes : DFLT_NODE_SZ;
  cacheSz = cacheSz ? cacheSz : DFLT_CACHE_SZ;
  _impl = std::make_unique<BddImpl>(numVars, maxNodes, cacheSz);
} // BddMgr::BddMgr


//      Function : BddMgr::~BddMgr
//      Abstract : Destructor.
BddMgr::~BddMgr()
{
} // BddMgr::~BddMgr


//      Function : BddMgr::getOne
//      Abstract : Return the BDD of the constant one function.
Bdd
BddMgr::getOne() const
{
  return Bdd(_impl->getOne(), this);
} // BddMgr::getOne

//      Function : BddMgr::getZero
//      Abstract : Return the BDD of the constant zero function.
Bdd
BddMgr::getZero() const
{
  return Bdd(_impl->getZero(), this);
} // BddMgr:getZero


//      Function : BddMgr::getLit
//      Abstract : Return the BDD for the given literal. If lit > 0,
//      then the positive phase of abs(lit) is returned, If lit < 0,
//      then the negative phase is returned. It is an error to
//      request literal 0.
Bdd
BddMgr::getLit(const BddLit lit) const
{
  assert(lit != 0);
  return Bdd(_impl->getLit(lit), this);
} // BddMgr::getLit


//      Function : BddMgr::getIthLit
//      Abstract : Return the BDD for the given literal. If lit > 0,
//      then the positive phase of abs(lit) is returned, If lit < 0,
//      then the negative phase is returned. It is an error to
//      request literal 0.
Bdd
BddMgr::getIthLit(const BddIndex index) const
{
  assert(index != 0);
  return Bdd(_impl->getIthLit(index), this);
} // BddMgr::getIthLit


//      Function : BddMgr::supportVec
//      Abstract : Return the support of all functions as a cube.
BddVarVec
BddMgr::supportVec(const BddVec &bdds) const
{
  Bdd supp(getOne());
  for (const auto &bdd : bdds) {
    supp *= bdd.supportCube();
  } // for
  return supp.supportVec();
} // BddMgr::supportVec


//      Function : BddMgr::lockGC
//      Abstract : Lock the manager from performing a garbage
//      collection. Each call to lockGC() needs a corresponding call
//      to unlockGC() in order for garbage corresponding to be enabled.
void
BddMgr::lockGC() const
{
  _impl->lockGC();
} // BddMgr::lockGC


//      Function : BddMgr::unlockGC
//      Abstract : Unlock the manager from performing a garbage
//      collection. Each call to lockGC() needs a corresponding call
//      to unlockGC() in order for garbage corresponding to be enabled.
void
BddMgr::unlockGC() const
{
  _impl->unlockGC();
} // BddMgr::unlockGC


//      Function : BddMgr::gc
//      Abstract : Force a garbage collection.
size_t
BddMgr::gc(bool force, bool verbose) const
{
  return _impl->gc(force, verbose);
} // BddMgr::gc


//      Function : BddMgr::reorder
//      Abstract : Force a variable reordering.
size_t
BddMgr::reorder(bool verbose) const
{
  return _impl->reorder(verbose);
} // BddMgr::reorder


//      Function : BddMgr::getVarOrder
//      Abstract : Return the ordering of the current BddVars
const BddVarVec &
BddMgr::getVarOrder() const
{
  return _impl->getVarOrder();
} // BddMgr::getVarOrder


//      Function : BddMgr::checkMem
//      Abstract : Perform a simple check of the memory state.
bool
BddMgr::checkMem() const
{
  return _impl->checkMem();
} // BddMgr::checkMem


//      Function : BddMgr::printStats
//      Abstract : Print cache stats if enabled.
void
BddMgr::printStats()
{
  _impl->printStats();
} // BddMgr::printStats


//      Function : BddMgr::nodesAllocd
//      Abstract : Return the number of nodes allocated.
size_t
BddMgr::nodesAllocd() const
{
  return _impl->nodesAllocd();
} // BddMgr::nodesAllocd


//      Function : BddMgr::varsCreated
//      Abstract : Return the number of variables created.
size_t
BddMgr::varsCreated() const
{
  return _impl->varsCreated();
} // BddMgr::varsCreated


//      Function : BddMgr::setMaxNodes
//      Abstract : Set the maximum number of nodes allowed. Mostly for
//      debugging and test generation.
void
BddMgr::setMaxNodes(size_t maxNodes)
{
  _impl->setMaxNodes(maxNodes);
} // BddMgr::setMaxNodes


//      Function : BddMgr::isOne
//      Abstract : Return true if the bdd is the one function.
bool
BddMgr::isOne(const Bdd &f) const
{
  return _impl->isOne(f._me);
} // BddMgr::isOne


//      Function : BddMgr::isZero
//      Abstract : Return true if the bdd is the zero function.
bool
BddMgr::isZero(const Bdd &f) const
{
  return _impl->isZero(f._me);
} // BddMgr::isZero


//      Function : BddMgr::isConstant
//      Abstract : Return true if the bdd is a constant function.
bool
BddMgr::isConstant(const Bdd &f) const
{
  return _impl->isConstant(f._me);
} // BddMgr::isConstant


//      Function : BddMgr::isPosLit
//      Abstract : Return true if the bdd is a positive literal.
bool
BddMgr::isPosLit(const Bdd &f) const
{
  return _impl->isPosLit(f._me);
} // BddMgr::isPosLit


//      Function : BddMgr::isNegLit
//      Abstract : Return true if the bdd is a negative literal.
bool
BddMgr::isNegLit(const Bdd &f) const
{
  return _impl->isNegLit(f._me);
} // BddMgr::isNegLit


//      Function : BddMgr::isCube
//      Abstract : Return true if the bdd is a cube (product of literals).
bool
BddMgr::isCube(const Bdd &f) const
{
  return _impl->isCube(f._me);
} // BddMgr::isCube


//      Function : BddMgr::invert
//      Abstract : Return the BDD of the inverted boolean function.
Bdd
BddMgr::invert(const BDD f) const
{
  return Bdd(_impl->invert(f), this);
} // BddMgr::invert


//      Function : BddMgr::abs
//      Abstract : Return the absolute value of this BDD defined as
//      the sign-bit being 0.
Bdd
BddMgr::abs(const BDD f) const
{
  return Bdd(_impl->abs(f), this);
} // BddMgr::abs


//      Function : BddMgr::apply
//      Abstract : Return the BDD that results from applying the
//      operation to the two input functions.
Bdd
BddMgr::apply(const BDD f, const BDD g, const BddOp op) const
{
  Bdd rtn(_impl->apply(f, g, op), this);
  _impl->gc(false, false);
  return rtn;
} // BddMgr::apply


//      Function : BddMgr::restrict
//      Abstract : Restrict the function f to the function c. Also, known
//      as the generalized cofactor. If c is a cube, the result will
//      be the same as the normal Shannon cofactor.
Bdd
BddMgr::restrict(const BDD f, const BDD c) const
{
  Bdd rtn(_impl->restrict(f, c), this);
  _impl->gc(false, false);
  return rtn;
} // BddMgr::restrict


//      Function : BddMgr::compose
//      Abstract : Replace variable x with function g in function f.
Bdd
BddMgr::compose(BDD f, BddVar x, BDD g) const
{
  Bdd rtn(_impl->compose(f, x, g), this);
  _impl->gc(false, false);
  return rtn;
} // BddMgr::compose


//      Function : BddMgr::andExists
//      Abstract : Parameters f and g are arbitrary functions and c is
//      a product of positive literals. The result is Exists(c,
//      f*g). This is sometimes called the relational product.
Bdd
BddMgr::andExists(const Bdd f, const Bdd g, const Bdd c) const
{
  return andExists(f._me, g._me, c._me);
} // BddMgr::andExists


//      Function : BddMgr::andExists
//      Abstract : Parameters f and g are arbitrary functions and c is
//      a product of positive literals. The result is Exists(c,
//      f*g). This is sometimes called the relational product.
Bdd
BddMgr::andExists(const BDD f, const BDD g, const BDD c) const
{
  Bdd rtn(_impl->andExists(f, g, c), this);
  _impl->gc(false, false);
  return rtn;
} // BddMgr::andExists


//      Function : BddMgr::ite
//      Abstract : External access to ite() function. Might be useful,
//      but mostly for unit testing.
Bdd
BddMgr::ite(const Bdd f, const Bdd g, const Bdd h) const
{
  Bdd rtn(_impl->ite(f._me, g._me, h._me), this);
  _impl->gc(false, false);
  return rtn;
} // BddMgr::ite


//      Function : BddMgr::covers
//      Abstract : Returns true if f covers g.
bool
BddMgr::covers(const BDD f, const BDD g) const
{
  return _impl->covers(f, g);
} // BddMgr::covers


//      Function : BddMgr::cubeFactor
//      Abstract : Extract the cube factor with the most leterals.
Bdd
BddMgr::cubeFactor(const BDD f) const
{
  return Bdd(_impl->cubeFactor(f), this);
} // BddMgr::cubeFactor


//      Function : BddMgr::getIf
//      Abstract : Returns the a BDD for the top variable.
Bdd
BddMgr::getIf(BDD f) const
{
  return getLit(getTopVar(f));
} // BddMgr::getIf


//      Function : BddMgr::getThen
//      Abstract : Returns the a BDD for positive cofactor w.r.t. the
//      top variable.
Bdd
BddMgr::getThen(BDD f) const
{
  return Bdd(_impl->getThen(f), this);
} // BddMgr::getThen


//      Function : BddMgr::getElse
//      Abstract : Returns the a BDD for positive cofactor w.r.t. the
//      top variable.
Bdd
BddMgr::getElse(BDD f) const
{
  return Bdd(_impl->getElse(f), this);
} // BddMgr::getElse


//      Function : BddMgr::getTopVar
//      Abstract : Get the top variable of this BDD.
BddVar
BddMgr::getTopVar(const BDD f) const
{
  return _impl->getTopVar(f);
} // BddMgr::getTopVar


//      Function : BddMgr::getIndex
//      Abstract : Get the index (i.e. order) of the top variable.
BddIndex
BddMgr::getIndex(BDD f) const
{
  return _impl->getIndex(f);
} // BddMgr::getIndex


//      Function : BddMgr::supportSize
//      Abstract : Return the support of the function as vector.
size_t
BddMgr::supportSize(const BDD f) const
{
  return _impl->supportSize(f);
} // BddMgr::supportSize


//      Function : BddMgr::supportVec
//      Abstract : Return the support of the function as vector.
BddVarVec
BddMgr::supportVec(const BDD f) const
{
  return _impl->supportVec(f);
} // BddMgr::supportVec


//      Function : BddMgr::supportCube
//      Abstract : Return the support of the function as a cube.
Bdd
BddMgr::supportCube(const BDD f) const
{
  return Bdd(_impl->supportCube(f), this);
} // BddMgr::supportCube


//      Function : BddMgr::oneCube
//      Abstract : Return a satisfying cube if one exists. zero otherwise.
Bdd
BddMgr::oneCube(const BDD f ) const
{
  return Bdd(_impl->oneCube(f), this);
} // BddMgr::oneCube


//      Function : BddMgr::countNodes
//      Abstract : Count the number of nodes in the BDD rooted here.
size_t
BddMgr::countNodes(const BDD f) const
{
  BDDVec v;
  v.push_back(f);

  return _impl->countNodes(v);
} // BddMgr::countNodes


//      Function : BddMgr::countNodes
//      Abstract : Count the number of nodes in the rooted at the BDDs
//      in the vector.
size_t
BddMgr::countNodes(const BddVec &bdds) const
{
  BDDVec v;
  for (const auto &bdd : bdds) {
    v.push_back(bdd.id());
  } // fill vect with BDDs

  return _impl->countNodes(v);
} // BddMgr::countNodes


//      Function : BddMgr::supportCube
//      Abstract : Return the support of all functions as a cube.
Bdd
BddMgr::supportCube(const BddVec &bdds) const
{
  Bdd rtn(getOne());
  for (const auto &bdd : bdds) {
    rtn *= bdd.supportCube();
  } // for
  return rtn;
} // BddMgr::supportCube


//      Function : BddMgr::incRef
//      Abstract : Increment the reference count for BDD.
void
BddMgr::incRef(BDD f) const
{
  _impl->incRef(f);
} // BddMgr::incRef


//      Function : BddMgr::decRef
//      Abstract : Decrement the reference count for BDD.
void
BddMgr::decRef(BDD f) const
{
  _impl->decRef(f);
} // BddMgr::decRef


//      Function : BddMgr::numRefs
//      Abstract : Get the number of external refs.
size_t
BddMgr::numRefs(BDD f) const
{
  return _impl->numRefs(f);
} // BddMgr::numRefs


//      Function : BddMgr::print
//      Abstract : Pretty print the BDD.
void
BddMgr::print(const BDD f) const
{
  _impl->print(f);
} // BddMgr::print


//      Function : BddMgr::print
//      Abstract : Pretty print the BDD.
void Bdd::print() const {
  _mgr->print(_me);
} // Bdd::print


// BddFnSet


//      Function : BddFnSet::insert
//      Abstract : Insert a Bdd function into the set. Return true if
//      the function was added. Return false if insertion failed or
//      the function was already in the set.
bool
BddFnSet::insert(Bdd f)
{
  assert (_mgr == nullptr || (_mgr == f.getMgr()));
  _mgr = f.getMgr();
  auto [iter, inserted] = _bddSet.insert(f);
  return inserted;
} // BddFnSet::insert


//      Function : BddFnSet::erase
//      Abstract : Removes a Bdd function from the set. Returns true
//      if a function was removed.
bool
BddFnSet::erase(Bdd f)
{
  assert (_mgr == f.getMgr());
  return _bddSet.erase(f);
} // BddFnSet::erase


//      Function : BddFnSet::size
//      Abstract : Return the number of functions in the set.
int
BddFnSet::size()
{
  return _bddSet.size();
} // BddFnSet::size


//      Function : BddFnSet::getTop
//      Abstract : Return a Bdd for the positive literal of the top
//      variable of all Bdds in the set. Returns 0 function if set is
//      empty.
Bdd
BddFnSet::getTop()
{
  BddVar getTopVar = UINT_MAX;
  Bdd rtn = _mgr->getZero();
  for (auto bdd : _bddSet) {
    if (!bdd.isConstant()) {
      BddVar tmp = bdd.getTopVar();
      if (tmp < getTopVar) {
        getTopVar = tmp;
        rtn = _mgr->getLit(getTopVar);
      } // if new top
    } // if not constant
  } // for

  return rtn;
} // BddFnSet::getTop


//      Function : BddFnSet::restrict
//      Abstract : Return a new set restricted to the literal.
BddFnSet
BddFnSet::restrict(const Bdd lit) const
{
  BddFnSet rtn;
  for (auto bdd : _bddSet) {
    auto f = bdd.restrict(lit);
    rtn.insert(f);
  } // for

  return rtn;
} // BddFnSet::restrict


//      Function : BddFnSet::eliminate
//      Abstract : Return a new set with the literal eliminated. Both
//      cofactors of each element are placed in the new set.
BddFnSet
BddFnSet::eliminate(const Bdd lit) const
{
  BddFnSet rtn;
  for (auto bdd : _bddSet) {
    auto f = bdd/lit;
    rtn.insert(f);
    f = bdd/~lit;
    rtn.insert(f);
  } // for

  return rtn;
} // BddFnSet::eliminate

} // namespace abide
