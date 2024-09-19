#include <stdlib.h>
#include "Bdd.h"

#include "Ticker.h"
using namespace std;

//      Class    : QueensSolver
//      Abstract : Compute the BDD representing solutions to the
//      N-QueensSolver problem
class QueensSolver {
public:
  QueensSolver(int numQueensSolver) : _numQs(numQueensSolver) {}; // CTOR
  ~QueensSolver(); // DTOR

  QueensSolver(const QueensSolver &) = delete; // Copy CTOR
  QueensSolver &operator=(const QueensSolver &) = delete; // Copy assignment
  QueensSolver(QueensSolver &&) = delete; // Move CTOR
  QueensSolver &operator=(QueensSolver &&) = delete; // Move assignment

  void solve();
private:
  void buildVariables();
  void placeQueens();
  void addConstraints();
  void addConstraints(int row, int col);
  Bdd addRowConstraints(const int row, const int col);
  Bdd addColumnConstraints(const int row, const int col);
  Bdd addDiagonalConstraints(const int row, const int col);
  void printResults();

  BddLit toVarIndex(const int row, const int col) const {
    return row * _numQs + col + 1;
  } // toVarIndex

  int _numQs;
  BddMgr _mgr;
  Bdd **_vars;	// Variables for each position the board.
  Bdd _queens;	// Solutions.
}; // QueensSolver


//      Function : QueensSolver::~QueensSolver
//      Abstract : DTOR
QueensSolver::~QueensSolver()
{
  for (int row = 0; row < _numQs; ++row) {
    delete [] _vars[row];
  } // for
  delete [] _vars;
} // QueensSolver::~QueensSolver


//      Function : QueensSolver::solve
//      Abstract : Solve the n-queens problem using BDDs.
void
QueensSolver::solve()
{
  _queens = _mgr.getOne();

  buildVariables();
  placeQueens();
  addConstraints();
  printResults();

  _mgr.gc(true);
  _queens = _mgr.getOne();
  _mgr.gc(true);

  _mgr.printStats();
} // QueensSolver::solve


//      Function : QueensSolver::buildVariables
//      Abstract : Create variables for each square.
void
QueensSolver::buildVariables()
{
  _queens = _mgr.getOne();
  _vars = new Bdd*[_numQs];

  for (int row = 0; row < _numQs; ++row) {
    _vars[row] = new Bdd[_numQs];
  } // for

  for (int row = 0; row < _numQs; ++row) {
    for (int col = 0; col < _numQs; ++col) {
      _vars[row][col] = _mgr.getLit(toVarIndex(row, col));
    } // col
  } // for
} // QueensSolver::buildVariables


//      Function : QueensSolver::placeQueens
//      Abstract : For each row add the constraint that it has a
//      queen.
void
QueensSolver::placeQueens()
{
  cout << "Adding one queen to each row." << endl;
  for (int row = 0; row < _numQs; ++row) {
    Bdd inRow = _mgr.getZero();
    for (int col = 0; col < _numQs; ++col) {
      inRow += _vars[row][col];
    } // col
    _queens *= inRow;
  } // for
} // QueensSolver::placeQueens


//      Function : QueensSolver::addConstraints
//      Abstract : Add add constraints that no queens share a row,
//      column or diagonal.
void
QueensSolver::addConstraints()
{
  cout << "Adding only one queen per row/column/diagonal constraints." << endl;
  Ticker ticker(_numQs * _numQs);
  for (int row = 0; row < _numQs; ++row) {
    for (int col = 0; col < _numQs; ++col) {
      addConstraints(row, col);
      ticker.tick();
    } // col
  } // for
} // QueensSolver::addConstraints


//      Function : QueensSolver::addConstraints
//      Abstract : Add constraints that a queen at this position does
//      not share a row, column or diagonal with another queen.
void
QueensSolver::addConstraints(const int row, const int col)
{
  Bdd rowConstr, colConstr, diagConstr;

  rowConstr = addRowConstraints(row, col);
  colConstr = addColumnConstraints(row, col);
  diagConstr = addDiagonalConstraints(row, col);

  _queens *= rowConstr * colConstr * diagConstr;
} // QueensSolver::addConstraints


//      Function : QueensSolver::addRowConstraints
//      Abstract : Add constraints to ensure only one queen per row.
Bdd
QueensSolver::addRowConstraints(const int row, const int col)
{
  Bdd constr = _mgr.getOne();
  Bdd me = _vars[row][col];
  for (int c = 0; c < _numQs; ++c) {
    if (col != c) {
      constr *= me.implies(~_vars[row][c]);
    } // if
  } // for

  return constr;
} // QueensSolver::addRowConstraints


//      Function : QueensSolver::addColumnConstraints
//      Abstract : Add constraints to ensure only one queen per
//      column.
Bdd
QueensSolver::addColumnConstraints(const int row, const int col)
{
  Bdd constr = _mgr.getOne();
  Bdd me = _vars[row][col];
  for (int r = 0; r < _numQs; ++r) {
    if (row != r) {
      constr *= me.implies(~_vars[r][col]);
    } // if
  } // for

  return constr;
} // QueensSolver::addColumnConstraints


//      Function : QueensSolver::addDiagonalConstraints
//      Abstract : Add constraints to ensure only one queen per
//      diagonal.
Bdd
QueensSolver::addDiagonalConstraints(const int row, const int col)
{
  Bdd constr = _mgr.getOne();
  Bdd me = _vars[row][col];

  // No other queens in down diagonal.
  for (int c = 0; c < _numQs; ++c) {
    int r = row - col + c;
    if (r >= 0 && r < _numQs && row != r) {
      constr *= me.implies(~_vars[r][c]);
    } // if
  } // if

  // No other queens in up diagonal.
  for (int c = 0; c < _numQs; ++c) {
    int r = row + col - c;
    if (r >= 0 && r < _numQs && col != c) {
      constr *= me.implies(~_vars[r][c]);
    } // if
  } // for

  return constr;
} // QueensSolver::addDiagonalConstraints


//      Function : main
//      Abstract : Driver
int
main(int argc, char *argv[])
{
  int N = -1;
  if (argc == 2) {
    N = atoi(argv[1]);
    if (N <= 0) {
      cout << N << " is not a positive integer." << endl;
      return 1;
    } // if
  } else {
    cout << "Usage: queens N (N is a positive integer.)" << endl;
    return 1;
  } // if

  QueensSolver(N).solve();
} // main


//      Function : QueensSolver::printResults
//      Abstract : Print results.
void
QueensSolver::printResults()
{
  cout << _numQs <<"-Queens is " << (_queens.isZero() ? "UNSAT" : "SAT") << endl;
} // QueensSolver::printResults
