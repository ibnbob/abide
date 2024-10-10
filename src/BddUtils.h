//
//      File     : BddUtils.h
//      Abstract : Interface to various utility classes and functions
//      which can be used with the Bdd package.
//
//      + Bdd findProduct(Bdd &f) - try to find a Bdd h s.t. there is
//        also a Bdd g and f = g*h. Bdd g can be obtained using the
//        generalized cofactor of f w.r.t. h. For example:
//        Bdd f - ...;
//        Bdd g = findProduct(f);
//        Bdd h = f/g;
//        assert(f == g*h);
//
//      * Dnf extractDnf(Bdd &f) - extract an irredundant DNF formula
//        for f.

#ifndef BDDUTILS_H
#define BDDUTILS_H

#include <Bdd.h>

namespace abide {

Bdd findProduct(const Bdd &f);
Bdd findXor(const Bdd &f);

using Term = std::vector<BddLit>;
using Dnf = std::vector<Term>;

Dnf extractDnf(Bdd &f);
Bdd dnf2Bdd(const BddMgr &mgr, Dnf &dnf);

} // namespace abide

#endif // BDDUTILS_H
