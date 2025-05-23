//
//      File     : sudoku.cc
//      Abstract : Sudoku solver using BDDs.
//

#include "colors.h"
#include "Args.h"
#include "Timer.h"
#include <Bdd.h>
#include <BddUtils.h>

using namespace abide;

#include <fstream>
#include <sys/stat.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

namespace {
const int MAX_SOLUTIONS = 4;
} // anonymous


//      Function : fileExists
//      Abstract : Test if file exists.
bool
fileExists(const std::string &fname)
{
  struct stat buffer;
  return (stat(fname.c_str(), &buffer) == 0);
} // fileExists


//      Struct   : GenConstraint
//      Abstract : Represents a general constraint, i.e., a row,
//      column or box constraint.
struct GenConstraint {
public:
  GenConstraint(int row, int col) :
    _row(row),
    _col(col) {};     // CTOR
  ~GenConstraint() {}; // DTOR

  GenConstraint(const GenConstraint &) = delete; // Copy CTOR
  GenConstraint &operator=(const GenConstraint &) = delete; // Copy assignment
  GenConstraint(GenConstraint &&) = default; // Move CTOR
  GenConstraint &operator=(GenConstraint &&) = delete; // Move

  int _row;
  int _col;
}; // GenConstraint

using ConstrVec = std::vector<GenConstraint>;
using Constraints = std::vector<ConstrVec>;

//      Class    : Sudoku
//      Abstract : Class for solving Sudoku puzzles.
class Sudoku {
public:
  Sudoku(std::string input,
         std::string output); // CTOR
  ~Sudoku() = default; // DTOR

  Sudoku(const Sudoku &) = delete; // Copy CTOR
  Sudoku &operator=(const Sudoku &) = delete; // Copy assignment
  Sudoku(Sudoku &&) = delete; // Move CTOR
  Sudoku &operator=(Sudoku &&) = delete; // Move assignment
  void readPuzzleConstraints();
  void buildCommonConstraints();
  void printSolutions();
  void writePuzzleConstraints();

 private:
  using Entry = std::tuple<int, int, int>;
  using Cell = std::tuple<int, int>;

  void readPuzzleConstraints(std::istream &strm);
  void addLine(std::istream &strm, int row);
  std::vector<int> parseLine(std::string &line);
  bool addEntry(int row, int col, int val);
  bool fromFile() { return _input.size() && fileExists(_input); };
  bool isComment(std::string &line) {
    return line[0] == '#'; };

  void gatherExclusionConstraints();
  void gatherRowConstraints();
  void gatherColConstraints();
  void gatherBoxConstraints();
  void gatherBoxConstraint(int i, int j);

  void buildExclusionConstraints();
  void buildRowConstraints(int row);
  void buildColConstraints(int col);
  void buildBoxConstraints(int row, int col);
  void buildBoxConstraints(int row, int col, int val);

  void buildCellConstraints();

  void printSolution(Bdd cube);

  Bdd entryToVar(int row, int col, int val);
  Entry varToEntry(const Bdd var);

  int packEntry(int x, int y, int z);
  Entry unpackEntry(int e);
  Cell unpackCell(int c);

  // Data members.
  int _N = 9;
  int _M = 3;
  std::string _input;
  std::string _output;

  Constraints _constraints;

  std::vector< std::vector<int> > _grid;
  BddMgr _mgr;
  Bdd _solution;
}; // Sudoku


//      Function : Sudoku::Sudoku
//      Abstract : CTOR with input file.
Sudoku::Sudoku(std::string input,
               std::string output) :
  _mgr(_N*_N*_N)
{
  _solution = _mgr.getOne();
  _input = input;
  _output = output;
  _grid.resize(_N);
  for (auto &row : _grid) {
    row.resize(_N, 0);
  } // for

  _constraints.resize(_N+1);
} // Sudoku::Sudoku


//      Function : Sudoku::readPuzzleConstraints
//      Abstract : Read the puzzle constraints either from a file or
//      stdin.
void
Sudoku::readPuzzleConstraints()
{
  if (_input.empty() || !fileExists(_input)) {
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
  } // for
  cout << GREEN << "\nSolving ..." << NORMAL << endl;
} // Sudoku::readPuzzleConstraints


//      Function : Sudoku::addLine
//      Abstract : Read and add a line of constraints.
void
Sudoku::addLine(std::istream &strm, int row)
{
  std::string line;
  std::vector<int> entries;
  bool readFile = fromFile();
  while (true) {
    std::cout << row+1 << ": ";
    std::getline(strm, line);
    entries = parseLine(line);
    if (readFile) {
      std::cout << line << std::endl;
    } // if

    if (entries.size() == _N) {
      break;
    } else if (! isComment(line)) {
      std::cout << '\a' << std::flush;
    } // if
  } // while

  for (int col = 0; col < _N; ++col) {
    int val = entries[col];
    if (val) {
      _grid[row][col] = -val;
      Bdd var = entryToVar(row, col, val-1);
      _solution *= var;
    } else {
      _grid[row][col] = 0;
    } // if
  } // for
} // Sudoku::addLine


