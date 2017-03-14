#include "muladd.h"

int64_t LONG_MASK = 0xffffffffL;
int32_t mulAdd(int32_t *out, int32_t *in, int32_t offset, int len, int32_t k, size_t out_len) {
  int64_t carry = 0;

#ifdef _ASM


  offset = out_len - offset;
  int a=0,b=0;
  size_t in_aux, out_aux;
 asm volatile (
     "cmpdi  %[len], 0\n\t"
     "ble    SKIP\n\t"
     "clrldi %[k],%[k],32\n\t"
     "mtctr  %[len]\n\t"
     "mr     %[in_aux],%[in]\n\t"
     "mr     %[out_aux],%[out]\n\t"

     // offset and len will be adjusted to 4 bytes
     "sldi   %[offset],%[offset],2\n\t"
     "sldi   %[len],%[len],2\n\t"
     "add    %[in_aux],%[len],%[in_aux]\n\t"
     "add    %[out_aux],%[offset],%[out_aux]\n\t"
     "LOOP:\n\t"
     "lwzu   %[a],-4(%[in_aux])\n\t"
     "lwzu   %[b],-4(%[out_aux])\n\t"
     "mulld  %[a],%[a],%[k]\n\t"
     "add    %[b],%[carry],%[b]\n\t"
     "add    %[b],%[a],%[b]\n\t"
     "stwx   %[b],0,%[out_aux]\n\t\n\t"
     "srdi   %[carry],%[b],32\n\t"
     "bdnz   LOOP\n\t"
     "SKIP:\n\t"
     :
     [carry] "+b" (carry),
     [a] "=&b" (a),
     [b] "=&b" (b),
     [k] "+b" (k),
     [offset] "+b" (offset),
     [len] "+b" (len),
     [in_aux] "=&b" (in_aux),
     [out_aux] "=&b" (out_aux)
     :
     [in] "r" (in),
     [out] "r" (out)
     :
     "memory", "ctr"
   );
#else

  offset = out_len - offset - 1;
  int64_t kLong = k & LONG_MASK;
  for (int j=len-1; j >= 0; j--) {
    int64_t product = (in[j] & LONG_MASK) * kLong + (out[offset] & LONG_MASK) + carry;
    out[offset--] = (int32_t)(product);
    carry = (uint64_t)product >> 32;
  }

#endif

  return (int32_t)carry;
}
