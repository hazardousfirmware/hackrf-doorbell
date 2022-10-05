#ifndef RF_MODULATION_H_INCLUDED
#define RF_MODULATION_H_INCLUDED

/* Signal frame format:
CODE BREAK CODE BREAK ... for about ~2 seconds
the code + break happens about 100 times
*/

#define REPEATS 100


// Line code values (microseconds)
#define SYMBOL_PERIOD 610
#define BIG_PULSE 450
#define SMALL_PULSE 160

// the gap between consecutive codes (~4200 us)
#define BREAK_BETWEEN 4190


/* The Symbols
LOW = 160us PULSE, 450 us SPACE
HIGH = 450us PULSE, 160 us SPACE

length of each code + break = 19.8ms

signals weakens after 1.79sec

The code:

LOW,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,LOW,LOW,LOW,HIGH,LOW,HIGH,LOW,LOW,LOW,LOW,LOW,LOW,LOW,HIGH,LOW,LOW,LOW

25 Bits
*/

// the code sent by the doorbell button
const uint32_t frame = 0x002028FE; // Depends on individual unit (see DIP switches)
#define SYMBOLS_PER_FRAME 25 // Number of bits per frame

#endif // RF_MODULATION_H_INCLUDED
