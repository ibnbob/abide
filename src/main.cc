//
//      File     : main.cc
//      Abstract : Test driver.
//

#include <Bdd.h>
#include <BddUtils.h>
#include <iostream>
using std::cout;
using std::endl;

using namespace abide;

void testMemBasic();
void testOutOfMem();
void testOutOfMem1();
void testOutOfMem2();
void testReorder();
void testOps();
void testSupport();
void testAndExists();
void testRestrict();
void testCompose();
void testIte();
void testProduct();
void testXor();
void testDnf();
void testMisc();

void printDnf(Dnf &dnf);
void printCube(Bdd cube);

#define VALIDATE(expr) cout << (expr ? "PASSED" : "FAILED") << " @ " << __LINE__ << ": " << #expr << endl;

//      Function : main
//      Abstract : Driver to test minimal functionality.
int
main()
{
  testMemBasic();
  testOutOfMem();
  testReorder();
  testOps();
  testSupport();
  testAndExists();
  testRestrict();
  testCompose();
  testIte();
  testProduct();
  testXor();
  testDnf();
  testMisc();

  return 0;
} // main


//      Function : testMemBasic
//      Abstract : Test ability to create BDDs and verify expected
//      memory usage.
void
testMemBasic()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Memory Tests:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr(48);

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

      mgr.lockGC();
      mgr.lockGC();
      VALIDATE(mgr.gc(true, true) == 0);
      mgr.unlockGC();
      VALIDATE(mgr.gc(true, true) == 0);
      mgr.unlockGC();
      VALIDATE(mgr.gc(true, true) == 5);
      VALIDATE(mgr.nodesAllocd() == 16);
      mgr.reorder(true);
      VALIDATE(g.countNodes() == 7);
    } // scope 2
    mgr.gc(true);
    VALIDATE(mgr.nodesAllocd() == 13);

    g *= c;
    cout << "g = g * c\n";
    Bdd cube = g.cubeFactor();
    cout << "cube = cubeFactor(g)\n";

    VALIDATE(cube == c);

    Bdd one = mgr.getOne();
    cube = one.cubeFactor();
    cout << "cube = one.cubeFactor()\n" << endl;
    VALIDATE(cube == one);
  } // scope 1
  VALIDATE(mgr.gc(true) == 12);
  VALIDATE(mgr.nodesAllocd() == 2);
  VALIDATE(mgr.checkMem());

  cout << endl;
} // testMemBasic


//      Function : testOutOfMem
//      Abstract : Test that exhausting max nodes is handled
//      correctly.
void
testOutOfMem()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Out-of-memory Tests:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;
  testOutOfMem1();
  testOutOfMem2();
} // testOutOfMem


//      Function : testOutOfMem1
//      Abstract : Tests for AND and XOR.
void
testOutOfMem1()
{
  BddMgr mgr;
  BddVec vars;
  const int M = 5;
  const int N = 1 << M;

  vars.push_back(Bdd());
  for (int idx = 1; idx < N+1; ++idx) {
    vars.push_back(mgr.getLit(idx));
  } // for

  Bdd sum = mgr.getZero();
  for (int idx = 1; idx < (N>>1)+1; ++idx) {
    Bdd prod = vars[idx] * vars[idx + (N>>1)];
    sum += prod;
  } // for

  cout << "Size: " << sum.countNodes() << endl;
  VALIDATE(sum.countNodes() == 131071);

  sum = mgr.getZero();
  mgr.gc(true);
  mgr.setMaxNodes(1024);
  for (int idx = 1; idx < (N>>1)+1; ++idx) {
    Bdd prod = vars[idx] * vars[idx + (N>>1)];
    sum += prod;
  } // for
  VALIDATE(!sum.valid());

  sum = mgr.getZero();
  mgr.gc(true);
  mgr.setMaxNodes(1024);
  for (int idx = 1; idx < (N>>1)+1; ++idx) {
    Bdd prod = vars[idx] * vars[idx + (N>>1)];
    sum ^= prod;
  } // for
  VALIDATE(!sum.valid());
} // testOutOfMem1


