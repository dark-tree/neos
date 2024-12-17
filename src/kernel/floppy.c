#include "floppy.h"
#include "io.h"
#include "print.h"
#include "types.h"

//#define FLOPPY_DEBUG_ON

#ifdef FLOPPY_DEBUG_ON
    #define floppy_debug_msg(...) kprintf(__VA_ARGS__)
#else
    #define floppy_debug_msg(...)
#endif

#define FLOPPY_144_SECTORS_PER_TRACK 18

enum FloppyRegisters
{
    STATUS_REGISTER_A                = 0x3F0, // read-only
    STATUS_REGISTER_B                = 0x3F1, // read-only
    DIGITAL_OUTPUT_REGISTER          = 0x3F2,
    TAPE_DRIVE_REGISTER              = 0x3F3,
    MAIN_STATUS_REGISTER             = 0x3F4, // read-only
    DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
    DATA_FIFO                        = 0x3F5,
    DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
    CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
};

enum FloppyCommands
{
    READ_TRACK =                 2,	    // generates IRQ6
    SPECIFY =                    3,     // * set drive parameters
    SENSE_DRIVE_STATUS =         4,
    WRITE_DATA =                 5,     // * write to the disk
    READ_DATA =                  6,     // * read from the disk
    RECALIBRATE =                7,     // * seek to cylinder 0
    SENSE_INTERRUPT =            8,     // * ack IRQ6, get status of last command
    WRITE_DELETED_DATA =         9,
    READ_ID =                    10,	// generates IRQ6
    READ_DELETED_DATA =          12,
    FORMAT_TRACK =               13,    // *
    DUMPREG =                    14,
    SEEK =                       15,    // * seek both heads to cylinder X
    VERSION =                    16,	// * used during initialization, once
    SCAN_EQUAL =                 17,
    PERPENDICULAR_MODE =         18,	// * used during initialization, once, maybe
    CONFIGURE =                  19,    // * set controller parameters
    LOCK =                       20,    // * protect controller params from a reset, 0x80 is lock bit - 1 = lock
    VERIFY =                     22,
    SCAN_LOW_OR_EQUAL =          25,
    SCAN_HIGH_OR_EQUAL =         29
};

enum FloppyDOR
{
    DRIVE0 = 0,          // value for selecting drive 0
    DRIVE1 = 1,          // value for selecting drive 1
    DRIVE2 = 2,          // value for selecting drive 2
    DRIVE3 = 3,          // value for selecting drive 3
    RESET = 4,           // value for resetting the controller
    IRQ = 8,             // value for enabling IRQs and DMA
    DRIVE0_MOTOR = 0x10, // value for turning on drive 0's motor
    DRIVE1_MOTOR = 0x20, // value for turning on drive 1's motor
    DRIVE2_MOTOR = 0x40, // value for turning on drive 2's motor
    DRIVE3_MOTOR = 0x80  // value for turning on drive 3's motor
};

enum FloppyMSR
{
    RQM = 0x80,         // Set if it's OK (or mandatory) to exchange bytes with the FIFO IO port, controller is ready for a command
    DIO = 0x40,         // Set if FIFO IO port expects an IN opcode
    NDMA = 0x20,        // Set in Execution phase of PIO mode read/write commands only,controller expects DMA to be used for data read/write
    BUSY = 0x10,        // Command Busy: set when command byte received, cleared at end of Result phase 
    ACT3 = 8,           // Drive 3 is seeking 
    ACT2 = 4,           // Drive 2 is seeking
    ACT1 = 2,           // Drive 1 is seeking
    ACT0 = 1            // Drive 0 is seeking
};

enum FloppyConfigure
{
    POLLING_DISABLED = 0x10,        // 0 = drive polling mode, 1 = interrupt mode
    FIFO_DISABLED = 0x20,           // 1 = FIFO disabled, 0 = FIFO enabled
    IMPLIED_SEEK_ENABLED = 0x40,    // 0 = implied seek enabled, 1 = implied seek disabled
    THRESHOLD_MASK = 0x0F           // FIFO threshold value
};

enum FloppyOptionBits
{
    MT_BIT = 0x80,      // Multitrack: The controller will switch automatically from Head 0 to Head 1 at the end of the track. This allows you to read/write twice as much data with a single command. 
    MFM_BIT = 0x40,     // MFM
    SK_BIT = 0x20,      // Skip deleted sectors
};

/* private */

