#include "gdt.h"
#include "types.h"
#include "tables.h"
#include "config.h"
#include "kmalloc.h"
#include "print.h"

#define EMPTY_GDT_ENTRY 0x0000000000C09300
#define EMPTY_CODE 0x0000000000C09B00

uint64_t* gdt;


uint64_t bswap(uint64_t number)
{
    uint64_t result = 0;
    uint64_t mask = 0xFF;
    for(int i=0;i<8;i++)
    {
        result += ((mask & number)>>(i*8));
        if(i!=7)
        {
            result = result << 8;
        }
        mask = mask << 8;
    }
    return result;
}

uint64_t dwswap(uint64_t number)
{
    uint64_t result = 0;
    uint64_t mask = 0xFFFFFFFF;
    result += (mask & number);
    mask = mask << 32;
    result = result << 32;
    result += ((mask & number)>>32);
    return result;
}

uint64_t set_gdt_field_mask(uint32_t offset, uint32_t size)
{
    uint32_t size_low = (size<<16)>>16;
    uint32_t size_high = size>>16;
    uint32_t offset_low = (offset<<16)>>16;
    uint32_t offset_highmid = offset>>16;
    uint32_t offset_mid = offset_highmid & 0xFF;
    uint32_t offset_high = offset_highmid>>8;

    uint64_t mask = offset_low;
    mask = mask << 16;
    mask = mask | size_low;
    mask = mask << 8;
    mask = mask | offset_high;
    mask = mask << 8;
    mask = mask | size_high;
    mask = mask << 16;
    mask = mask | offset_mid;
    return mask;
}

void ginit()
{
    gdt = kmalloc(GDT_SIZE*8+8);
    gdt[0] = 0;
    for(int i=1;i<GDT_SIZE+1;i+=2)
    {
        gdt[i] = dwswap(EMPTY_CODE);
        gdt[i] = dwswap(EMPTY_GDT_ENTRY);
    }
    uint64_t kernelCode = 0x0000FFFF00CF9B00;
    uint64_t kernelData = 0x0000FFFF00CF9300;

    kernelCode = dwswap(kernelCode);
    kernelData = dwswap(kernelData);

    gdt[1]=kernelCode;
    gdt[2]=kernelData;
    kprintf("New GDTR set: %d\n", gdt);
    gdtr_store((void*)gdt, GDT_SIZE*8+8);
    gdtr_switch(2, 1);
}


int gput(uint32_t offset, uint32_t size)
{
    //size = size>>10;
    for(int i=3;i<GDT_SIZE+1;i+=2)
    {
        if(gdt[i] == dwswap(EMPTY_GDT_ENTRY) || gdt[i] == dwswap(EMPTY_CODE))
        {
            uint64_t mask = set_gdt_field_mask(offset, size);
            uint64_t code = EMPTY_CODE;
            uint64_t data = EMPTY_GDT_ENTRY;
            code = code | mask;
            data = data | mask;
            gdt[i] = dwswap(code);
            gdt[i+1] = dwswap(data);
            return i;
        }
    }
}

void grm(int i)
{
    gdt[i] = dwswap(EMPTY_CODE);
    gdt[i+1] = dwswap(EMPTY_GDT_ENTRY);
}

void gad(int i, uint32_t offset, uint32_t size)
{
    uint64_t mask = set_gdt_field_mask(offset, size);

    uint64_t code = EMPTY_CODE;
    uint64_t data = EMPTY_GDT_ENTRY;
    code = code | mask;
    data = data | mask;

    gdt[i] = code;//code;
    gdt[i+1] = data;//data;
}
