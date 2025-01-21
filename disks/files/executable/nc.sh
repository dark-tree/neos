#!/bin/env bash

nasm -f elf32 $1.s -o $1.o
ld -melf_i386 -nostdlib --gc-sections --strip-all ./$1.o -o ./$1
rm ./$1.o
