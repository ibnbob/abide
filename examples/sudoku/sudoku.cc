//
//      File     : sudoku.cc
//      Abstract : Sudoku solver using BDDs.
//

#include "Bdd.h"
#include "BddUtils.h"

#include <fstream>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

//      Class    : Sudoku
//      Abstract : Class for solving Sudoku puzzles.
class Sudoku {
public:
  Sudoku(std::string filename); // CTOR
  ~Sudoku() = default; // DTOR

  Sudoku(const Sudoku &) = delete; // Copy CTOR
  Sudoku &operator=(const Sudoku &) = delete; // Copy assignment
  Sudoku(Sudoku &&) = delete; // Move CTOR
  Sudoku &operator=(Sudoku &&) = delete; // Move assignment
  void readPuzzleConstraints();
  void buildGeneralConstraints();
  void printSolution();

 private:
  using Entry = std::tuple<int, int, int>;
  using Cell = std::tuple<int, int>;

  void buildRowConstraints();
  void buildColConstraints();
  void buildBoxConstraints();
  void buildBoxConstraint(int val, int i, int j);
  void buildCellConstraints();

  void readPuzzleConstraints(std::istream &strm);
  void addLine(std::istream &strm, int row);
  std::vector<int> parseLine(std::string &line);
  bool addEntry(int row, int col, int val);

  Bdd entryToVar(int row, int col, int val);
  Entry varToEntry(const Bdd var);

  int packEntry(int x, int y, int z);
  Entry unpackEntry(int e);
  Cell unpackCell(int c);

  // Data members.
  int _N = 9;
  int _M = 3;
  std::string _input;

  BddMgr _mgr;
  Bdd _solution;
}; // Sudoku


//      Function : Sudoku::Sudoku
//      Abstract : CTOR with input file.
Sudoku::Sudoku(std::string filename)
{
  _solution = _mgr.getOne();
  _input = filename;
} // Sudoku::Sudoku


//      Function : Sudoku::buildGeneralConstraints
//      Abstract : Build the initial constraints: Every number appears
//      in each row, column and box.
void
Sudoku::buildGeneralConstraints()
{
  buildBoxConstraints();
  buildRowConstraints();
  buildColConstraints();
  buildCellConstraints();
} // Sudoku::buildGeneralConstraints


//      Function : Sudoku::buildRowConstraints
//      Abstract : Add the constraint that no number appears in a
//      row twice.
void
Sudoku::buildRowConstraints()
{
  for (int val = 0; val < _N; ++val) {
    for (int row = 0; row < _N; ++row) {
      Bdd rowConstraint = _mgr.getOne();
      for (int col1 = 0; col1 < _N-1; ++col1) {
        Bdd var1 = entryToVar(row, col1, val);
        for (int col2 = col1+1; col2 < _N; ++col2) {
          Bdd var2 = entryToVar(row, col2, val);
          rowConstraint *= var1.nand2(var2);
        } // for column 2
      } // for column 1
      _solution *= rowConstraint;
    } // for each row
  } // for each value
} // Sudoku::buildRowConstraints


//      Function : Sudoku::buildColConstraints
//      Abstract : Add the constraint that no number appears in a
//      column twice.
void
Sudoku::buildColConstraints()
{
  for (int val = 0; val < _N; ++val) {
    for (int col = 0; col < _N; ++col) {
      Bdd colConstraint = _mgr.getOne();
      for (int row1 = 0; row1 < _N-1; ++row1) {
        Bdd var1 = entryToVar(row1, col, val);
        for (int row2 = row1+1; row2 < _N; ++row2) {
          Bdd var2 = entryToVar(row2, col, val);
          colConstraint *= var1.nand2(var2);
        } // for row 2
      } // for row 1
      _solution *= colConstraint;
    } // for each column
  } // for each value
} // Sudoku::buildColConstraints


//      Function : Sudoku::buildBoxConstraints
//      Abstract : Add the constraint the no number appears in each
//      MxM box twice.
void
Sudoku::buildBoxConstraints()
{
  for (int val = 0; val < _N; ++val) {
    for (int i = 0; i < _M; ++i) {
      for (int j = 0; j < _M; ++j) {
        buildBoxConstraint(val, i, j);
      } // for
    } // for
  } // for
} // Sudoku::buildBoxConstraints


//      Function : Sudoku::buildBoxConstraint
//      Abstract : Add contraint for this box.
void
Sudoku::buildBoxConstraint(int val, int i, int j)
{
  int row0 = i * _M;
  int col0 = j * _M;

  Bdd boxConstraint = _mgr.getOne();
  for (int cell1 = 0; cell1 < _N-1; ++cell1) {
    auto [row1, col1] = unpackCell(cell1);
    Bdd var1 = entryToVar(row0+row1, col0+col1, val);

    for (int cell2 = cell1+1; cell2 < _N; ++cell2) {
      auto [row2, col2] = unpackCell(cell2);
      Bdd var2 = entryToVar(row0+row2, col0+col2, val);
      boxConstraint *= var1.nand2(var2);
    } // for
  } // for

  _solution *= boxConstraint;
} // Sudoku::buildBoxConstraint


//      Function : Sudoku::buildCellConstraints
//      Abstract : Add constraint the each cell contains a number.
void
Sudoku::buildCellConstraints()
{
  for (int row = 0; row < _N; ++row) {
    for (int col = 0; col < _N; ++col) {
      Bdd cellConstraint = _mgr.getZero();
      for (int val = 0; val < _N; ++val) {
        Bdd var = entryToVar(row, col, val);
        cellConstraint += var;
      } // for
      _solution *= cellConstraint;
    } // for each column
  } // for each row
} // Sudoku::buildCellConstraints


