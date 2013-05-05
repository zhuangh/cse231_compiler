#ifndef ERROR_MESSAGE_H
#define ERROR_MESSAGE_H
#include <ostream>

#define PrintError(out) ( out << __FILE__ << " :" << __func__ << " :" << __LINE__ << '\n')

#endif
