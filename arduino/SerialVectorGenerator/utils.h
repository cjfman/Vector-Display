#ifndef VECTOR_UTILS_H
#define VECTOR_UTILS_H

#define DAC_BIT_WIDTH 16

#ifndef max
#define max(a,b)                \
   ({ __typeof__ (a) _a = (a);  \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })      \

#endif // max

#ifndef min
#define min(a,b)                \
   ({ __typeof__ (a) _a = (a);  \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })      \

#endif // mix

// The ceiling of log2
// Number of bits required to hold a number
static inline uint8_t log2ceil(uint16_t num) {
    for (short i=15; i >= 0; i--) {
        if ((num >> i) & 0x01) return i + 1;
    }
    return 0;
}

// Scale position to a bit width
static inline uint16_t position_to_binary(int16_t pos, uint8_t scale_power, uint8_t bits, bool dipole) {
    bits = min(16, bits);
    if (dipole) {
        bits--;
    }
    else {
        pos = max(0, pos);
    }
    uint32_t max_value = (1l << bits) - 1;
    return (uint16_t)(((max_value * pos) >> scale_power) & 0xFFFF);
}

#endif // VECTOR_UTILS_H
