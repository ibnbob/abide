//
//      File     : Element.h
//	Abstract : Header file for Element class.
//

#ifndef ELEMENT_H
#define ELEMENT_H

#include <Bdd.h>
#include <string>
#include <vector>

//      Enum     : ElType
//      Abstract : Allowable element types
enum class ElType {
  INPUT,
  BUF,
  INV,
  AND,
  OR,
  NAND,
  NOR,
  XOR,
  XNOR,
  MERDE,
}; // ElType

using ElId = unsigned int;
using ElIdVec = std::vector<ElId>;

//      Class    : Element
//      Abstract : Class for holding a combinational element. An
//      element is an PI, PO or logic gate.

class Element {
public:
  Element(std::string name, ElId id); // CTOR
  ~Element(); // DTOR

  Element(const Element &) = delete; // Copy CTOR
  Element &operator=(const Element &) = delete; // Copy assignment
  Element(Element &&) = delete; // Move CTOR
  Element &operator=(Element &&) = delete; // Move assignment

  // Access
  const std::string &getName() const { return _name; };
  ElId getId() const { return _id; };
  ElType getType() const { return _type; };
  ElIdVec &getFanins() { return _fanins; };
  ElIdVec &getFanouts() { return _fanouts; };
  int getRank() const { return _rank; };
  bool isOutput() const { return _isOutput; };
  Bdd getBdd() const { return _bdd; };

  // Modify
  void setType(ElType type) { _type = type; };
  void addFanin(ElId id) { _fanins.push_back(id); };
  void addFanout(ElId id) { _fanouts.push_back(id); };
  void setRank(int rank) { _rank = rank; };
  void setOutupt(bool flag) { _isOutput = flag; };
  void setBdd(Bdd bdd) { _bdd = bdd; };
private:
  std::string _name;
  ElId _id;
  ElType _type;
  ElIdVec _fanins;
  ElIdVec _fanouts;

  int _rank;
  bool _isOutput;

  Bdd _bdd;
}; // Element

std::ostream& operator<<(std::ostream& os, const ElType type);
ElType getTypeByName(std::string_view &typeNm);

#endif // ELEMENT_H
