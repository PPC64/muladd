#include "muladd.h"

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
  int32_t out1[] = {17,16,15,14,13,12,11,10,0x9,0x8,0x7,0x6,0x5};
  int32_t in1[] = {13,12,11,10,9,8,7,6,5,4,3,2,1};
  int32_t offset = 0;
  int32_t len = 13;
  int32_t k = 0x7fffffff;
  int32_t carry;
  carry = mulAdd(out1, in1, offset, len, k, sizeof(out1)/sizeof(int32_t));
  print_mul_add_result(out1, sizeof(out1)/sizeof(int32_t), carry);
  return 0;
}
