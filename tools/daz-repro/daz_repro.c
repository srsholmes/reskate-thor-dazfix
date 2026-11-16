// DAZ/FTZ repro: does _controlfp_s(_DN_FLUSH) reach the x86 MXCSR, and do SSE ops honour FTZ/DAZ?
// Expected on native x86 Windows: MXCSR gains 0x8040 after _controlfp_s, denormals flush.
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <xmmintrin.h>

static unsigned bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }

static void test(const char* when) {
    volatile float tiny = 1e-39f;            // denormal input
    volatile float one = 1.0f, small = 1e-20f;
    __m128 a = _mm_set_ss(tiny), b = _mm_set_ss(one);
    float mul = _mm_cvtss_f32(_mm_mul_ss(a, b));                    // DAZ: input flushed -> 0
    float gt = _mm_cvtss_f32(_mm_cmpgt_ss(a, _mm_setzero_ps()));   // DAZ: 1e-39 > 0 is false
    float under = _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(small), _mm_set_ss(small)));  // FTZ: 1e-40 result -> 0
    __m128 x = _mm_set_ss(tiny);                                    // the game's guard: sqrt(1/x) & (0 < x)
    __m128 inv = _mm_sqrt_ss(_mm_div_ss(_mm_set_ss(1.0f), x));
    __m128 guard = _mm_and_ps(inv, _mm_cmplt_ss(_mm_setzero_ps(), x));
    printf("%-34s MXCSR=0x%04x  1e-39*1=0x%08x (%g)  (1e-39>0)=%s  1e-20*1e-20=0x%08x (%g)  guard=%g\n", when,
           _mm_getcsr(), bits(mul), mul, bits(gt) ? "true" : "false", bits(under), under, _mm_cvtss_f32(guard));
}

int main(void) {
    test("start");
    unsigned cw = 0;
    errno_t e = _controlfp_s(&cw, _DN_FLUSH, _MCW_DN);
    printf("_controlfp_s(_DN_FLUSH, _MCW_DN) -> err=%d control=0x%08x\n", (int)e, cw);
    test("after _controlfp_s(_DN_FLUSH)");
    e = _controlfp_s(&cw, 0x0108001f, 0x0308031f);     // the exact call skate. makes
    printf("_controlfp_s(0x0108001f, 0x0308031f) -> err=%d control=0x%08x\n", (int)e, cw);
    test("after skate.'s _controlfp_s");
    _mm_setcsr(0x1f80);
    test("after ldmxcsr 0x1f80");
    _mm_setcsr(0x9fc0);
    test("after ldmxcsr 0x9fc0 (FTZ+DAZ)");
    return 0;
}
