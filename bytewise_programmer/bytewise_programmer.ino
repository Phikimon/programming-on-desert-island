#include <SPI.h>
#include <SoftSPI.h>
#include <stdint.h>

// This is parallel programmer for ATtiny13 that first flashes
// blink program and then allows to rewrite one page of it
// byte-by-byte conveniently.

// Program that changes pin state approximately each 74K cycles, which
// at 128 KHz clock frequency produces 577 ms delay
uint8_t blink_program[] = {
    // === Page 0 ===

    //     ldi r24, 0x02
    //     ldi r25, 0x00
    //     out 0x17(DDRB), r24
    // L1:
    //     out 0x18(PORTB), r25
    //     rcall wait
    //     out 0x18(PORTB), r24
    //     rcall wait
    //     rjmp L1

    // === Page 1 ===

    //  wait:
    //      ldi r19, 0x53(83d) ; This value defines how long will the delay be.
    //                         ; It is put in separate page so that it can be
    //                         ; easily modified.
    //      rjmp L2            ; Just jumps over the trash in the end of the page.

    // === Page 2 ===

    //  L2:
    //      ldi r20, 0
    //      loop: dec r20
    //      brne loop
    //      dec r19
    //      brne loop
    //      ret
    0b10101100,  0b01010011,  0b00000000,  0b00000000, // program enable
    0b10101100,  0b10000000,  0b00000000,  0b00000000, // chip erase

    0b01000000,  0b00000000,  0b00000000,  0b10000010, // load addr.0000  low byte 82
    0b01001000,  0b00000000,  0b00000000,  0b11100000, // load addr.0000 high byte E0 ; ldi r24, 0x02

    0b01000000,  0b00000000,  0b00000001,  0b10010000, // load addr.0001  low byte 90
    0b01001000,  0b00000000,  0b00000001,  0b11100000, // load addr.0001 high byte E0 ; ldi r25, 0x00

    0b01000000,  0b00000000,  0b00000010,  0b10000111, // load addr.0010  low byte 87
    0b01001000,  0b00000000,  0b00000010,  0b10111011, // load addr.0010 high byte BB ; out 0x17(DDRB), r24

    0b01000000,  0b00000000,  0b00000011,  0b10011000, // load addr.0011  low byte 98
    0b01001000,  0b00000000,  0b00000011,  0b10111011, // load addr.0011 high byte BB ; L1: out 0x18(PORTB), r25

    0b01000000,  0b00000000,  0b00000100,  0b00001011, // load addr.0100  low byte 0B
    0b01001000,  0b00000000,  0b00000100,  0b11010000, // load addr.0100 high byte D0 ; rcall wait

    0b01000000,  0b00000000,  0b00000101,  0b10001000, // load addr.0101  low byte 88
    0b01001000,  0b00000000,  0b00000101,  0b10111011, // load addr.0101 high byte BB ; out 0x18(PORTB), r24

    0b01000000,  0b00000000,  0b00000110,  0b00001001, // load addr.0110  low byte 09
    0b01001000,  0b00000000,  0b00000110,  0b11010000, // load addr.0110 high byte D0 ; rcall wait

    0b01000000,  0b00000000,  0b00000111,  0b11111011, // load addr.0111  low byte FB ; rjmp L1
    0b01001000,  0b00000000,  0b00000111,  0b11001111, // load addr.0111 high byte CF

                               //  V
    0b01001100,  0b00000000,  0b00000000,  0b00000000, // write page 0

// If you read presentation carefully, then you will notice that we rewrite instruction
//   ldi r19, 0x54(83d)
// to
//   ldi r19, 0x29(41d)
// Rewriting memory in AVR (due to its NOR nature) in fact works this way:
// bytes being written to memory are bitwisely AND-ed with those that are stored already,
// so I had to come up with a hack -- I put the following sequence of instructions in
// the page #1:
//   ldi r19, 0xFF ; (so that when it is rewritten new value
//                 ;  ends up actually stored in the memory)
//   ldi r19, 0x53 ; (assigns value that we initially wanted
//                 ;  to store in the memory)
//   rjmp TEMP_LABEL  ; The point of introducing temporary label is that when (ldi r19, 0x53) is
//   ...              ; AND-ed in memory with (rjmp L2; 0xE) the resulting instruction
//                    ; (rjmp (0x00E & 0x533)) = (rjmp 0x2) will actually jump on TEMP_LABEL, that's
// TEMP_LABEL:        ; why I put fixing correct rjmp there
//   rjmp L2 ; Jumps to leftovers of the wait function
#ifdef FAIR
    0b01000000,  0b00000000,  0b00000000,  0b00110011, // load addr.0000  low byte 33 ; wait: ldi r19, 0x53(83d)
    0b01001000,  0b00000000,  0b00000000,  0b11100101, // load addr.0000 high byte E5

    0b01000000,  0b00000000,  0b00000001,  0b00001110, // load addr.0001  low byte 0E ; rjmp L2
    0b01001000,  0b00000000,  0b00000001,  0b11000000, // load addr.0001 high byte C0
#else //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    0b01000000,  0b00000000,  0b00000000,  0b00111111, // load addr.0000  low byte 3F ; wait: ldi r19, 0xFF
    0b01001000,  0b00000000,  0b00000000,  0b11101111, // load addr.0000 high byte EF

    0b01000000,  0b00000000,  0b00000001,  0b00110011, // load addr.0001  low byte 33 ; wait: ldi r19, 0x53(83d) => 02
    0b01001000,  0b00000000,  0b00000001,  0b11100101, // load addr.0001 high byte E5                            => C0

    0b01000000,  0b00000000,  0b00000010,  0b00001110, // load addr.0010  low byte 0D ; rjmp L2
    0b01001000,  0b00000000,  0b00000010,  0b11000000, // load addr.0010 high byte C0
    // here is nop(note the addresses)
    0b01000000,  0b00000000,  0b00000100,  0b00001110, // load addr.0100  low byte 0B ; rjmp L2
    0b01001000,  0b00000000,  0b00000100,  0b11000000, // load addr.0100 high byte C0
#endif
                               //  V
    0b01001100,  0b00000000,  0b00010000,  0b00000000, // write page 1

    0b01000000,  0b00000000,  0b00000000,  0b01001111, // load addr.0000  low byte 4F ; L2: ldi r20, FF
    0b01001000,  0b00000000,  0b00000000,  0b11101111, // load addr.0000 high byte EF

    0b01000000,  0b00000000,  0b00000001,  0b01001010, // load addr.0001  low byte 4A ; loop: dec r20
    0b01001000,  0b00000000,  0b00000001,  0b10010101, // load addr.0001 high byte 95

    0b01000000,  0b00000000,  0b00000010,  0b11110001, // load addr.0010  low byte F1 ; brne loop
    0b01001000,  0b00000000,  0b00000010,  0b11110111, // load addr.0010 high byte F7

    0b01000000,  0b00000000,  0b00000011,  0b00111010, // load addr.0011  low byte 3A ; dec r19
    0b01001000,  0b00000000,  0b00000011,  0b10010101, // load addr.0011 high byte 95

    0b01000000,  0b00000000,  0b00000100,  0b11100001, // load addr.0100  low byte E1 ; brne loop
    0b01001000,  0b00000000,  0b00000100,  0b11110111, // load addr.0100 high byte F7

    0b01000000,  0b00000000,  0b00000101,  0b00001000, // load addr.0101  low byte 08 ; ret
    0b01001000,  0b00000000,  0b00000101,  0b10010101, // load addr.0101 high byte 95

                               // VV
    0b01001100,  0b00000000,  0b00100000,  0b00000000, // write page 2
};