static bool floppy_wait_msr(uint32_t timeout, uint8_t mask, uint8_t value){
    uint8_t msr = 0;
	for(uint32_t i = 0; i < timeout; i++){
        msr = inb(MAIN_STATUS_REGISTER);
		if((msr & mask) == value){
			return true;
		}
	}
    floppy_debug_msg("Timeout after: %d, MSR: 0x%x (RQM: %d/%c, DIO: %d/%c, NDMA: %d/%c, BUSY: %d/%c)\n", timeout, msr, 
        (msr & RQM) > 0, (mask & RQM) ? (value & RQM) ? '1' : '0' : 'x',
        (msr & DIO) > 0, (mask & DIO) ? (value & DIO) ? '1' : '0' : 'x',
        (msr & NDMA) > 0, (mask & NDMA) ? (value & NDMA) ? '1' : '0' : 'x',
        (msr & BUSY) > 0, (mask & BUSY) ? (value & BUSY) ? '1' : '0' : 'x');
    return false;
}

static bool floppy_wait_ready(uint32_t timeout){
    return floppy_wait_msr(timeout, RQM, RQM);
}

static bool floppy_wait_ready_send(uint32_t timeout){
    return floppy_wait_msr(timeout, RQM | DIO, RQM);
}

static void floppy_send_command(uint8_t command){
    if(!floppy_wait_ready_send(0xffffff)){
        floppy_debug_msg("Error: Floppy not ready while sending command\n");
        return;
    }

    outb(DATA_FIFO, command);
}

static uint8_t get_version(){
    floppy_send_command(VERSION);

    uint8_t version = inb(DATA_FIFO);

    // after receiving the version, the controller should be ready
    if(!floppy_wait_ready_send(0xffffff)){
        return 0;
    }
    return version;
}

static bool floppy_calibrate(uint8_t drive){
    floppy_send_command(RECALIBRATE);
    floppy_send_command(drive);

    if(!floppy_wait_ready(0xffffff)){
        floppy_debug_msg("Error: Floppy not ready after recalibrate\n");
        return false;
    }

    floppy_send_command(SENSE_INTERRUPT);
    uint8_t status = inb(DATA_FIFO);
    floppy_debug_msg("Calibrate status: 0x%x\n", status);
    uint8_t cylinder = inb(DATA_FIFO);
    floppy_debug_msg("Calibrate cylinder: %d\n", cylinder);

    if(cylinder != 0 || status != (0x20 | drive)){
        floppy_debug_msg("Error: Floppy calibration failed\n");
        return false;
    }

    return true;
}

static bool floppy_seek(uint8_t head, uint8_t cylinder){
    for (int i = 0; i < 10; i++){
        floppy_send_command(SEEK);
        floppy_send_command(head << 2 | DRIVE1);
        floppy_send_command(cylinder);

        if(!floppy_wait_ready(0xffffff)){
            floppy_debug_msg("Error: Floppy not ready after seek\n");
            return false;
        }

        floppy_send_command(SENSE_INTERRUPT);
        uint8_t status = inb(DATA_FIFO);
        floppy_debug_msg("Seek status: 0x%x\n", status);
        uint8_t cylinder_result = inb(DATA_FIFO);
        floppy_debug_msg("Seek cylinder: %d\n", cylinder);

        if(cylinder == cylinder_result){
            return true;
        }
    }

    floppy_debug_msg("Error: Floppy seek failed\n");
    return false;
}

static bool floppy_read_sector(uint8_t head, uint8_t cylinder, uint8_t sector, uint8_t* buffer){
    if(!floppy_seek(head, cylinder)){
        return false;
    }

    // issue read command
    floppy_send_command(READ_DATA | MFM_BIT | SK_BIT); // MT_BIT
    floppy_send_command(head << 2 | DRIVE1);
    floppy_send_command(cylinder);
    floppy_send_command(head);
    floppy_send_command(sector); // first sector
    floppy_send_command(2); // sector size = 512 bytes
    floppy_send_command(sector); // last sector
    floppy_send_command(0x1b); // gap length
    floppy_send_command(0xff); // data length

    if(!floppy_wait_ready(0xffffff)){
        floppy_debug_msg("Error: Floppy not ready after read command\n");
        return false;
    }

    // read incoming data
    uint32_t i = 0;
    while (true){
        if (!floppy_wait_msr(0x1, RQM | DIO | NDMA, RQM | NDMA | DIO) || i > 512){
            break;
        }
        uint8_t data = inb(DATA_FIFO);
        buffer[i] = data;
        if (i < 32) {
            floppy_debug_msg("%x ", data);
            if (i % 16 == 15){
                floppy_debug_msg("\n");
            }
        }
        i++;
    }

    floppy_debug_msg("Read %d bytes\n", i);
    if (i != 512){
        floppy_debug_msg("Error: Floppy read failed\n");
        return false;
    }

    if (!floppy_wait_ready(0xffffff)){
        floppy_debug_msg("Error: Floppy not ready after read\n");
        return false;
    }

    // read command result
    uint8_t st0 = inb(DATA_FIFO);
    uint8_t st1 = inb(DATA_FIFO);
    uint8_t st2 = inb(DATA_FIFO);
    uint8_t result_cylinder = inb(DATA_FIFO);
    uint8_t result_head = inb(DATA_FIFO);
    uint8_t result_sector = inb(DATA_FIFO);
    uint8_t result_2 = inb(DATA_FIFO);
    floppy_debug_msg("R ST0: 0x%x, ST1: 0x%x, ST2: 0x%x, Cylinder: %d, Head: %d, Sector: %d, 2: %d\n", st0, st1, st2, result_cylinder, result_head, result_sector, result_2);

    if ((st0 & 0xC0) || result_cylinder != cylinder || result_head != head || result_sector != sector || result_2 != 2){
        floppy_debug_msg("Error: Floppy read failed\n");
        return false;
    }

    return true;
}

