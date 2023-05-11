#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <iostream>         // cout

#define __MSG__(msg_type, msg)                                  \
do{                                                             \
    std::cout << (*msg_type == '\0' ? "" : "[" msg_type "] ")   \
              << __func__ << ": " << msg << std::endl;          \
}while(0)

#define OUTMSG(msg)    __MSG__("", msg)
//#define DEBUGMSG(msg)   __MSG__("DEBUG", msg)
#define DEBUGMSG(msg)
#define ERRORMSG(msg)  __MSG__("ERROR", msg)

#endif // __LOGGER_H__

