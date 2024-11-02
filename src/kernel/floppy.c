#include "floppy.h"

#include "print.h"
#include "types.h"

/* floppy disk IO functions */

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

static void outb(uint16_t port, uint8_t data){
    __asm__ volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}

static uint8_t inb(uint16_t port){
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

static bool floppy_wait_ready(uint32_t timeout){
    uint8_t msr = 0;
	for(uint32_t i = 0; i < timeout; i++){
        msr = inb(MAIN_STATUS_REGISTER);
        // if controller is ready for a command and not expecting an IN opcode
		if(msr & RQM && !(msr & DIO)){
			return true;
		}
	}
    kprintf("Error: Floppy not ready after timeout: %d, MSR: 0x%x\n", timeout, msr);
    return false;
}

static void floppy_send_command(uint8_t command){
    if(!floppy_wait_ready(0xffffff)){
        kprintf("Error: Floppy not ready while sending command\n");
        return;
    }

    outb(DATA_FIFO, command);
}

static uint8_t get_version(){
    floppy_send_command(VERSION);

    uint8_t version = inb(DATA_FIFO);

    // after receiving the version, the controller should be ready
    if(!floppy_wait_ready(0xffffff)){
        return 0;
    }
    return version;
}

static bool floppy_calibrate(uint8_t drive){
    floppy_send_command(RECALIBRATE);
    floppy_send_command(drive);

    if(!floppy_wait_ready(0xffffff)){
        kprintf("Error: Floppy not ready after recalibrate\n");
        return false;
    }

    floppy_send_command(SENSE_INTERRUPT);
    uint8_t status = inb(DATA_FIFO);
    kprintf("Calibrate status: 0x%x\n", status);
    uint8_t cylinder = inb(DATA_FIFO);
    kprintf("Calibrate cylinder: %d\n", cylinder);

    if(cylinder != 0 || status != (0x20 | drive)){
        kprintf("Error: Floppy calibration failed\n");
        return false;
    }

    return true;
}

void floppy_init(){
    /*======== verify version ========*/
    uint8_t version = get_version();
    kprintf("Floppy version: %x\n", version);

    if (version != 0x90){
        kprintf("Floppy version not supported\n");
        return;
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
    kprintf("Lock result: 0x%x\n", lock_result);

    if (!(lock_result & 0x10)){
        kprintf("Error: Floppy configuration lock failed\n");
        return;
    }

    /*========= reset ==========*/
    // disable controller
    outb(DIGITAL_OUTPUT_REGISTER, 0x00);
    // enable controller, select drive 1
    outb(DIGITAL_OUTPUT_REGISTER, RESET | DRIVE1_MOTOR | DRIVE1);

    // wait for a while
    for(uint32_t i = 0; i < 0xfffffff; i++){
        __asm__ volatile ("nop");
    }

    // sense interrupt 4 times
    for(uint8_t i = 0; i < 4; i++){
        floppy_send_command(SENSE_INTERRUPT);
        uint8_t status = inb(DATA_FIFO);
        kprintf("Reset status: 0x%x\n", status);
        uint8_t cylinder = inb(DATA_FIFO);
        kprintf("Reset cylinder: %d\n", cylinder);
    }

    /*======== specify drive parameters ========*/
    floppy_send_command(SPECIFY);
    floppy_send_command(0xdf);
    floppy_send_command(0x03); // non-DMA mode

    // recalibrate drive
    if(!floppy_calibrate(DRIVE1)){
        return;
    }

    kprintf("Floppy initialized successfully\n");
}


