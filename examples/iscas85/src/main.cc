//
//      File     : main.cc
//	Abstract :Driver
//
#include "Ckt.h"

#include <iostream>
#include <string>
#include <unistd.h>

using std::cin;
using std::cout;
using std::endl;


//      Function : usage
//      Abstract : Print help message.
void
usage()
{
  cout << R"(Usage: iscas [<option>*] <file>

where <option> is one of the following:

-h		Print this help message and exit.

-r		Enable variable reordering.

-R <file>	Use <file> to generate an initial variable ordering.

-W <file>	Write the final variable ordering to <file>.
)"

       << endl;

} // usage



//      Function : main
//      Abstract : Driver
int
main(int argc, char *argv[])
{
  bool reorder = false;
  std::string readVarFn;
  std::string writeVarFn;

  int c;
  while((c = getopt(argc, argv, "hrR:W:")) != -1) {
    switch (c) {
    case 'h':
      usage();
      return 0;
      break;
    case 'r':
      reorder = true;
      break;
    case 'R':
      readVarFn = optarg;
      break;
    case 'W':
      writeVarFn = optarg;
      break;
    default:
      usage();
      return 1;
    } // switch
  } // while

  if (optind != argc-1) {
    cout << "Error: Exactly one filename must be specified." << endl;
    return 1;
  } // if

  std::string filename(argv[optind]);
  Ckt ckt(reorder);
  if (! ckt.parse(filename)) {
    cout << "Error: could not parse file \""
	 << filename << "\"." << endl;
    return 1;
  } // if

  cout << "Processing ..." << endl;
  ckt.readOrder(readVarFn);
  ckt.buildBdds();
  ckt.printSizes();
  ckt.writeOrder(writeVarFn);
  ckt.printStats();

  return 0;
} // main