//   This sequence of bytes only rewrites page 1 of program memory
// so that delay counter value is changed from 0x53 to 0x29, thus
// increasing blinking frequency twice.
//   This sequence is intended to be written manually byte-by-byte,
// though for debug reasons it is written when 0b01010101 pattern
// is set on PC0:PC5, PD0:PD1 and both 'write' and 'reset' buttons
// are pressed at once.
uint8_t tweak_blink_program[] = {
    0b10101100,  0b01010011,  0b00000000,  0b00000000, // program enable

    0b01000000,  0b00000000,  0b00000000,  0b00111001, // load addr.1001  low byte 39 ; wait: ldi r19, 0x29(41)
    0b01001000,  0b00000000,  0b00000000,  0b11100010, // load addr.1001 high byte E2

    0b01000000,  0b00000000,  0b00000001,  0b00001110, // load addr.0111  low byte 0E ; rjmp L2
    0b01001000,  0b00000000,  0b00000001,  0b11000000, // load addr.0111 high byte C0

                              //   V
    0b01001100,  0b00000000,  0b00010000,  0b00000000, // write page 1
};

//   'Reset' and 'write' buttons configuration.
// If they are pressed simultaneously, the blink program
// is flashed to the target.
//   Pressing only reset button causes change of state of
// pin connected to target's reset pin.
//   Pressing only write button writes one byte set by
// states of pins PC0:PC5; PD0:PD1 to SPI pins configured below.
//   Note: LOW state for buttons is 'released', HIGH is 'pressed".
int reset_button_state = LOW;
int last_reset_button_state = LOW;
unsigned long last_time_reset_button = 0;  // the last time the pin was toggled

