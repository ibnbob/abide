//
//      File     : Ckt.h
//	Abstract :Header for Ckt class.
//

#ifndef CKT_H
#define CKT_H

#include "Element.h"
#include <Bdd.h>

#include <string>
#include <deque>
#include <unordered_map>

//      Class    : Ckt
//      Abstract : Class for holding a combinational circuit.
class Ckt {
public:
  Ckt(bool reorder) :
    _maxRank(-1),
    _reorder(reorder),
    _reorderSz(1<<16)
  {}; // CTOR
  ~Ckt() = default; // DTOR

  Ckt(const Ckt &) = delete; // Copy CTOR
  Ckt &operator=(const Ckt &) = delete; // Copy assignment
  Ckt(Ckt &&) = delete; // Move CTOR
  Ckt &operator=(Ckt &&) = delete; // Move assignment

  bool parse(std::string &filename);
  void buildBdds();
  void printSizes();
  void printStats() { _mgr.printStats(); };

  bool readOrder(std::string &filename);
  bool writeOrder(std::string &filename);
private:
  bool parseLine(std::string_view line);
  bool parseInput(std::string_view &line);
  bool parseOutput(std::string_view &line);
  bool parseGate(std::string_view &line);

  Element &findOrAddElement(std::string &name);

  int calcRanks();
  int calcRank(ElId id);
  void sortByRank();

  Bdd buildBdd(ElId id);
  Bdd buildInputBdd(Element &el);
  Bdd buildBufBdd(Element &el);
  Bdd buildInvBdd(Element &el);
  Bdd buildAndBdd(Element &el);
  Bdd buildOrBdd(Element &el);
  Bdd buildNandBdd(Element &el);
  Bdd buildNorBdd(Element &el);
  Bdd buildXorBdd(Element &el);
  Bdd buildXnorBdd(Element &el);

  void tryReorder(bool verbose);

  // Private data elements.
  BddMgr _mgr;

  std::deque<Element> _elements;
  std::unordered_map<std::string, ElId> _elMap;
  ElIdVec _inputs;
  ElIdVec _outputs;
  int _maxRank;

  bool _reorder;
  unsigned int _reorderSz;
}; // Ckt


#endif // CKT_H
