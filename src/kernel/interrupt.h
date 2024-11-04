#pragma once

typedef void (*interupt_hander) (int number, int error);

void idt_init();
void idt_register(int interupt, interupt_hander handler, void* user);
void idt_await(int interupt);

void idt_test_function(int number, void* user);
