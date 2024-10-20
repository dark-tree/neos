
// C0 codes
#define ANSI_BELL            0x07 /* Makes an audible noise. */
#define ANSI_BACKSPACE       0x08 /* Moves cursor one character back */
#define ANSI_TAB             0x09 /* Moves cursor forward to the next tab stop */
#define ANSI_LINE_FEED       0x0A /* Moves cursor to the next line */
#define ANSI_CARRIAGE_RETURN 0x0D /* Moves cursor to column on the current line */
#define ANSI_ESCAPE          0x1B /* Starts all the escape sequences (C1 codes) */
#define ANSI_ESC_DCS         ((char) 0x90) /* Equivalent to ANSI_ESCAPE + ANSI_DCS */
#define ANSI_ESC_CSI         ((char) 0x9B) /* Equivalent to ANSI_ESCAPE + ANSI_CSI */
#define ANSI_ESC_OCS         ((char) 0x9D) /* Equivalent to ANSI_ESCAPE + ANSI_OSC */

// Fe Escape Sequences
#define ANSI_ST  '\\' /* String Terminator */
#define ANSI_CSI '['  /* Control Sequence Introducer, terminated by a byte in range 0x40 through 0x7E */
#define ANSI_OSC ']'  /* Operating System Command, terminated by ANSI_ST */
#define ANSI_SOS 'X'  /* Start of String, terminated by ANSI_ST */
#define ANSI_PM  '^'  /* Privacy Message, terminated by ANSI_ST */
#define ANSI_APC '_'  /* Application Program Command, terminated by ANSI_ST */
#define ANSI_DCS 'P'  /* Device Control String, terminated by ANSI_ST */

// Fp Escape Sequences (Private Code)
#define ANSI_DECSC '7' /* Save Current Cursor Position and Formatting Attributes */
#define ANSI_DECRC '8' /* Restore Cursor Position and Formatting Attribute */

// Control Sequence Introducer
#define ANSI_CSI_CUU 'A' /* Cursor Up */
#define ANSI_CSI_CUD 'B' /* Cursor Down */
#define ANSI_CSI_CUF 'C' /* Cursor Forward */
#define ANSI_CSI_CUB 'D' /* Cursor Back */
#define ANSI_CSI_CNL 'E' /* Cursor Next Line */
#define ANSI_CSI_CPL 'F' /* Cursor Previous Line */
#define ANSI_CSI_CHA 'G' /* Cursor Horizontal Absolute */
#define ANSI_CSI_CUP 'H' /* Cursor Position */
#define ANSI_CSI_ED  'J' /* Erase in Display */
#define ANSI_CSI_EL  'K' /* Erase in Line  */
#define ANSI_CSI_SU  'S' /* Scroll Up */
#define ANSI_CSI_SD  'T' /* Scroll Down */
#define ANSI_CSI_HVP 'f' /* Horizontal Vertical Position */
#define ANSI_CSI_SGR 'm' /* Select Graphic Rendition */
#define ANSI_CSI_SCP 's' /* Save Current Cursor Position (Private Code) */
#define ANSI_CSI_RCP 'u' /* Restore Saved Cursor Position (Private Code) */

// Select Graphic Rendition
#define ANSI_SGR_RESET             0   /* Turns off all attributes */
#define ANSI_SGR_BOLD              1   /* Intensifies the color */
#define ANSI_SGR_BOLD_RESET        22  /* De-intensifies the color */
#define ANSI_SGR_INVERT            7   /* Swaps background and foreground colors */
#define ANSI_SGR_INVERT_RESET      27  /* Un-swaps background and foreground colors */
#define ANSI_SGR_FG_COLOR_BEGIN    30  /* First foreground color code */
#define ANSI_SGR_FG_COLOR_END      37  /* Last foreground color code */
//#define ANSI_SGR_FG_COLOR_EXTENDED 38  /* Marks the start of extended foreground color code */
#define ANSI_SGR_FG_COLOR_RESET    39  /* Reset foreground color to the default value */
#define ANSI_SGR_FG_BOLD_BEGIN     90  /* Marks the start of intensified foreground color code */
#define ANSI_SGR_FG_BOLD_END       97  /* Last intensified foreground color code */
#define ANSI_SGR_BG_COLOR_BEGIN    40  /* First background color code */
#define ANSI_SGR_BG_COLOR_END      47  /* Last background color code */
//#define ANSI_SGR_BG_COLOR_EXTENDED 48  /* Marks the start of extended background color code */
#define ANSI_SGR_BG_COLOR_RESET    49  /* Reset background color to the default value */
#define ANSI_SGR_BG_BOLD_BEGIN     100 /* Marks the start of intensified background color code */
#define ANSI_SGR_BG_BOLD_END       107 /* Last intensified background color code */
//#define ANSI_SGR_COLOR_PALLET      2   /* follows the *_EXTENDED colors, marks the start of pallet color */
//#define ANSI_SGR_COLOR_RGB         5   /* follows the *_EXTENDED colors, marks the start of RGB color */

// Erase modes (ANSI_CSI_ED, ANSI_CSI_EL)
#define ANSI_ERASE_AFTER  0 /* Clear from cursor to the end of object */
#define ANSI_ERASE_BEFORE 1 /* Clear from start of object to the cursor */
#define ANSI_ERASE_WHOLE  2 /* Clear whole object */
#define ANSI_ERASE_RESET  3 /* Clear whole object, and clear buffers (if applicable) */

// Helpers
#define ANSI_CSI_FINAL(byte) (((byte) >= 0x40) && ((byte) <= 0x7E))
