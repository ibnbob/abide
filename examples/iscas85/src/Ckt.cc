//
//      File     : Ckt.cc
//	Abstract :Implementation kf Ckt class.
//

#include "Ckt.h"
#include "Ticker.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

using std::cin;
using std::cout;
using std::endl;

//      Function : Ckt::parse
//      Abstract : Parse and ISCAS-85 file.
bool
Ckt::parse(std::string &filename)
{
  std::ifstream infile(filename.c_str());

  if (infile.is_open()) {
    std::string line;
    cout << "Reading file " << filename << "." << endl;
    int lineNo = 0;
    while (getline(infile, line)) {
      ++lineNo;
      // std::erase(line,' ');
      auto tmp = std::remove(line.begin(), line.end(), ' ');
      line.erase(tmp, line.end());
      if (!parseLine(std::string_view(line))) {
	cout << "Parsing failed at line " << lineNo << "." << endl;
	cout << line << endl;
	return 1;
      } // if
    } //while
    cout << "Read " << lineNo << " lines." << endl;
    calcRanks();
    sortByRank();
  } else {
    cout << "Could not open file " << filename << "." << endl;
    return false;
  } // if

  return true;
} // Ckt::parse


//      Function : Ckt::parseLine
//      Abstract : Parse a line from the input.
bool
Ckt::parseLine(std::string_view line)
{
  if (line.find("#") == 0 ||
      line.empty()) {
    return true;
  } else if (int pos = line.find("INPUT(");
	     pos == 0) {
    return parseInput(line);
  } else if (int pos = line.find("OUTPUT");
	     pos == 0) {
    return parseOutput(line);
  } else {
    return parseGate(line);
  } // if

  return false; // Unreachable
} // Ckt::parseLine



//      Function : Ckt::parseInput
//      Abstract : Parse an input line
bool
Ckt::parseInput(std::string_view &line)
{
  line.remove_prefix(6);
  line.remove_suffix(1);
  std::string nm(line.begin(), line.end());
  Element &el = findOrAddElement(nm);
  el.setType(ElType::INPUT);
  _inputs.push_back(el.getId());

  return true;
} // Ckt::parseInput


//      Function : Ckt::parseOutput
//      Abstract : Parse an output line
bool
Ckt::parseOutput(std::string_view &line)
{
  line.remove_prefix(7);
  line.remove_suffix(1);
  std::string nm(line.begin(), line.end());
  Element &el = findOrAddElement(nm);
  el.setOutupt(true);
  _outputs.push_back(el.getId());

  return true;
} // Ckt::parseOutput


//      Function : Ckt::parseGate
//      Abstract : Parse a gate line
bool
Ckt::parseGate(std::string_view &line)
{
  auto pos = line.find_first_of("=");
  auto name = line.substr(0, pos);
  line.remove_prefix(pos+1);
  std::string nm(name.begin(), name.end());
  Element &el = findOrAddElement(nm);

  pos = line.find_first_of("(");
  auto typeNm = line.substr(0, pos);
  line.remove_prefix(pos+1);
  el.setType(getTypeByName(typeNm));
  assert(el.getType() != ElType::MERDE);

  pos = line.find_first_of(",)");
  while (pos < std::string_view::npos) {
    auto input = line.substr(0, pos);
    line.remove_prefix(pos+1);
    std::string nm(input.begin(), input.end());
    auto &el2 = findOrAddElement(nm);
    el.addFanin(el2.getId());
    el2.addFanout(el.getId());

    pos = line.find_first_of(",)");
  } // while

  return true;
} // Ckt::parseGate


//      Function : Ckt::findOrAddElement
//      Abstract : Finds or adds and element with the given name.
Element &
Ckt::findOrAddElement(std::string &name)
{
  if (auto entry = _elMap.find(name);
      entry != _elMap.end()) {
    // cout << "Found " << name << endl;
    return _elements[entry->second];
  } else {
    ElId id = _elements.size();
    _elements.emplace_back(name, id);
    _elMap[name] = id;
    // cout << "Added " << name << endl;
    return _elements.back();
  } // if
} // Ckt::findOrAddElement


//      Function : Ckt::calcRanks
//      Abstract : Calculate the rank (level) of each element. PIs are
//      0. Others are one  more than the max of all their inputs
int
Ckt::calcRanks()
{
  int _maxRank = -1;
  for (auto id : _outputs) {
    int tmp = calcRank(id);
    _maxRank = std::max(_maxRank, tmp);
  } //

  return _maxRank;
} // Ckt::calcRanks


