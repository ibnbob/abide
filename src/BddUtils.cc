//
//      File     : BddUtils.cc
//      Abstract : Implementation of BddUtils.h.
//

#include "BddUtils.h"
#include <algorithm>

namespace abide {

////////////////////////////////////////////////////////////////
//
// Implementation of findProduct().
//

//      Function : noProduct
//      Abstract : Returns true if there cannot be a non-trivial
//      product based on the current set of subfunctions.
bool
noProduct(BddFnSet &fnSet)
{
  bool seenOne = false;
  bool seenOther = false;
  if (fnSet.size() > 1) {
    for (auto bdd : fnSet) {
      if (bdd.isOne()) {
        seenOne = true;
      } else if (!bdd.isZero()) {
        seenOther = true;
      } // if

      if (seenOne && seenOther) {
        return true;
      } // if
    } // for
  } // if

  return false;
} // noProduct


//      Function : findProduct
//      Abstract : Checks conditions for this level.
Bdd
findProduct(BddFnSet &fnSet)
{
  if (noProduct(fnSet)) {
    return Bdd();
  } else if (fnSet.size() == 1) {
    return *fnSet.getSet().begin();
  } else if (fnSet.size() == 2) {
    auto iter = fnSet.begin();
    Bdd f1 = *iter;
    ++iter;
    Bdd f2 = *iter;
    if (f1.isZero()) {
      return f2;
    } else if (f2.isZero()) {
      return f1;
    } else if (f1 == ~f2) {
      return Bdd();
    } // if either is zero
  } // terminal cases.

  // Non-terminal calculation.
  Bdd x = fnSet.getTop();
  BddFnSet H1 = fnSet.restrict(x);
  BddFnSet H0 = fnSet.restrict(~x);

  if (Bdd h1 = findProduct(H1);
      h1.valid()) {
    if (Bdd h0 = findProduct(H0);
        h0.valid()) {
      return x*h1+ ~x*h0;
    } // if
  } // if

  return Bdd();
} // findProduct


//      Function : findProduct
//      Abstract : Tries to find a non-trival function h s.t. there is
//      also a function g and f=g*h. Function g can be computed using
//      the generalized cofactor. Should return at least the trivial
//      product 1.
//
//      This is a simplified implementation of the algorithm in
//
//      T. Stanion and C. Sechen, "Quasi-algebraic decompositions of
//      switching functions," Proceedings Sixteenth Conference on
//      Advanced Research in VLSI, Chapel Hill, NC, USA, 1995,
//      pp. 358-367.
//
//      for the product case only. (The original algorithm considers
//      all binary operations.)
Bdd
findProduct(const Bdd &f)
{
  assert(f.valid());

  Bdd rtn;

  if (!f.isConstant()) {
    const BddMgr *mgr = f.getMgr();
    BddVarVec vars = f.supportVec();
    BddFnSet H;
    H.insert(f);

    // Ensures at least one variable is in a separate partition.
    vars.pop_back();
    for (auto var : vars) {
      Bdd lit = mgr->getLit(var);
      H = H.eliminate(lit);
      Bdd result = findProduct(H);
      if (result.valid()) {
        rtn = result;
      } else if (!rtn.valid()) {
        break;
      } // if
    } // for
  } // if

  return rtn.valid() ? rtn : f.getMgr()->getOne();;
} // findProduct


////////////////////////////////////////////////////////////////
//
// Implementation of findXor()
//

//      Function : noXor
//      Abstract : Returns true if there cannot be a non-trivial
//      XOR based on the current set of subfunctions.
bool
noXor(BddFnSet &fnSet)
{
  bool seenOne = false;
  bool seenZero = false;
  bool seenOther = false;
  if (fnSet.size() > 1) {
    for (auto bdd : fnSet) {
      if (bdd.isOne()) {
        seenOne = true;
      } else if (bdd.isZero()) {
        seenZero = true;
      } else {
        seenOther = true;
      } // if

      if ((seenOne || seenZero) && seenOther) {
        return true;
      } // if
    } // for
  } // if

  return false;
} // noXor


//      Function : findXor
//      Abstract : Checks conditions for this level.
Bdd
findXor(BddFnSet &fnSet)
{
  if (noXor(fnSet)) {
    return Bdd();
  } else if (fnSet.size() == 1) {
    return *fnSet.getSet().begin();
  } else if (fnSet.size() == 2) {
    auto iter = fnSet.begin();
    Bdd f1 = *iter;
    ++iter;
    Bdd f2 = *iter;
    if (f1 == ~f2) {
      return f1.abs();
    } // if either is zero
  } // terminal cases.

  // Non-terminal calculation.
  Bdd x = fnSet.getTop();
  BddFnSet H1 = fnSet.restrict(x);
  BddFnSet H0 = fnSet.restrict(~x);

  if (Bdd h1 = findXor(H1);
      h1.valid()) {
    if (Bdd h0 = findXor(H0);
        h0.valid()) {
      return x*h1+ ~x*h0;
    } // if
  } // if

  return Bdd();
} // findXor


//      Function : findXor
//      Abstract : Tries to find a non-trivial function h s.t. there is
//      also a function g and f=g^h. Function g can be computed
//      g = f^h. Should return at least the trivial result 0;
//
//      This is a simplified implementation of the algorithm in
//
//      T. Stanion and C. Sechen, "Quasi-algebraic decompositions of
//      switching functions," Proceedings Sixteenth Conference on
//      Advanced Research in VLSI, Chapel Hill, NC, USA, 1995,
//      pp. 358-367.
//
//      for the XOR case only. (The original algorithm considers
//      all binary operations.)
Bdd
findXor(const Bdd &f)
{
  assert(f.valid());

  Bdd rtn;
  const BddMgr *mgr = f.getMgr();
  BddVarVec vars = f.supportVec();
  BddFnSet H;
  H.insert(f);

  // Ensures at least one variable is in a separate partition.
  vars.pop_back();
  for (auto var : vars) {
    Bdd lit = mgr->getLit(var);
    H = H.eliminate(lit);
    Bdd result = findXor(H);
    if (result.valid()) {
      rtn = result;
    } else if (!rtn.valid()) {
      break;
    } // if
  } // for

  return rtn.valid() ? rtn : f.getMgr()->getZero();
} // findXor


////////////////////////////////////////////////////////////////
//
// Implementation of extractDnf().
//


using DnfPair = std::pair<Bdd, Dnf>;

//      Function : combineDnf
//      Abstract : Combine the three DNF formulas w.r.t. the variable x.
Dnf
combineDnf(Bdd x, Dnf d0, Dnf d1, Dnf d2)
{
  Dnf rtn;

  BddVar v = x.getTopVar();
  size_t numTerms = d0.size() + d1.size() + d2.size();
  size_t idx = 0;
  rtn.resize(numTerms);

  for (auto &term : d0) {
    term.push_back(-v);
    rtn[idx++] = std::move(term);
  } // for

  for (auto &term : d1) {
    term.push_back(v);
    rtn[idx++] = std::move(term);
  } // for

  for (auto &term : d2) {
    rtn[idx++] = std::move(term);
  } // for

  assert(idx == numTerms);

  return rtn;
} // combineDnf


//      Function : dnf2Bdd
//      Abstract : Create BDD for this DNF formula.
Bdd
dnf2Bdd(const BddMgr &mgr, Dnf &dnf)
{
  Bdd sum = mgr.getZero();
  for (auto &term : dnf) {
    Bdd prod = term2Bdd(mgr, term);
    sum += prod;
  } // for

  return sum;
} // dnf2Bdd


//      Function : term2Bdd
//      Abstract : Create a BDD for this term.
Bdd
term2Bdd(const BddMgr &mgr, Term &term)
{
  Bdd prod = mgr.getOne();
  for (auto &lit : term) {
    prod *= mgr.getLit(lit);
  } // for

  return prod;
} // term2Bdd


//      Function : extractDnf
//      Abstract : Recursively extract a DNF formula from an
//      interval. This is an implementation of the Mintato-Morreale
//      algorithm as described in
//
//      S. Minato: "Fast Generation of Prime-Irredundant Covers from
//      Binary Decision Diagrams," IEICE Trans. Fundamentals,
//      Vol. E76-A, No. 6, pp. 967-973, June 1993.
DnfPair
extractDnfPair(BddInterval &f)
{
  Dnf dnf;
  if (f.min().isZero()) {
    return {f.min(), dnf};
  } // if

  if (f.max().isOne()) {
    dnf.resize(1);
    return {f.max(), dnf};
  } // if

  Bdd x = f.getTopVar();

  BddInterval f0(f.min()/~x, f.max()/~x);
  BddInterval f1(f.min()/ x, f.max()/ x);

  BddInterval fp0(f0.min()*~f1.max(), f0.max());
  BddInterval fp1(f1.min()*~f0.max(), f1.max());

  auto [g0, dnf0] = extractDnfPair(fp0);
  assert(g0 == dnf2Bdd(*x.getMgr(), dnf0));
  assert(g0 <= fp0);
  auto [g1, dnf1] = extractDnfPair(fp1);
  assert(g1 == dnf2Bdd(*x.getMgr(), dnf1));
  assert(g1 <= fp1);

  BddInterval fpp0(f0.min()*~g0, f0.max());
  BddInterval fpp1(f1.min()*~g1, f1.max());
  BddInterval fStar(fpp0.min()+fpp1.min(), fpp0.max()*fpp1.max());

  auto [g2, dnf2] = extractDnfPair(fStar);
  assert(g2 == dnf2Bdd(*x.getMgr(), dnf2));
  assert(g2 <= fStar);

  Bdd g = ~x*g0 + x*g1 + g2;
  dnf = combineDnf(x, dnf0, dnf1, dnf2);
  assert(g == dnf2Bdd(*x.getMgr(), dnf));
  assert(g <= f);

  return {g, dnf};
} // extractDnf


//      Function : extractDnf
//      Abstract : Extract an irredundant DNF formula for f. This is
//      an implementation of the Minato-Morreale algorithm as
//      described in:
//
//      S. Minato: "Fast Generation of Prime-Irredundant Covers from
//      Binary Decision Diagrams," IEICE Trans. Fundamentals,
//      Vol. E76-A, No. 6, pp. 967-973, June 1993.
Dnf
extractDnf(Bdd &f)
{
  BddInterval ff(f);
  auto [g, dnf] = extractDnfPair(ff);
  return dnf;
} // extractDnf



//      Function : extractDnf
//      Abstract : As above, but for an interval.
Dnf
extractDnf(BddInterval &ff)
{
  auto [g, dnf] = extractDnfPair(ff);
  return dnf;
} // extractDnf


} // namespace abide
