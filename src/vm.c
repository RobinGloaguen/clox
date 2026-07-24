#include "common.h"
#include "vm.h"
#include <stdarg.h>
#include <stdio.h>
#include "compiler.h"
#include "debug.h"
VM vm;

static void resetStack() {
    vm.stackTop = vm.stack; //La pile est initialisé comme vide
}

/*
You’ve certainly called variadic functions—ones that take a varying number of
arguments—in C before: printf() is one. But you may not have defined your
own. This book isn’t a C tutorial, so I’ll skim over it here, but basically the ...
and va_list stuff let us pass an arbitrary number of arguments to
runtimeError() . It forwards those on to vfprintf() , which is the flavor of
printf() that takes an explicit va_list .
*/
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}


void initVM() {
    resetStack();
}

void freeVM() {
}

//Attention : On ne vérifie pas si la stack est pleine ?
void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/*This is the single most important function in all of clox, by far. When the
interpreter executes a user’s program, it will spend something like 90% of its
time inside run() . It is the beating heart of the VM.
*/
static InterpretResult run() {
    //Comment le ip passe d'une instruction a une autre en sautant les opérandes de certaines ?
#define READ_BYTE() (*vm.ip++) //The READ_BYTE macro reads the byte currently pointed at by ip and then advances the instruction pointer.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()]) //Le READ_BYTE lit l'opérande et passe à l'instruction suivante    
#define BINARY_OP(valueType, op) \
    do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
    runtimeError("Operands must be numbers."); \
    return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
    } while (false)
//le do while(false) permet d'avoir une macro multi instruciton perçu comme une seul qui fonctionne dans un case
//Car autrement on a un ";" en trop ce qui fera bugguer le switch avec la macro
    for (;;) {

// Macro qui permet juste d'avoir un affichage pour le debug
#ifdef DEBUG_TRACE_EXECUTION 
    printf("        ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
    //ip est un pointeur, une adresse tandis que nous voulons un offset entier. 
    // Donc on fait le différence entre ip et la premiere adresse du chunk du chunk
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD:        BINARY_OP(NUMBER_VAL, +); break;
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) { //peek regarde la premiere valeur du stack
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    InterpretResult result = run();
    
    freeChunk(&chunk);
    return result;
}