//      Function : testOutOfMem2
//      Abstract : Tests for restrict and ite.
void
testOutOfMem2()
{
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);

  Bdd F = a*f + b*e + c*d;
  cout << "F = a*f + b*e + c*d" << endl;
  mgr.gc(true);
  mgr.setMaxNodes(mgr.nodesAllocd());
  F = F / (a * d);
  cout << "F = F / (a * d)" << endl;
  VALIDATE(!F.valid());

  mgr.setMaxNodes(1<<20);
  F = a*f + b*e + c*d;
  cout << "F = a*f + b*e + c*d" << endl;
  mgr.gc(true);
  Bdd C = a * d;
  cout << "C = a * d" << endl;
  mgr.gc(true);
  mgr.setMaxNodes(mgr.nodesAllocd()+1);
  F = F / C;
  cout << "F = F / C" << endl;
  VALIDATE(!F.valid());

  mgr.setMaxNodes(1<<20);
  F = a * b + ~c;
  Bdd G1 = b + e * f;
  Bdd G2 = d * ~e + ~f;
  cout << "F = a * b + ~c" << endl;
  cout << "G1 = b + e * f" << endl;
  cout << "G2 = d * ~e + ~f" << endl;
  mgr.gc(true);
  mgr.setMaxNodes(mgr.nodesAllocd()+1);
  F = mgr.ite(F, G1, G2);
  cout << "F = mgr.ite(F, G1, G2)" << endl;
  VALIDATE(!F.valid());
} // testOutOfMem2


//      Function : testReorder
//      Abstract : Test reordering on large BDDs
void
testReorder()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Reorder Tests:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;

  BddMgr mgr(16, 163855);
  BddVec vars;
  const int M = 5;
  const int N = 1 << M;

  vars.push_back(Bdd());
  for (int idx = 1; idx < N+1; ++idx) {
    vars.push_back(mgr.getLit(idx));
  } // for

  Bdd sum = mgr.getZero();
  for (int idx = 1; idx < (N>>1)+1; ++idx) {
    Bdd prod = vars[idx] * vars[idx + (N>>1)];
    sum += prod;
  } // for

  cout << "Size: " << sum.countNodes() << endl;
  VALIDATE(sum.countNodes() == 131071);
  VALIDATE(mgr.checkMem());
  mgr.reorder(true);
  cout << "Size: " << sum.countNodes() << endl;
  VALIDATE(sum.countNodes() == 33);
  VALIDATE(mgr.checkMem());

  Bdd sum2 = mgr.getZero();
  for (int idx = 1; idx < (N>>1)+1; ++idx) {
    Bdd prod = vars[idx] * vars[idx + (N>>1)];
    sum2 += prod;
  } // for

  VALIDATE(sum == sum2);
  VALIDATE(mgr.checkMem());
} // testReorder


//      Function : testOps
//      Abstract : Test basic operations.
void
testOps()
{
  cout << "\n----------------------------------------------------------------"
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
  Bdd f = mgr.getLit(6);
  Bdd g = mgr.getLit(7);
  Bdd h = mgr.getLit(8);

  VALIDATE(~a == a.inv());
  VALIDATE(a*b == a.and2(b));
  VALIDATE(~(a*b) == a.nand2(b));
  VALIDATE(a+b == a.or2(b));
  VALIDATE(~(a+b) == a.nor2(b));
  VALIDATE((a^b) == a.xor2(b));
  VALIDATE((~a^b) == a.xnor2(b));

  Bdd h1 = mgr.getIthLit(8);
  VALIDATE(h == h1);

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

  F = (a + b) * (~c) * (d + e) * f * (g + h);
  G = F.cubeFactor();
  cout << "F = (a + b) * c * (d + e) * f * (g + h)" << endl;
  cout << "G = F.cubeFactor()" << endl;
  VALIDATE(G == ~c*f);

  F = (~a + ~b) * (c + d);
  G = F.oneCube();
  cout << "F = (~a + ~b) * (c + d)" << endl;
  cout << "G = F.oneCube()" << endl;
  VALIDATE(G == a*~b*c);

  cout << endl;
} // testOps


