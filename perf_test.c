#include "muladd.h"
#include <time.h>
#include <stdlib.h>

void print_mul_add_result(int *out, int len, int carry)
{
  printf("\nout:");
  for(size_t i=0; i < len; i++){
    printf("%d\n",out[i]);
  }
  printf("\ncarry: %d\n",carry);
  return;
}

int main () {
  int32_t *out1;
  int32_t *in1;
  int32_t offset = 1;
  int32_t k = 945325299;
  int64_t SIZE = 536936447; //536875007;


  in1 =  (int32_t*) malloc (sizeof(int32_t)*SIZE);
  out1 = (int32_t*) malloc (sizeof(int32_t)*SIZE);

  for (int i = 0; i < SIZE ; i++) {
    in1[i] = i;
    out1[i] = i;
  }

  for(int len = 0; len < SIZE; len += (SIZE/100)) {
    clock_t t;
    t = clock();
    mulAdd(out1, in1, offset, len, k, SIZE);
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("%d %f\n", len, time_taken);
  }
  return 0;
}
