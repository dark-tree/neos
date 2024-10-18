
// C0 codes
#define ANSI_BELL            0x07 /* Makes an audible noise. */
#define ANSI_BACKSPACE       0x08 /* Moves cursor one character back */
#define ANSI_TAB             0x09 /* Moves cursor forward to the next tab stop */
#define ANSI_LINE_FEED       0x0A /* Moves cursor to the next line */
#define ANSI_CARRIAGE_RETURN 0x0D /* Moves cursor to column on the current line */
#define ANSI_ESCAPE          0x1B /* Starts all the escape sequences */

// Check if the byte after a ANSI_ESCAPE is a valid C1 code
#define ANSI_IS_VALID_CODE(x) (((x) >= 0x40) && ((x) <= 0x5F))

// C1 codes
