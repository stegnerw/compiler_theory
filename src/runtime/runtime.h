#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdbool.h>

static char str_buff[256];

// get functions
bool getbool();
int getinteger();
float getfloat();
char* getstring();

// put functions
bool putbool(bool);
bool putinteger(int);
bool putfloat(float);
bool putstring(char*);

// misc functions
//float sqrt(int);

#endif  // RUNTIME_H
