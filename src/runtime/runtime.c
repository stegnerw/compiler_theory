#include "runtime.h"

#include <stdio.h>
#include <math.h>

// get functions
bool getbool() {
  int input = 0;
  scanf("%d", &input);
  return (bool)input;
}
int getinteger() {
  int input = 0;
  scanf("%d", &input);
  return input;
}
float getfloat() {
  float input = 0;
  scanf("%f", &input);
  return input;
}
char* getstring() {
  scanf("%s", str_buff);
  return str_buff;
}

// put functions
bool putbool(bool b) {
  if (b) printf("true\n");
  else printf("false\n");
  return 0;
}
bool putinteger(int i) {
  printf("%d\n", i);
  return 0;
}
bool putfloat(float f) {
  printf("%f\n", f);
  return 0;
}
bool putstring(char* c) {
  printf("%s\n", c);
  return 0;
}

// misc functions
float altsqrt(int i) {
  return (float) sqrt(i);
}