int write_button_state = LOW;
int last_write_button_state = LOW;
unsigned long last_time_write_button = 0;

#define DEBOUNCE_DELAY 50

// State of pin connected to target's reset pin.
int reset_pin_state = HIGH;

// SPI configuration
#define MISO  7
#define MOSI  5
#define SCK   6
#define RESET 10

#define WRITE_BUTTON_PIN A0
#define RESET_BUTTON_PIN 11

SoftSPI mySPI(MOSI, MISO, SCK);

inline int get_switch_value(void) {
      // PC2:PC5 => 0-3 bits
      // PD4:PD1 => 4-7 bits
      // PD4 => 4 bit
      // PD3 => 5 bit
      // PD2 => 6 bit
      // PD1 => 7 bit
   return ((PINC & 0b00111100) >> 2) |
          ((PIND & 0b00010000)     ) | // PD4
          ((PIND & 0b00001000) << 2) | // PD3
          ((PIND & 0b00000100) << 4) | // PD2
          ((PIND & 0b00000010) << 6);  // PD1

}

void setup() {
    // Set up SPI
    mySPI.begin();
    // Boot target properly
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, HIGH);
    delay(20);
    // Set up pins PC2:PC5, PD1:PD4
    DDRC &= ~(0x3C);
    DDRD &= ~(0x1E);
}

void spi_write()
{
    uint8_t v = get_switch_value();
    mySPI.transfer(v);
    digitalWrite(MOSI, LOW); // Just for convenience
}

void flash_program(void)
{
    uint8_t* program = blink_program;
    int program_size = sizeof(blink_program)/sizeof(blink_program[0]);

    // Debugging hack: if magical number 0b01010101 is set
    // on switch, tweak_blink_program sequence
    // is sent to target.
    uint8_t v = get_switch_value();
    if (v == 0b01010101) {
        program = tweak_blink_program;
        program_size = sizeof(tweak_blink_program)/sizeof(tweak_blink_program[0]);
    }

    // Don't depend on reset pin previous state
    digitalWrite(RESET, HIGH);
    delay(10);
    digitalWrite(RESET, LOW);
    delay(20);
    for (int i = 0; i < program_size; i++) {
        mySPI.transfer(program[i]);
        delay(10);
    }
    digitalWrite(RESET, HIGH);
    reset_pin_state = HIGH;
}

void reset_button_action(void)
{
    if (reset_button_state == HIGH) { // Change reset pin state on button press
        reset_pin_state ^= 1;
        digitalWrite(RESET, reset_pin_state);
    }

    if ((reset_button_state == HIGH) && // If both buttons are pressed
        (write_button_state == HIGH))   // load initial program
        flash_program();
}

void write_button_action(void)
{
    if ((reset_pin_state == LOW) &&   // It is only sensible to write when target's reset pin is LOW
        (write_button_state == LOW)) { // Write on button release. If program was flashed on button
                                        // press then reset_pin_state can only be high.
        spi_write();
    }

    if ((reset_button_state == HIGH) && // If both buttons are pressed
        (write_button_state == HIGH))   // load initial program
        flash_program();
}

// Logic to read buttons' states with debouncing
void loop() {
    int read_reset = digitalRead(RESET_BUTTON_PIN);
    int read_write = digitalRead(WRITE_BUTTON_PIN);

    if (read_reset != last_reset_button_state)
        last_time_reset_button = millis();

    if (read_write != last_write_button_state)
        last_time_write_button = millis();

    if ((millis() - last_time_reset_button) > DEBOUNCE_DELAY) {
        if (read_reset != reset_button_state) {
            reset_button_state = read_reset;

            reset_button_action();
        }
    }

    if ((millis() - last_time_write_button) > DEBOUNCE_DELAY) {
        if (read_write != write_button_state) {
            write_button_state = read_write;

            write_button_action();
        }
    }

    last_reset_button_state = read_reset;
    last_write_button_state = read_write;
}