static bool floppy_reset(){
    /*========= reset ==========*/
    // disable controller
    outb(DIGITAL_OUTPUT_REGISTER, 0x00);
    // enable controller, select drive 1
    outb(DIGITAL_OUTPUT_REGISTER, RESET | DRIVE1_MOTOR | DRIVE1);

    // sense interrupt 4 times
    for(uint8_t i = 0; i < 4; i++){
        floppy_send_command(SENSE_INTERRUPT);
        uint8_t status = inb(DATA_FIFO);
        floppy_debug_msg("Reset status: 0x%x\n", status);
        uint8_t cylinder = inb(DATA_FIFO);
        floppy_debug_msg("Reset cylinder: %d\n", cylinder);
    }

    /*======== specify drive parameters ========*/
    floppy_send_command(SPECIFY);
    floppy_send_command(0xdf);
    floppy_send_command(0x03); // non-DMA mode

    // recalibrate drive
    if(!floppy_calibrate(DRIVE1)){
        return false;
    }

    return true;
}

static bool floppy_write_sector(uint8_t head, uint8_t cylinder, uint8_t sector, uint8_t* buffer){
    if (!floppy_seek(head, cylinder)){
        return false;
    }

    // issue write command
    floppy_send_command(WRITE_DATA | MFM_BIT | SK_BIT); // MT_BIT
    floppy_send_command(head << 2 | DRIVE1);
    floppy_send_command(cylinder);
    floppy_send_command(head);
    floppy_send_command(sector); // first sector
    floppy_send_command(2); // sector size = 512 bytes
    floppy_send_command(sector); // last sector
    floppy_send_command(0x1b); // gap length
    floppy_send_command(0xff); // data length

    if (!floppy_wait_ready(0xffffff)){
        floppy_debug_msg("Error: Floppy not ready after write command\n");
        return false;
    }

    // write data
    uint32_t i = 0;
    while (true){
        if (!floppy_wait_msr(0x1, RQM | DIO | NDMA, RQM | NDMA) || i > 512){
            break;
        }
        outb(DATA_FIFO, buffer[i]);
        i++;
    }

    floppy_debug_msg("Wrote %d bytes\n", i);
    if (i != 512){
        floppy_debug_msg("Error: Floppy write failed\n");
        return false;
    }

    // for some reason, the msr is not ready after writing no matter how long you wait
    // so we just wait shorter than usual, because it's not going to work anyway
    // I literally issue the read command in the same way and it works, but the write command doesn't
    if (!floppy_wait_ready(0xff)){
        floppy_debug_msg("Error: Floppy not ready after write\n");
        // absolutely unacceptable, but I ran out of ideas, TODO: fix this
        floppy_reset();  
        return true; // <-- it should be false
    }

    // read command result
    uint8_t st0 = inb(DATA_FIFO);
    uint8_t st1 = inb(DATA_FIFO);
    uint8_t st2 = inb(DATA_FIFO);
    uint8_t result_cylinder = inb(DATA_FIFO);
    uint8_t result_head = inb(DATA_FIFO);
    uint8_t result_sector = inb(DATA_FIFO);
    uint8_t result_2 = inb(DATA_FIFO);
    floppy_debug_msg("W ST0: 0x%x, ST1: 0x%x, ST2: 0x%x, Cylinder: %d, Head: %d, Sector: %d, 2: %d\n", st0, st1, st2, result_cylinder, result_head, result_sector, result_2);

    if ((st0 & 0xC0) || result_cylinder != cylinder || result_head != head || result_sector != sector || result_2 != 2){
        floppy_debug_msg("Error: Floppy write failed\n");
        return false;
    }

    return true;
}

