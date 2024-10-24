//
//      File     : colors.h
//      Abstract : Color definitions.
//

#ifndef COLORS_H
#define COLORS_H

#define ENABLE_COLORS

#ifdef ENABLE_COLORS

#define BLUE	"\033[34m"
#define BOLD	"\033[1m"
#define GREEN	"\033[32m"
#define MAGENTA	"\033[35m"
#define NORMAL	"\033[0m"
#define RED	"\033[31m"
#define YELLOW	"\033[33m"

#else

#define BLUE ""
#define BOLD ""
#define GREEN ""
#define MAGENTA ""
#define NORMAL ""
#define RED ""
#define YELLOW ""

#endif

#endif // COLORS_H
