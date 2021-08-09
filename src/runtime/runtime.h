#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdbool.h>

// get functions
extern bool getbool();
extern int getinteger();
extern float getfloat();
extern char* getstring();

// put functions
extern bool putbool(bool);
extern bool putinteger(int);
extern bool putfloat(float);
extern bool putstring(char*);

// misc functions
extern float altsqrt(int);

#endif  // RUNTIME_H
