//
//      File     : Defines.h
//      Abstract : Compile-time flags
//

#ifndef DEFINES_H
#define DEFINES_H

// Enables gathering of statistics about cache performance.
// #define CACHESTATS

// Enables use of banked memory for BddNode objects. This is similar
// to a deque. When enabled there is a double dereference to get to
// the BddNode (id->bank->node). The trade-off is that less memory is
// needed in general. Non-banked mode is not well tested yet. For the
// n-queens example there is a modest run time improvement for a large
// memory penalty. For ISCAS-85 c7225 with reordering there is a
// significant run time penalty and a modest memory penalty.
#define BANKEDMEM

#endif // DEFINES_H