//      Function : Sudoku::parseLine
//      Abstract :
std::vector<int>
Sudoku::parseLine(std::string &line)
{
  std::vector<int> entries;
  if (! isComment(line)) {
    for (unsigned char c : line) {
      if (std::isspace(c)) {
      } else if (c == '.' ||
                 c == '0') {
        entries.push_back(0);
      } else if (std::isdigit(c)) {
        int val = c - '0';
        entries.push_back(val);
      } // if
    } // for
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


//      Function : Sudoku::buildCommonConstraints
//      Abstract : Build the constraints common to every
//      puzzle. Exclusion constraints demand that a number appears at
//      most once in any row, column or box. Cell constraints demand
//      the every cell contains a number.
void
Sudoku::buildCommonConstraints()
{
  gatherExclusionConstraints();
  buildExclusionConstraints();
  buildCellConstraints();
} // Sudoku::buildCommonConstraints


//      Function : Sudoku::gatherExclusionConstraints
//      Abstract : Gather all of the exclusion constraints sorted by
//      the number zeroes.
void
Sudoku::gatherExclusionConstraints()
{
  gatherRowConstraints();
  gatherColConstraints();
  gatherBoxConstraints();
} // Sudoku::gatherExclusionConstraints


//      Function : Sudoku::gatherRowConstraints
//      Abstract : Gather all the row constraints
void
Sudoku::gatherRowConstraints()
{
  for (int row = 0; row < _N; ++row) {
    int zeroes = 0;
    for (int col = 0; col < _N ; ++col) {
      if (_grid[row][col] == 0) {
        ++zeroes;
      } // for
    } // for
    _constraints[zeroes].emplace_back(row, -1);
  } // for
} // Sudoku::gatherRowConstraints


//      Function : Sudoku::gatherColConstraints
//      Abstract : Gather all the column constraints
void
Sudoku::gatherColConstraints()
{
  for (int col = 0; col < _N ; ++col) {
    int zeroes = 0;
    for (int row = 0; row < _N; ++row) {
      if (_grid[row][col] == 0) {
        ++zeroes;
      } // for
    } // for
    _constraints[zeroes].emplace_back(-1, col);
  } // for
} // Sudoku::gatherColConstraints


//      Function : Sudoku::gatherBoxConstraints
//      Abstract : Gather all the box constraints.
void
Sudoku::gatherBoxConstraints()
{
  for (int i = 0; i < _M; ++i) {
    for (int j = 0; j < _M; ++j) {
      gatherBoxConstraint(i, j);
    } // for
  } // for
} // Sudoku::gatherBoxConstraints


//      Function : Sudoku::gatherBoxConstraint
//      Abstract : Add this box constraint.
void
Sudoku::gatherBoxConstraint(int i, int j)
{
  int row0 = i * _M;
  int col0 = j * _M;
  int zeroes = 0;

  for (int cell = 0; cell < _N-1; ++cell) {
    auto [row1, col1] = unpackCell(cell);
    if (_grid[row0+row1][col0+col1] == 0) {
      ++zeroes;
    } // if
  } // for

  _constraints[zeroes].emplace_back(row0, col0);
} // Sudoku::gatherBoxConstraint


//      Function : Sudoku::buildExclusionConstraints
//      Abstract : Build all the gathered exclusion constraints.
void
Sudoku::buildExclusionConstraints()
{
  for (auto &cvector : _constraints) {
    for (auto &constr : cvector) {
      if (constr._col < 0) {
        buildRowConstraints(constr._row);
      } else if (constr._row < 0) {
        buildColConstraints(constr._col);
      } else {
        buildBoxConstraints(constr._row, constr._col);
      } // if
    } // for
  } // for
} // Sudoku::buildExclusionConstraints


//      Function : Sudoku::buildRowConstraints
//      Abstract : Add the constraint that no number appears in this
//      row twice.
void
Sudoku::buildRowConstraints(int row)
{
  for (int val = 0; val < _N; ++val) {
    Bdd rowConstraint = _mgr.getOne();
    for (int col1 = 0; col1 < _N-1; ++col1) {
      Bdd var1 = entryToVar(row, col1, val);
      for (int col2 = col1+1; col2 < _N; ++col2) {
        Bdd var2 = entryToVar(row, col2, val);
        rowConstraint *= var1.nand2(var2);
      } // for column 2
    } // for column 1
    _solution *= rowConstraint;
  } // for each value
} // Sudoku::buildRowConstraints


//      Function : Sudoku::buildColConstraints
//      Abstract : Add the constraint that no number appears in a
//      column twice.
void
Sudoku::buildColConstraints(int col)
{
  for (int val = 0; val < _N; ++val) {
    Bdd colConstraint = _mgr.getOne();
    for (int row1 = 0; row1 < _N-1; ++row1) {
      Bdd var1 = entryToVar(row1, col, val);
      for (int row2 = row1+1; row2 < _N; ++row2) {
        Bdd var2 = entryToVar(row2, col, val);
        colConstraint *= var1.nand2(var2);
      } // for row 2
    } // for row 1
    _solution *= colConstraint;
  } // for each value
} // Sudoku::buildColConstraints


//      Function : Sudoku::buildBoxConstraints
//      Abstract : Add the constraint the no number appears in each
//      MxM box twice.
void
Sudoku::buildBoxConstraints(int row, int col)
{
  for (int val = 0; val < _N; ++val) {
    buildBoxConstraints(row, col, val);
  } // for
} // Sudoku::buildBoxConstraints


//      Function : Sudoku::buildBoxConstraints
//      Abstract : Add contraint for this box.
void
Sudoku::buildBoxConstraints(int row, int col, int val)
{
  int row0 = row;
  int col0 = col;

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
} // Sudoku::buildBoxConstraints


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


//      Function : Sudoku::printSolutions
//      Abstract : Print solutions.
void
Sudoku::printSolutions()
{
  Bdd cube = _solution.oneCube();

  cout << endl;
  if (cube != _solution) {
    cout << "Puzzle has multiple solutions.\n" << endl;
  } else if (cube.isZero()) {
    cout << BOLD << RED << "Puzzle has no solutions." << NORMAL << endl;
    return;
  } // if

  int numSolutions = 0;
  while (!cube.isZero() && numSolutions < MAX_SOLUTIONS) {
    printSolution(cube);
    _solution *= (~cube);
    cube = _solution.oneCube();
    cout << endl;
    ++numSolutions;
  } // while

  if (numSolutions > 1) {
    if (!cube.isZero() && numSolutions >= MAX_SOLUTIONS) {
      cout << "Printed " << numSolutions << " solutions.";
    } else {
      cout << "Found " << numSolutions << " solutions.";
    } // if
    cout << endl;
  } // if
} // Sudoku::printSolutions


//      Function : Sudoku::printSolution
//      Abstract : Print the solution represented by this cube.
void
Sudoku::printSolution(Bdd cube)
{
  while (!cube.isOne()) {
    Bdd hi = cube.getThen();
    Bdd lo = cube.getElse();
    assert(hi.isZero() || lo.isZero());
    if (lo.isZero()) {
      Bdd var = cube.getIf();
      auto [row, col, val] = varToEntry(var);
      if (_grid[row][col] == 0) {
        _grid[row][col] = val+1;
      } // if

      cube = hi;
    } else {
      cube = lo;
    } // if
  } // while

  for (auto &row : _grid) {
    for (auto &val : row) {
      if (val < 0) {
        cout << BOLD << RED << -val << " ";
      } else {
        cout << NORMAL << val << " ";
      } // if
    } // for
    cout << endl;
  } // for
  cout << NORMAL;
} // Sudoku::printSolution


//      Function : Sudoku::writePuzzleConstraints
//      Abstract : If a filename was specified on the command line,
//      and the file was not used as input, then write a file with
//      this puzzle's constraints.
void
Sudoku::writePuzzleConstraints()
{
  if (!_output.empty()) {
    std::ofstream ofile;
    ofile.open(_output);

    for (auto &row : _grid) {
      for (auto &val : row) {
        if (val < 0) {
          ofile << -val << " ";
        } else {
          ofile << ". ";
        } // if
      } // for
      ofile << endl;
    } // for
  } // if
} // Sudoku::writePuzzleConstraints


//      Struct    : SudokuArgs
//      Abstract : Argument parsing struct.
struct SudokuArgs : public argparse::Args {
  std::optional<std::string> &input =
    kwarg("i,input", "name of optional input file.");
  std::optional<std::string> &output =
    kwarg("o,output", "name of optional output file.");

  void prolog() override {
    std::cout << "Solve a Sudoku puzzle instance."
              << std::endl;
  } // prolog

  void epilog() override {
    std::cout << R"(
Solve a Sudoku puzzle instance. If an input file is specified and it
exists, then the problem specification is read from there. Otherwise,
input is taken from the terminal. Legal input is nine lines of nine
characters from the set {'.', '0', ..., '9'}. White space is ignored.

If an output file is specified, then the problem specification is written
to that file. This file may then be used as an input file.
)" << std::endl;
  } // epilog
}; // SudokuArgs

//      Function : main
//      Abstract : Driver
int
main(int argc, char *argv[])
{
  Timer timer("sudoku");
  auto args = argparse::parse<SudokuArgs>(argc, argv);

  std::string input;
  std::string output;
  if (args.input) {
    input = args.input.value();
  } // if
  if (args.output) {
    output = args.output.value();
  } // if
  Sudoku sudoku(input, output);
  sudoku.readPuzzleConstraints();
  sudoku.buildCommonConstraints();
  sudoku.printSolutions();
  sudoku.writePuzzleConstraints();

  return 0;
} // main