//      Function : test Support
//      Abstract : Test support functions.
void
testSupport()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Test Support\n";
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);
  Bdd g = mgr.getLit(7);

  Bdd F = a * (b + ~c);
  Bdd G = d ^ e ^ f ^ g;
  Bdd H = b * f + c * e + (a^g);

  VALIDATE(F.supportSize() == 3);
  VALIDATE(G.supportSize() == 4);
  VALIDATE(H.supportSize() == 6);

  BddVec set1{F};
  BddVec set2{F, G};
  const BddVec set3{F, H};

  BddVarVec vec1 = mgr.supportVec(set1);
  BddVarVec vec2 = mgr.supportVec(set2);
  BddVarVec vec3 = mgr.supportVec(set3);

  VALIDATE(vec1.size() == 3);
  VALIDATE(vec2.size() == 7);
  VALIDATE(vec3.size() == 6);

  Bdd supp = mgr.supportCube(set1);
  BddVec suppVec{supp};
  VALIDATE(mgr.countNodes(suppVec) == vec1.size()+1);
} // testSupport


//      Function : testAndExists
//      Abstract : Test andExists operation.
void
testAndExists()
{
  cout << "----------------------------------------------------------------"
       << endl;
  cout << "Test andExists\n";
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr;
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);
  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);
  Bdd g = mgr.getLit(7);

  {
    Bdd G1 = e.xnor2(a * b);
    Bdd G2 = f.xnor2(c + e);
    Bdd G3 = g.xnor2(d * f);

    cout << "G1 = e.xnor2(a * b)" << endl;
    cout << "G2 = f.xnor2(c + e)" << endl;
    cout << "G3 = g.xnor2(d * f)" << endl;

    Bdd H1 = g.xnor2(d*(c+(a*b)));
    Bdd H2 = mgr.andExists(G1*G2, G3, e*f);
    Bdd H3 = G1.andExists(G2*G3, e*f);

    cout << "H1 = g.xnor2(d*(c+(a*b)))" << endl;
    cout << "H2 = mgr.andExists(G1*G2, G3, e*f)" << endl;
    cout << "H3 = mgr.andExists(G1, G2*G3, e*f)" << endl;

    VALIDATE(H1 == H2);
    VALIDATE(H1 == H3);
  }

  Bdd F1 = (b^c^d);
  Bdd F2 = (c^(e+f));
  Bdd cube = a*c;

  mgr.setMaxNodes(42);

  Bdd F = F1.andExists(F2, cube);
  F = F1.andExists(~F1, cube);

  F1 = a + ~c;
  F2 = ~a + b;
  cube = c;
  F = F1.andExists(F2, cube);
} // testAndExists


//      Function : testRestrict
//      Abstract : Tickle hard to reach line of BddImplCalc.cc
void
testRestrict()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Test Restrict:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;

  BddMgr mgr(16, 115); // Only allow 115 BDD nodes!

  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);

  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);
  Bdd g = mgr.getLit(7);
  Bdd h = mgr.getLit(8);

  mgr.lockGC();
  Bdd F = a*e + b*f + c*g + d*h;
  F = a*h + b*g + c*f + d*e;
  F = (a+e)*(b+f)*(c+g)*(d+h);
  F = a*e + b*f + c*g + d*h;
  mgr.unlockGC();

  Bdd G = F/(g*h);

  Bdd i = mgr.getLit(9);
  Bdd j = mgr.getLit(10);

  mgr.lockGC();
  F = (a*b + ~e + c*(i^j))*((d*j)^(b+e));
  cout << F.countNodes() << endl;
  mgr.checkMem();
  G = (a+e)^(c*f);
  cout << F.countNodes() << endl;
  mgr.checkMem();
  mgr.unlockGC();

  F.compose(d.getTopVar(), G);
  cout << F.countNodes() << endl;
  mgr.checkMem();

} // testRestrict


