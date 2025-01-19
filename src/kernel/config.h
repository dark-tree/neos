
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

/**
 * @brief The maximum length of a number, in digits, as
 *        as printed by kprintf() (for any base)
 */
#define KPRINTF_MAX_DIGITS 64

/**
 * @brief The physical address of the Interrupt Descriptor Table,
 *        Refer to the x86 memory map when modifying this value.
 */
#define MEMORY_MAP_IDT 0x0

/**
 * @brief The physical address of the Memory Map, as constructed by the
 *        bootloader. Refere to the BIOS Function INT 0x15, EAX = 0xE820
 */
#define MEMORY_MAP_RAM 0x500

/**
 * @brief Initial size of the process vector
 *        inside the sheduler, that holds process descriptors.
 */
#define INITIAL_PROCESS_TABLE_SIZE 2

/**
 * @brief The maximal number of file
 *        descriptors per process.
 */
#define MAX_FILES_PER_PROCESS 1024

/**
 * @brief The maximum length (in characters) of a single path element (file or
 *        directory name) in a path string, including the null-byte
 */
#define FILE_MAX_NAME 256

/**
 * @brief The maximum number of segments (slash separated sections) in a path,
 *        including the resolves used during link traversal.
 */
#define PATH_MAX_RESOLVES 64

/**
 * @brief The size of stack allocated by default to a new process,
 *        this does not concern processes created using fork.
 */
#define INITIAL_STACK_SIZE 1024*32

/**
 * @brief Maximum number of processes in the system.
 *
 */
#define MAX_PROCESS_COUNT 1023

/**
 * @brief Fixed size of the GDT
 *
 */
#define GDT_SIZE 2*(1+MAX_PROCESS_COUNT)
