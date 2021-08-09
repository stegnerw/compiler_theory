#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdbool.h>

static char str_buff[256];

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
//float sqrt(int);

#endif  // RUNTIME_H
