#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
//When that flag is defined, we use our existing “debug” module to print out the chunk’s bytecode.
#define DEBUG_TRACE_EXECUTION
//When this flag is defined, the VM disassembles and prints each instruction right before executing it.

#endif