//      Function : Ckt::calcRank
//      Abstract : Recursive function for calculating ranks.
int
Ckt::calcRank(ElId id)
{
  Element &el = _elements[id];
  if (el.getRank() == -1) {
    int rank = 0;
    for (auto fi : el.getFanins()) {
      int tmp = calcRank(fi) + 1;
      rank = std::max(rank, tmp);
    } // for
    el.setRank(rank);
  } // if

  return el.getRank();
} // Ckt::calcRank


//      Function : Ckt::sortByRank
//      Abstract : Sort output vector and all fanin vectors by rank.
void
Ckt::sortByRank()
{
  struct cmpRank {
    cmpRank(Ckt &ckt) : _ckt(ckt) {};
    ~cmpRank() {};
    bool operator()(ElId a, ElId b) const {
      Element &elA = _ckt._elements[a];
      Element &elB = _ckt._elements[b];
      return elA.getRank() < elB.getRank();
    };
    Ckt &_ckt;
  } cmp(*this); // cmpRank

  std::sort(_outputs.begin(), _outputs.end(), cmp);
  for (auto &el : _elements) {
    auto &fanins = el.getFanins();
    sort(fanins.begin(), fanins.end(), cmp);
  } // for
} // Ckt::sortByRank


//      Function : Ckt::buildBdds
//      Abstract : Build bdds for each element.
void
Ckt::buildBdds()
{
  Ticker ticker(_outputs.size());
  for (auto id : _outputs) {
    Bdd bdd = buildBdd(id);
    ticker.tick();
  } // for
} // Ckt::buildBdds


//      Function : Ckt::buildBdd
//      Abstract : Build Bdd for this element.
Bdd
Ckt::buildBdd(ElId id)
{
  auto &el = _elements[id];
  Bdd bdd = el.getBdd();
  if (!bdd.valid()) {
    // cout << "Processing element " << el.getName() << endl;
    switch (el.getType()) {
      case ElType::INPUT:
	bdd = buildInputBdd(el);
	break;
      case ElType::BUF:
	bdd = buildBufBdd(el);
	break;
      case ElType::INV:
	bdd = buildInvBdd(el);
	break;
      case ElType::AND:
	bdd = buildAndBdd(el);
	break;
      case ElType::OR:
	bdd = buildOrBdd(el);
	break;
      case ElType::NAND:
	bdd = buildNandBdd(el);
	break;
      case ElType::NOR:
	bdd = buildNorBdd(el);
	break;
      case ElType::XOR:
	bdd = buildXorBdd(el);
	break;
      case ElType::XNOR:
	bdd = buildXnorBdd(el);
	break;
      case ElType::MERDE:
      default:
	assert(false);
	bdd = Bdd();
	break;
    } // switch
  } // fi

  return bdd;
} // Ckt::buildBdd


//      Function : Ckt::buildInputBdd
//      Abstract : Build  bdd for input.
Bdd
Ckt::buildInputBdd(Element &el)
{
  static BddLit nextLit = 1;
  auto bdd = _mgr.getLit(nextLit++);
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildInputBdd


//      Function : Ckt::buildBufBdd
//      Abstract : Build bdd for inverter.
Bdd
Ckt::buildBufBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  assert(fanins.size() == 1);
  Bdd bdd = buildBdd(fanins[0]);
  el.setBdd(bdd);
  tryReorder(false);

  return bdd;
} // Ckt::buildBufBdd


//      Function : Ckt::buildInvBdd
//      Abstract : Build bdd for inverter.
Bdd
Ckt::buildInvBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  assert(fanins.size() == 1);
  Bdd bdd = ~buildBdd(fanins[0]);
  el.setBdd(bdd);
  tryReorder(false);

  return bdd;
} // Ckt::buildInvBdd


//      Function : Ckt::buildAndBdd
//      Abstract : Build bdd for AND gate.
Bdd
Ckt::buildAndBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  Bdd bdd = _mgr.getOne();
  for (auto id : fanins) {
    bdd *= buildBdd(id);
    tryReorder(false);
  } // for
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildAndBdd


//      Function : Ckt::buildOrBdd
//      Abstract : Build bdd for OR gate.
Bdd
Ckt::buildOrBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  Bdd bdd = _mgr.getZero();
  for (auto id : fanins) {
    bdd += buildBdd(id);
    tryReorder(false);
  } // for
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildOrBdd


//      Function : Ckt::buildNandBdd
//      Abstract : Build bdd or NAND gate.
Bdd
Ckt::buildNandBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  Bdd bdd = _mgr.getOne();
  for (auto id : fanins) {
    bdd *= buildBdd(id);
    tryReorder(false);
  } // for
  bdd = ~bdd;
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildNandBdd


//      Function : Ckt::buildNorBdd
//      Abstract : Build bdd for NOR gate.
Bdd
Ckt::buildNorBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  Bdd bdd = _mgr.getZero();
  for (auto id : fanins) {
    bdd += buildBdd(id);
    tryReorder(false);
  } // for
  bdd = ~bdd;
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildNorBdd


//      Function : Ckt::buildXorBdd
//      Abstract : Build bdd for XOR gate.
Bdd
Ckt::buildXorBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  Bdd bdd = _mgr.getZero();
  for (auto id : fanins) {
    bdd ^= buildBdd(id);
    tryReorder(false);
  } // for
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildXorBdd


//      Function : Ckt::buildXnorBdd
//      Abstract : Build bdd for XNOR Xgate.
Bdd
Ckt::buildXnorBdd(Element &el)
{
  ElIdVec fanins = el.getFanins();
  Bdd bdd = _mgr.getZero();
  for (auto id : fanins) {
    bdd ^= buildBdd(id);
    tryReorder(false);
  } // for
  bdd = ~bdd;
  el.setBdd(bdd);

  return bdd;
} // Ckt::buildXnorBdd



//      Function : Ckt::tryReorder
//      Abstract : Try reordering when the total number of nodes is
//      too large.
void
Ckt::tryReorder(bool verbose)
{
  if (_reorder && _mgr.nodesAllocd() > _reorderSz) {
    _mgr.gc();
    auto startSz = _mgr.nodesAllocd();
    if (startSz > _reorderSz) {
      verbose &&
	cout << "Reordering Begin: " << startSz << endl;
#ifdef REORDER_LOOP
      for (auto saved = _mgr.reorder();
	   saved >= (0.1 * startSz);
	   startSz = _mgr.nodesAllocd()) {
	saved = _mgr.reorder();
      } // for
#else
      _mgr.reorder();
#endif
      _reorderSz = 2 * _mgr.nodesAllocd();
      _reorderSz = std::max(_reorderSz,
			    static_cast<unsigned int>(1<<16));
      verbose &&
	cout << "Reordering End: " << _mgr.nodesAllocd() << endl;
    } // if
  } // if may need reorder

} // Ckt::tryReorder


//      Function : Ckt::readOrder
//      Abstract : Read an order file.
bool
Ckt::readOrder(std::string &filename)
{
  if (filename.empty()) {
    return true;
  } // if

  std::ifstream infile(filename.c_str());

  if (infile.is_open()) {
    std::string line;
    while (getline(infile, line)) {
      Element &el = findOrAddElement(line);
      if (el.getType() == ElType::INPUT) {
	buildInputBdd(el);
      } else {
	cout << "Element " << line
	     << " has type " << el.getType() << endl;
	return false;
      } // if
    } //while
  } else {
    cout << "Could not open file " << filename << "." << endl;
    return false;
  } // if

  return true;
} // Ckt::readOrder


//      Function : Ckt::writeOrder
//      Abstract : Write the current variable ordering to a file.
bool
Ckt::writeOrder(std::string &filename)
{
  if (filename.empty()) {
    return true;
  } // if

  std::ofstream outfile(filename.c_str());

  if (!outfile.is_open()) {
    cout << "Could not open file " << filename << "." << endl;
    return false;
  } // if

  ElIdVec order(_inputs.size(), 0);
  for (auto id : _inputs) {
    auto &el = _elements[id];
    BddIndex idx = el.getBdd().getIndex();
    order[idx-1] = id;
  } // for

  for (auto id : order) {
    auto &el = _elements[id];
    outfile << el.getName() << endl;
  } // for

  return true;
} // Ckt::writeOrder


//      Function : Ckt::printSizes
//      Abstract : Prinf number of nodes for each output BDD.
void
Ckt::printSizes()
{
  for (auto id : _outputs) {
    auto &el = _elements[id];
    cout << "Bdd for output "
	 << el.getName()
	 << " has "
	 << el.getBdd().countNodes()
	 << " nodes." << endl;
  } //for
  cout << endl;
} // Ckt::printSizes


