#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip; // Instruction pointer //Position de la prochaine instruction à exécuté
    Value stack[STACK_MAX]; //Pile pour les opération ((2+3)+(4+5)), garde le 5 + 9 en mémoire pour le résultat final
    Value* stackTop; //The pointer points at the array element just past the element containing the top value on the stack. Si ça pointure sur stack[0] alors la stack est vide !
} VM;
    //La stack n'est pas alloué dynamiquement car ce n'est pas un pointeur et qu'on connait sa taille fixe.

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);
void push(Value value);
Value pop();


#endif