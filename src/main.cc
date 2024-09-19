//
//      File     : main.cc
//      Abstract : Test driver.
//

#include <Bdd.h>
#include <BddUtils.h>
#include <iostream>
using std::cout;
using std::endl;

void testMem();
void testOps();
void testProduct();
void testXor();
void testDnf();
void printDnf(Dnf &dnf);

#define VALIDATE(expr) cout << (expr ? "PASSED: " : "FAILED: ") << #expr << endl;

//      Function : main
//      Abstract : Driver to test minimal functionality.
int
main(int argc, char *argv[])
{
  testMem();
  testOps();
  testProduct();
  testXor();
  testDnf();
  return 0;
} // main


//      Function : testMem
//      Abstract : Test ability to create BDDs and verify expected
//      memory usage.
void
testMem()
{
  cout << "----------------------------------------------------------------"
       << endl;
  cout << "Memory Tests:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;

  {
    Bdd g;

    Bdd a = mgr.getLit(10);
    Bdd b = mgr.getLit(20);
    Bdd c = mgr.getLit(40);
    Bdd d = mgr.getLit(30);
    Bdd e = mgr.getLit(50);
    Bdd f = mgr.getLit(60);

    {
      Bdd g0 = b + ~c;
      Bdd g1 = a + b + ~c;
      cout << "g0 = b + ~c" << endl;
      cout << "g1 = a + b + ~c" << endl;
      VALIDATE(g0 <= g1);

      g0 = ((~a) + (~b) + (~c));
      cout << "g0 = ~a + ~b + ~c" << endl;
      VALIDATE(g0.countNodes() == 4);

      g1 = ~d + e + f;
      cout << "g1 = ~d + e + f\n";
      VALIDATE(g1.countNodes() == 4);
      g = g1 * g0;
      cout << "g = g1 * g0\n";
      VALIDATE(g.countNodes() == 9);
      VALIDATE(mgr.nodesAllocd() == 21);
      VALIDATE(mgr.gc(true) == 5);
      VALIDATE(mgr.nodesAllocd() == 16);
      mgr.reorder();
      VALIDATE(g.countNodes() == 7);
    } // scope 2
    mgr.gc(true);
    VALIDATE(mgr.nodesAllocd() == 13);

    g *= c;
    cout << "g = g * c\n";
    Bdd cube = g.cubeFactor();
    cout << "cube = cubeFactor(g)\n";
    VALIDATE(cube == c);
  } // scope 1
  VALIDATE(mgr.gc(true) == 37);
  VALIDATE(mgr.nodesAllocd() == 2);
  VALIDATE(mgr.checkMem());

  cout << endl;
} // testMem


//      Function : testOps
//      Abstract : Test basic operations.
void
testOps()
{
  cout << "----------------------------------------------------------------"
       << endl;
  cout << "Simple Op Tests:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd e = mgr.getLit(5);

  VALIDATE(~a == a.inv());
  VALIDATE(a*b == a.and2(b));
  VALIDATE(~(a*b) == a.nand2(b));
  VALIDATE(a+b == a.or2(b));
  VALIDATE(~(a+b) == a.nor2(b));
  VALIDATE((a^b) == a.xor2(b));
  VALIDATE((~a^b) == a.xnor2(b));

  Bdd F0 = a * b;
  Bdd F1 = ~(~a + ~b);
  cout << "F0 = a * b" << endl;
  cout << "F1 = ~(~a * ~b)" << endl;
  VALIDATE(F0 == F1);

  F0 = a.implies(b);
  F1 = (~a + b);
  cout << "F0 = a.implies(b)" << endl;
  cout << "F1 = (~a + b)" << endl;
  VALIDATE(F0 == F1);

  F0 = a ^ b;
  F1 = a * ~b + ~a * b;
  cout << "F0 = a ^ b" << endl;
  cout << "F1 = a * ~b + ~a * b" << endl;
  VALIDATE(F0 == F1);

  Bdd G = a + b + c + d;
  Bdd H = c + d + e;
  Bdd F = G * H;

  cout << "G = a + b + c + d" << endl;
  cout << "H = c + d + e" << endl;
  cout << "F = g * h" << endl;
  VALIDATE(F/H == G);

  F = a*c + b*~c;
  G = d*e;
  cout << "F = a*c + b*~c" << endl;
  cout << "G = d*e" << endl;
  VALIDATE(F.compose(c.getTopVar(), G) == a*d*e + b*(~d + ~e));

  cout << endl;
} // testOps


//      Function : testProduct
//      Abstract : Test findProduct utility.
void
testProduct()
{
  cout << "----------------------------------------------------------------"
       << endl;
  cout << "Test findProduct\n";
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);

  Bdd G = a + b + c + d;
  Bdd H = c + d + e + f;
  Bdd F = G * H;
  cout << "G = a + b + c + d" << endl;
  cout << "H = c + d + e + f" << endl;
  cout << "F = G * H" << endl;

  Bdd H2 = findProduct(F);
  Bdd G2 = F/H2;
  cout << "H2 = findProduct(F)" << endl;
  cout << "G2 = F/H2" << endl;

  VALIDATE(G == G2);
  VALIDATE(H == H2);

  F = a * b * c + d * e * f;
  H = findProduct(F);
  cout << "F = a * b * c + d * e * f" << endl;
  cout << "H = findProduct(f)" << endl;
  VALIDATE(H.isOne());

  cout << endl;
} // testProduct


//      Function : testXor
//      Abstract : Test findXor utility.
void
testXor()
{
  cout << "----------------------------------------------------------------"
       << endl;
  cout << "Test findXor\n";
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);

  Bdd G = a + b + c + d;
  Bdd H = c + d + e + f;
  Bdd F = G ^ H;
  cout << "G = a + b + c + d" << endl;
  cout << "H = c + d + e + f" << endl;
  cout << "F = G ^ H" << endl;

  Bdd H2 = findXor(F);
  Bdd G2 = F ^ H2;
  cout << "H2 = findXor(F)" << endl;
  cout << "G2 = F ^ H2" << endl;

  VALIDATE(F != G2);
  VALIDATE(F != H2);

  F = a * b * c + d * e * f;
  H = findXor(F);
  cout << "F = a * b * c + d * e * f" << endl;
  cout << "H = findXor(F)" << endl;
  VALIDATE(H.isZero());

  cout << endl;
} // testXor


//      Function : testDnf
//      Abstract : Test DNF extraction.
void
testDnf()
{
  cout << "----------------------------------------------------------------"
       << endl;
  cout << "Test extractDnf():" << endl;
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd F;
  Dnf dnf;

  // F = a*b + ~c;
  // cout << "F = a*b + ~c" << endl;
  // dnf = extractDnf(F);
  // printDnf(dnf);

  F = a*b*d + ~a*c*d + ~b*c*~d;
  cout << "F = a*b*d + ~a*c*d + ~b*c*~d" << endl;
  dnf = extractDnf(F);
  VALIDATE(F == dnf2Bdd(mgr, dnf));
  printDnf(dnf);
  VALIDATE(mgr.reorder() == 1);
  dnf = extractDnf(F);
  VALIDATE(F == dnf2Bdd(mgr, dnf));
  printDnf(dnf);

  cout << endl;
} // testDnf


//      Function : printDnf
//      Abstract : Print the sum-of-products expression.
void
printDnf(Dnf &dnf)
{
  cout << "--- DNF START ---" << endl;
  for (auto &term : dnf) {
    for (auto &literal : term) {
      cout << literal << " ";
    } // for each literal
    cout << endl;
  } // for each term
  cout << "--- DNF END -----" << endl;
} // printDnf

