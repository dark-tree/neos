
/**
 * @brief The maximum length of any ANSI control sequence,
 *        if a sequence that is longer is send to the output it will be ignored.
 */
#define CONSOLE_MAX_ANSI_SEQUENCE 64

/**
 * @brief The default attribute for screen buffer
 *        that is used when ANSI attributes are reset.
 */
#define CONSOLE_DEFAULT_ATTRIBUTE 0b00000111

/**
 * @brief The spacing between tab-stops, that is,
 *        the equivalent in spaces of the tab character.
 */
#define CONSOLE_TAB_DISTANCE 4
