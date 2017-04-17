#include "muladd.h"
#include <altivec.h>
#include <stdint.h>

int64_t LONG_MASK = 0xffffffffL;
int32_t mulAdd(int32_t *out, int32_t *in, int32_t offset, int len, int32_t k, size_t out_len) {
  int64_t carry = 0;
  offset = out_len - offset;

#ifdef _ASM
  int64_t in_scratch, out_scratch;
  int64_t tmp;
  int64_t tmp2;
  vector int64_t mask;
  vector int64_t zero_vector;
  vector int64_t magic_param_vector;
  vector int64_t k_vector;
  vector int64_t carry_vector;
  vector int64_t last_in_vector;
  vector int64_t last_out_vector;
  vector int64_t in_vector;
  vector int64_t out_vector;
  vector int64_t tmp_vector;
  vector int64_t mul_res_odd_vector;
  vector int64_t mul_res_eve_vector;
  vector int64_t add_vector;
  vector int64_t add_carry_vector;
  vector int64_t in_param_vector;
  vector int64_t out_param_vector;

  __asm__ volatile (
     // Move in and out to sratch regs
     "mr       %[in_scratch], %[in]\t\n"
     "mr       %[out_scratch], %[out]\t\n"

     // Calculate number of vectorized iterations
     "srdi.    %[tmp], %[len], 2\t\n"

     // Calculate offsets
     "sldi     %[len], %[len], 2\t\n"
     "add      %[in_scratch], %[in_scratch], %[len]\t\n"

     "sldi     %[offset], %[offset], 2\t\n"
     "add      %[out_scratch], %[out_scratch], %[offset]\t\n"

     "ble      SLOOP_INIT\t\n"
     "mtctr    %[tmp]\t\n"

     // set mask
     "vxor     %[zero_vector], %[zero_vector], %[zero_vector]\t\n"
     "vspltisw %[tmp_vector], -1\t\n"
     "vsldoi   %[mask], %[tmp_vector], %[zero_vector], 4\t\n"

     // set perm control reg to reverse words in a 4-word vector
     "li       %[tmp], 0\n\t"
     "lvsl     %[magic_param_vector], 0, %[tmp]\t\n"

     // Bit magic: by xor'ing every byte of the perm control reg with 0xC
     // we get a quick vector word reverse
     "vspltisb %[tmp_vector], 0xC\t\n"
     "vxor     %[magic_param_vector], %[magic_param_vector], %[tmp_vector]\t\n"

     // Copy k to a vector
     "mtvrwz   %[k_vector], %[k]\t\n"
     "vspltw   %[k_vector], %[k_vector], 1\t\n"

     // Copy carry to a vector
     "mtvrwz   %[carry_vector], %[carry]\t\n"
     "vspltw   %[carry_vector], %[carry_vector], 1\t\n"

     "lvx      %[last_in_vector], 0, %[in_scratch]\t\n"
     "lvsr     %[in_param_vector], 0, %[in_scratch]\t\n"
     "lvx      %[last_out_vector], 0, %[out_scratch]\t\n"
     "lvsr     %[out_param_vector], 0, %[out_scratch]\t\n"
    "VLOOP:\t\n"
     "addi     %[in_scratch], %[in_scratch], -16\t\n"
     "addi     %[out_scratch], %[out_scratch], -16\t\n"

     // Load in
     "lvx      %[tmp_vector], 0, %[in_scratch]\t\n"
     "vperm    %[in_vector], %[last_in_vector], %[tmp_vector], %[in_param_vector]\t\n"
     "vmr      %[last_in_vector], %[tmp_vector]\t\n"

     // Load out
     "lvx      %[tmp_vector], 0, %[out_scratch]\t\n"
     "vperm    %[out_vector], %[last_out_vector], %[tmp_vector], %[out_param_vector]\t\n"
     "vmr      %[last_out_vector], %[tmp_vector]\t\n"

     // multiplying 32x32 -> 64
     "vmuleuw  %[mul_res_eve_vector], %[in_vector], %[k_vector]\t\n"
     "vmulouw  %[mul_res_odd_vector], %[in_vector], %[k_vector]\t\n"

     // Aligning digits with previous carries
     "vsldoi   %[mul_res_eve_vector], %[mul_res_eve_vector], %[mul_res_eve_vector], 8\t\n"
     "vsldoi   %[mul_res_odd_vector], %[mul_res_odd_vector], %[mul_res_odd_vector], 12\t\n"

     // Update carry from previous iteration
     "vsel     %[tmp_vector], %[carry_vector], %[mul_res_odd_vector], %[mask]\t\n"
     // Carry from the multiplication
     "vandc    %[carry_vector], %[mul_res_odd_vector], %[mask]\t\n"

     // Add carries from multiply
     "vadduqm  %[add_vector], %[mul_res_eve_vector], %[tmp_vector]\t\n"
     "vaddcuq  %[add_carry_vector], %[tmp_vector], %[mul_res_eve_vector]\t\n"

     // TODO: draw an ASCII table to explain what's going on here

     // By now, we have a little-endian 128bit int on vr5
     // We should reverse out to match this
     "vperm    %[tmp_vector], %[out_vector], %[out_vector], %[magic_param_vector]\t\n"

     // Add to output and get carry
     "vadduqm  %[out_vector],  %[tmp_vector], %[add_vector]\t\n"
     "vaddcuq  %[tmp_vector], %[tmp_vector], %[add_vector]\t\n"

     // Add carries from both multiplication and sum
     "vadduwm  %[carry_vector], %[carry_vector], %[add_carry_vector]\t\n"
     "vadduwm  %[carry_vector], %[carry_vector], %[tmp_vector]\t\n"

     // Reverse out back to big-endian
     "vperm    %[out_vector], %[out_vector], %[out_vector], %[magic_param_vector]\t\n"

     // Update out in memory
     "xxswapd    %x[out_vector], %x[out_vector]\t\n"
     "stxvd2x    %x[out_vector], 0, %[out_scratch]\t\n"

     "bdnz     VLOOP\t\n"

     // Move carry back to a GPR
     "vsldoi   %[carry_vector], %[carry_vector], %[carry_vector], 8\t\n"
     "mfvrwz   %[carry], %[carry_vector]\t\n"

     // Deal serially with leftovers
     "SLOOP_INIT:\t\n"
     // Calculate number of sequential iterations
     "srdi     %[len], %[len], 2\n\t"
     "andi.    %[tmp], %[len], 3\t\n"
     "ble      SKIP\t\n"

     "clrldi   %[k], %[k], 32\t\n"
     "mtctr    %[tmp]\t\n"
    "SLOOP:\t\n"
     "lwzu     %[tmp], -4(%[in_scratch])\t\n"
     "lwzu     %[tmp2], -4(%[out_scratch])\t\n"
     "mulld    %[tmp], %[tmp], %[k]\t\n"
     "add      %[tmp], %[tmp2], %[tmp]\t\n"
     "add      %[tmp2], %[tmp], %[carry]\t\n"
     "stwx     %[tmp2], 0, %[out_scratch]\t\n"
     "srdi     %[carry], %[tmp2], 32\t\n"
     "bdnz     SLOOP\t\n"

    "SKIP:\t\n"
   : /* output  */
     [zero_vector] "=&v" (zero_vector),
     [mask] "=&v" (mask),
     [magic_param_vector] "=&v" (magic_param_vector),
     [in_param_vector] "=&v" (in_param_vector),
     [out_param_vector] "=&v" (out_param_vector),
     [k_vector] "=&v" (k_vector),
     [carry_vector] "=&v" (carry_vector),
     [in_vector] "=&v" (in_vector),
     [out_vector] "=&v" (out_vector),
     [last_in_vector] "=&v" (last_in_vector),
     [last_out_vector] "=&v" (last_out_vector),
     [tmp_vector] "=&v" (tmp_vector),
     [mul_res_odd_vector] "=&v" (mul_res_odd_vector),
     [mul_res_eve_vector] "=&v" (mul_res_eve_vector),
     [add_vector] "=&v" (add_vector),
     [add_carry_vector] "=&v" (add_carry_vector),
     [carry] "+r" (carry),
     [len] "+b" (len),
     [offset] "+b" (offset),
     [tmp] "=&r" (tmp),
     [tmp2] "=&r" (tmp2),
     [in_scratch] "=&b" (in_scratch),
     [out_scratch] "=&b" (out_scratch)
   : /* input   */
     [in] "r" (in),
     [out] "r" (out),
     [k] "r" (k)
   : /* clobber */
     "memory"
  );
#else
  offset--;
  int64_t kLong = k & LONG_MASK;
  for (int j=len-1; j >= 0; j--) {
    int64_t product = (in[j] & LONG_MASK) * kLong + (out[offset] & LONG_MASK) + carry;
    out[offset--] = (int32_t)(product);
    carry = (uint64_t)product >> 32;
  }

#endif

  return (int32_t)carry;
}