//      Function : testCompose
//      Abstract : Tickle hard to reach line of BddImplCalc.cc
void
testCompose()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Tests Compose:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;

  BddMgr mgr(16, 54); // Only allow 54 BDD nodes!

  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd d = mgr.getLit(4);

  Bdd e = mgr.getLit(5);
  Bdd f = mgr.getLit(6);
  Bdd g = mgr.getLit(7);
  Bdd h = mgr.getLit(8);

  Bdd i = mgr.getLit(9);
  Bdd j = mgr.getLit(10);

  mgr.lockGC();
  Bdd F = (a*b + ~e + c*(i^j))*((d*j)^(b+e));
  cout << F.countNodes() << endl;
  mgr.checkMem();
  Bdd G = (a+e)^(c*f);
  cout << G.countNodes() << endl;
  mgr.checkMem();
  mgr.unlockGC();

  F = F.compose(d.getTopVar(), G);
  cout << F.countNodes() << endl;
  mgr.checkMem();

} // testCompose


//      Function : testIte
//      Abstract : Provide line coverage for ite() function.
void
testIte()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Tests ite:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;

  BddMgr mgr;

  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);

  Bdd F = a^b;
  Bdd G = b^c;
  Bdd H = mgr.ite(G, F, ~F);
  cout << "F = a^b" << endl;
  cout << "G = b^c" << endl;
  cout << "H = mgr.ite(G, F, ~F)" << endl;
  VALIDATE(H == (~a^c));
} // testIte


//      Function : testProduct
//      Abstract : Test findProduct utility.
void
testProduct()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Test findProduct()\n";
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

  F = a^b^c^d;
  H = findProduct(F);
  cout << "F = a^b^c^d" <<endl;
  cout << "H = findProduct(f)" << endl;
  VALIDATE(H.isOne());

  cout << endl;
} // testProduct


//      Function : testXor
//      Abstract : Test findXor utility.
void
testXor()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Test findXor()\n";
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

  VALIDATE(F.supportCube() <= G2.supportCube());
  VALIDATE(F.supportCube() <= H2.supportCube());

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
  cout << "\n----------------------------------------------------------------"
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


//      Function : testMisc
//      Abstract : Tidbits to increase line coverage.
void
testMisc()
{
  cout << "\n----------------------------------------------------------------"
       << endl;
  cout << "Test Miscellaneous:" << endl;
  cout << "----------------------------------------------------------------"
       << endl;
  BddMgr mgr(2);
  Bdd a = mgr.getLit(1);
  Bdd b = mgr.getLit(2);
  Bdd c = mgr.getLit(3);
  Bdd dummy = mgr.getLit(4);
  Bdd d = mgr.getIthLit(4);
  Bdd bad = mgr.getIthLit(32);
  VALIDATE(!bad.valid());

  VALIDATE(a.isPosLit());
  VALIDATE((~b).isNegLit());
  Bdd F = a + b;
  cout << "F = a + b" << endl;
  VALIDATE(!F.isPosLit());
  VALIDATE(!F.isNegLit());

  b = ~b;
  F = a^b^c;
  cout << "F = a^b^c" << endl;
  VALIDATE(F.getIf() == a);
  VALIDATE(F.getThen() == (~b^c));
  VALIDATE(F.getElse() == (b^c));

  F.print();
  F = (a + b) * (c + d);


  BddFnSet fns;

  Bdd F1 = a+c;
  fns.insert(F1);
  Bdd F2 = a*b;
  fns.insert(F2);
  VALIDATE(fns.size() == 2);
  fns.insert(F2);
  VALIDATE(fns.size() == 2);
  fns.erase(F);
  VALIDATE(fns.size() == 2);
  fns.erase(F2);
  VALIDATE(fns.size() == 1);

  VALIDATE(F.numRefs() == 1);
  {
    Bdd F2 = F;
    VALIDATE(F.numRefs() == 2);
  }
  VALIDATE(F.numRefs() == 1);

  Bdd one = mgr.getOne();
  Bdd zero = mgr.getZero();

  VALIDATE(zero <= zero);
  VALIDATE(zero <= one);
  VALIDATE(one <= one);
  VALIDATE(!(one <= zero));

  a = mgr.getLit(1);
  b = mgr.getLit(2);
  c = mgr.getLit(3);

  F = a*b;
  VALIDATE(!(F <= ~F));
  VALIDATE(F <= F);

  F1 = a*b* c + ~a;
  F2 = a*b*~c + ~a;
  VALIDATE(!(F1 <= ~F2));
  F = F1 * F2;
  VALIDATE(!(F1 <= ~F2));

  mgr.printStats();
} // testMisc

