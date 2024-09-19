//
//      File     : Element.cc
//	Abstract : Implementation of Element class
//

#include "Element.h"
#include <iostream>

//      Function : Element::Element
//      Abstract : CTOR

Element::Element(std::string name, ElId id) :
  _name(name),
  _id(id),
  _type(ElType::MERDE),
  _rank(-1)
{
} // Element::Element


//      Function : Element::~Element
//      Abstract : DTOR

Element::~Element()
{
} // Element::~Element


//      Function : operator<<
//      Abstract : Write an ElType to a stream.
std::ostream&
operator<<(std::ostream& os, const ElType type)
{
  switch(type) {
    case ElType::INPUT:
      os << "INPUT";
      break;
    case ElType::BUF:
      os << "BUF";
      break;
    case ElType::INV:
      os << "INV";
      break;
    case ElType::AND:
      os << "AND";
      break;
    case ElType::OR:
      os << "OR";
      break;
    case ElType::NAND:
      os << "NAND";
      break;
    case ElType::NOR:
      os << "NOR";
      break;
    case ElType::XOR:
      os << "XOR";
      break;
    case ElType::XNOR:
      os << "XNOR";
      break;
    case ElType::MERDE:
      os << "MERDE";
      break;
  } // switch

  return os;
} // operator<<


//      Function : getTypeByName
//      Abstract : Return the proper element type.
ElType
getTypeByName(std::string_view &typeNm)
{
  if (typeNm == "INPUT") {
    return ElType::INPUT;
  } else if (typeNm == "buf") {
    return ElType::BUF;
  } else if (typeNm == "not") {
    return ElType::INV;
  } else if (typeNm == "and") {
    return ElType::AND;
  } else if (typeNm == "or") {
    return ElType::OR;
  } else if (typeNm == "nand") {
    return ElType::NAND;
  } else if (typeNm == "nor") {
    return ElType::NOR;
  } else if (typeNm == "xor") {
    return ElType::XOR;
  } else if (typeNm == "xnor") {
    return ElType::XNOR;
  } else {
    return ElType::MERDE;
  } // if
} // getTypeByName

