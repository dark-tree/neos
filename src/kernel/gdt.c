#include "gdt.h"
#include "types.h"
#include "tables.h"
#include "config.h"
#include "kmalloc.h"

#define EMPTY_GDT_ENTRY 0x000000000040F300
#define EMPTY_CODE 0x000000000040FB00

uint64_t* gdt;


void ginit()
{
    gdt = kmalloc(GDT_SIZE*8+8);
    gdt[0] = 0;
    for(int i=1;i<GDT_SIZE+1;i++)
    {
        gdt[i] = EMPTY_GDT_ENTRY;
    }
    uint64_t kernelCode = 0x0000FFFF00CF9B00;
    uint64_t kernelData = 0x0000FFFF00CF9300;
    gdt[1]=kernelCode;
    gdt[2]=kernelData;
    gdtr_store((void*)gdt, GDT_SIZE*8+8);
    gdtr_switch(2, 1);
}


int gput(uint32_t offset, uint32_t size)
{
    size = size>>10;
    for(int i=3;i<GDT_SIZE+1;i+=2)
    {
        if(gdt[i] == EMPTY_GDT_ENTRY)
        {
            uint32_t size_low = (size<<16)>>16;
            uint32_t size_high = size>>16;
            uint32_t offset_low = (offset<<16)>>16;
            uint32_t offset_highmid = offset>>16;
            uint32_t offset_mid = offset_highmid & 0xFF;
            uint32_t offset_high = offset_highmid>>8;

            uint64_t mask = offset_high;
            mask = mask << 8;
            mask = mask | size_high;
            mask = mask << 16;
            mask = mask | offset_mid;
            mask = mask << 16;
            mask = mask | offset_low;
            mask = mask << 16;
            mask = mask | size_low;

            uint64_t code = EMPTY_CODE;
            uint64_t data = EMPTY_GDT_ENTRY;
            code = code | mask;
            data = data | mask;
            gdt[i] = code;
            gdt[i+1] = data;
            return i;
        }
    }
}

void grm(int i)
{
    gdt[i] = EMPTY_GDT_ENTRY;
    gdt[i+1] = EMPTY_GDT_ENTRY;
}

void gad(int i, uint32_t offset, uint32_t size)
{
     size = size>>10;
     uint32_t size_low = (size<<16)>>16;
     uint32_t size_high = size>>16;
     uint32_t offset_low = (offset<<16)>>16;
     uint32_t offset_highmid = offset>>16;
     uint32_t offset_mid = offset_highmid & 0xFF;
     uint32_t offset_high = offset_highmid>>8;

     uint64_t mask = offset_high;
     mask = mask << 8;
     mask = mask | size_high;
     mask = mask << 16;
     mask = mask | offset_mid;
     mask = mask << 16;
     mask = mask | offset_low;
     mask = mask << 16;
     mask = mask | size_low;

     uint64_t code = EMPTY_CODE;
     uint64_t data = EMPTY_GDT_ENTRY;
     code = code | mask;
     data = data | mask;

     gdt[i] = 0x0000FFFF00CF9B00;//code;
     gdt[i+1] = 0x0000FFFF00CF9300;//data;
}