//      Function : Sudoku::readPuzzleConstraints
//      Abstract : Read the puzzle constraints either from a file or
//      stdin.
void
Sudoku::readPuzzleConstraints()
{
  if (_input.empty()) {
    readPuzzleConstraints(std::cin);
  } else {
    std::ifstream ifile;
    ifile.open(_input);
    if (ifile) {
      readPuzzleConstraints(ifile);
    } else {
      cerr << "Could not open file: "
           << _input << endl;
    } // if
  } // if
} // Sudoku::readPuzzleConstraints


//      Function : Sudoku::readPuzzleConstraints
//      Abstract : Read constraints for a puzzle from stdin.
void
Sudoku::readPuzzleConstraints(std::istream &strm)
{
  for (int row = 0; row < _N; ++row) {
    addLine(strm, row);
  } //
} // Sudoku::readPuzzleConstraints


//      Function : Sudoku::addLine
//      Abstract : Read and add a line of constraints.
void
Sudoku::addLine(std::istream &strm, int row)
{
  std::string line;
  std::vector<int> entries;
  do {
    std::getline(strm, line);
    entries = parseLine(line);
  } while (entries.size() != _N);

  for (int col = 0; col < _N; ++col) {
    int val = entries[col];
    if (val) {
      Bdd var = entryToVar(row, col, val-1);
      _solution *= var;
    } // if
  } // for
} // Sudoku::addLine


//      Function : Sudoku::parseLine
//      Abstract :
std::vector<int>
Sudoku::parseLine(std::string &line)
{
  std::vector<int> entries;
  int val = 0;
  for (unsigned char c : line) {
    if (std::isspace(c)) {
      if (val > 0) {
        entries.push_back(val);
        val = 0;
      } // if
    } else if (c == '.' ||
               c == '0') {
      if (val > 0) {
        entries.push_back(val);
        val = 0;
      } // if
      entries.push_back(0);
    } else if (std::isdigit(c)) {
      val += c - '0';
    } // if
  } // for

  if (val > 0) {
    entries.push_back(val);
    val = 0;
  } // if

  return entries;
} // Sudoku::parseLine


//      Function : Sudoku::addEntry
//      Abstract : Use this entry to restrict the solution
//      space. Return true if a solution still exists.
bool
Sudoku::addEntry(int row, int col, int val)
{
  Bdd var = entryToVar(row, col, val);
  _solution *= var;
  return ! _solution.isZero();
} // Sudoku::addEntry


//      Function : Sudoku::entryToVar
//      Abstract : Convert an entry into its BDD variable.
Bdd
Sudoku::entryToVar(int row, int col, int val)
{
  BddVar var = packEntry(val, row, col) + 1;
  return _mgr.getLit(var);
} // Sudoku::entryToVar


//      Function : Sudoku::varToEntry
//      Abstract :
Sudoku::Entry
Sudoku::varToEntry(Bdd var)
{
  assert(var.isPosLit());

  int n = var.getTopVar() - 1;
  auto [val, row, col] = unpackEntry(n);

  return {row, col, val};
} // Sudoku::varToEntry


//      Function : Sudoku::packEntry
//      Abstract : Creates a packed integer for an entry.
int
Sudoku::packEntry(int x, int y, int z)
{
  return x*_N *_N + y*_N +z;
} // Sudoku::packEntry


//      Function : Sudoku::unpackEntry
//      Abstract : Unpacks a packed entry integer.
Sudoku::Entry
Sudoku::unpackEntry(int p)
{
  int z = p%_N;
  p -= z; p/= _N;
  int y = p %_N;
  p -= y; p/= _N;
  int x = p;

  return {x, y, z};
} // Sudoku::unpackEntry


//      Function : Sudoku::unpackCell
//      Abstract : Unpacks an integer into an M x M cell.
Sudoku::Cell
Sudoku::unpackCell(int c)
{
  int col = c % _M;
  int row = (c-col) / _M;

  return {row, col};
} // Sudoku::unpackCell


//      Function : Sudoku::printSolution
//      Abstract : Print one solution.
void
Sudoku::printSolution()
{
  std::vector< std::vector<int> > grid;
  grid.resize(_N);
  for (auto &row : grid) {
    row.resize(_N, 0);
  } // for

  Bdd cube = _solution.oneCube();
  while (!cube.isOne()) {
    Bdd hi = cube.getThen();
    Bdd lo = cube.getElse();
    assert(hi.isZero() || lo.isZero());
    if (lo.isZero()) {
      Bdd var = cube.getIf();
      auto [row, col, val] = varToEntry(var);
      assert(grid[row][col] == 0);
      grid[row][col] = val+1;
      cube = hi;
    } else {
      cube = lo;
    } // if
  } // while

  for (auto row : grid) {
    for (auto val : row) {
      cout << val << " ";
    } // for
    cout << endl;
  } // for
} // Sudoku::printSolution


//      Function : main
//      Abstract : Driver
int
main(int argc, char *argv[])
{
  std::string input;
  if (argc == 2) {
    input = argv[1];
  } // if
  Sudoku sudoku(input);
  sudoku.readPuzzleConstraints();
  sudoku.buildGeneralConstraints();
  sudoku.printSolution();

  return 0;
} // main

