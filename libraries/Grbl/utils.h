#ifndef UTILS_H
#define UTILS_H


#define N_AXIS 3 // Number of axes
#define X_AXIS 0 // Axis indexing value. Must start with 0 and be continuous.
#define Y_AXIS 1
#define Z_AXIS 2
// #define A_AXIS 3

#define SERIAL_NO_DATA 0xff

#define MM_PER_INCH (25.40)
#define INCH_PER_MM (0.0393701)

// Useful macros
#define clear_vector(a) memset(a, 0, sizeof(a))
#define clear_vector_float(a) memset(a, 0, sizeof(float)*N_AXIS)

// Bit field and masking macros
#define bit(n) (1 << n)
// to do
//#define bit_true_atomic(x,mask) {uint8_t sreg = SREG; cli(); (x) |= (mask); SREG = sreg; }
//#define bit_false_atomic(x,mask) {uint8_t sreg = SREG; cli(); (x) &= ~(mask); SREG = sreg; }
#define bit_true_atomic(x,mask) (x) |= (mask)
#define bit_false_atomic(x,mask) (x) &= ~(mask)

#define bit_true(x,mask) (x) |= (mask)
#define bit_false(x,mask) (x) &= ~(mask)
#define bit_istrue(x,mask) ((x & mask) != 0)
#define bit_isfalse(x,mask) ((x & mask) == 0)

#ifdef __cplusplus
extern "C" {
#endif

// Read a floating point value from a string. Line points to the input buffer, char_counter 
// is the indexer pointing to the current character of the line, while float_ptr is 
// a pointer to the result variable. Returns true when it succeeds
uint8_t read_float(char *line, uint8_t *char_counter, float *float_ptr);

// Computes hypotenuse, avoiding avr-gcc's bloated version and the extra error checking.
float hypot_f(float x, float y);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H