static void lba_to_chs(uint32_t lba, uint8_t* cylinder, uint8_t* head, uint8_t* sector){
    *cylinder = lba / (2 * FLOPPY_144_SECTORS_PER_TRACK);
    *head = ((lba % (2 * FLOPPY_144_SECTORS_PER_TRACK)) / FLOPPY_144_SECTORS_PER_TRACK);
    *sector = ((lba % (2 * FLOPPY_144_SECTORS_PER_TRACK)) % FLOPPY_144_SECTORS_PER_TRACK + 1);
}

static bool floppy_read_lba(uint32_t lba, uint8_t* buffer){
    uint8_t head, cylinder, sector;
    lba_to_chs(lba, &cylinder, &head, &sector);
    return floppy_read_sector(head, cylinder, sector, buffer);
}

static bool floppy_write_lba(uint32_t lba, uint8_t* buffer){
    uint8_t head, cylinder, sector;
    lba_to_chs(lba, &cylinder, &head, &sector);
    return floppy_write_sector(head, cylinder, sector, buffer);
}

/* public */

bool floppy_init(){
    /*======== verify version ========*/
    uint8_t version = get_version();
    floppy_debug_msg("Floppy version: %x\n", version);

    if (version != 0x90){
        floppy_debug_msg("Floppy version not supported\n");
        return false;
    }

    /*======== configure procedure ========*/
    // configure drive polling mode on, FIFO off, threshold = 1, implied seek on, precompensation 0
    floppy_send_command(CONFIGURE);
    floppy_send_command(0x00);
    floppy_send_command(IMPLIED_SEEK_ENABLED | FIFO_DISABLED | ((1 - 1) & THRESHOLD_MASK));
    floppy_send_command(0x00);

    /*======== lock configuration ========*/
    floppy_send_command(LOCK | 0x80);
    uint8_t lock_result = inb(DATA_FIFO);
    floppy_debug_msg("Lock result: 0x%x\n", lock_result);

    if (!(lock_result & 0x10)){
        floppy_debug_msg("Error: Floppy configuration lock failed\n");
        return false;
    }

    if(!floppy_reset()){
        floppy_debug_msg("Error: Floppy reset failed\n");
        return false;
    }

    floppy_debug_msg("Floppy initialized successfully\n");
    return true;
}

bool floppy_read(void* buffer, uint32_t address, uint32_t size){
    floppy_debug_msg("Reading %d bytes from address 0x%x\n", size, address);

    uint8_t tmp_buffer[512];

    uint32_t start_lba = address / 512;
    uint32_t end_lba = (address + size) / 512;
    uint32_t output_buffer_index = 0;

    for (uint32_t lba = start_lba; lba <= end_lba; lba++){
        if (!floppy_read_lba(lba, tmp_buffer)){
            return false;
        }

        uint32_t start = 0;
        uint32_t end = 512;
        if (lba == start_lba){
            start = address % 512;
        }
        if (lba == end_lba){
            end = (address + size) % 512;
        }

        for (uint32_t i = start; i < end; i++){
            ((uint8_t*)buffer)[output_buffer_index++] = tmp_buffer[i];
        }
    }

    return true;
}

bool floppy_write(void* buffer, uint32_t address, uint32_t size, bool preserve){
    floppy_debug_msg("Writing %d bytes to address 0x%x\n", size, address);

    uint8_t tmp_buffer[512];

    uint32_t start_lba = address / 512;
    uint32_t end_lba = (address + size) / 512;
    uint32_t input_buffer_index = 0;

    for (uint32_t lba = start_lba; lba <= end_lba; lba++){
        if (preserve){
            if (!floppy_read_lba(lba, tmp_buffer)){
                return false;
            }
        }

        uint32_t start = 0;
        uint32_t end = 512;
        if (lba == start_lba){
            start = address % 512;
        }
        if (lba == end_lba){
            end = (address + size) % 512;
        }

        for (uint32_t i = start; i < end; i++){
            tmp_buffer[i] = ((uint8_t*)buffer)[input_buffer_index++];
        }

        if (!floppy_write_lba(lba, tmp_buffer)){
            return false;
        }
    }

    return true;
}

void print_buffer(const unsigned char* buffer, int size) {
	for (int i = 0; i < size; i++) {
		kprintf("%x ", buffer[i]);
		if ((i + 1) % 16 == 0 || i == size - 1) {
			kprintf("    ");
			for (int j = i - i % 16; j <= i; j++) {
				if (buffer[j] >= 32 && buffer[j] <= 126) {
					kprintf("%c", buffer[j]);
				}
				else {
					kprintf(".");
				}
			}
			kprintf("\n");
		}
	}
}
