#include "common.h"
#include "vm.h"
#include <stdio.h>
#include "compiler.h"
#include "debug.h"
VM vm;

static void resetStack() {
    vm.stackTop = vm.stack; //La pile est initialisé comme vide
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


/*This is the single most important function in all of clox, by far. When the
interpreter executes a user’s program, it will spend something like 90% of its
time inside run() . It is the beating heart of the VM.
*/
static InterpretResult run() {
    //Comment le ip passe d'une instruction a une autre en sautant les opérandes de certaines ?
#define READ_BYTE() (*vm.ip++) //The READ_BYTE macro reads the byte currently pointed at by ip and then advances the instruction pointer.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()]) //Le READ_BYTE lit l'opérande et passe à l'instruction suivante    
#define BINARY_OP(op) \
    do { \
    double b = pop(); \
    double a = pop(); \
    push(a op b); \
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
            case OP_ADD:        BINARY_OP(+); break;
            case OP_SUBTRACT:   BINARY_OP(-); break;
            case OP_MULTIPLY:   BINARY_OP(*); break;
            case OP_DIVIDE:     BINARY_OP(/); break;
            case OP_NEGATE:     push(-pop()); break;
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
    compile(source);
    return INTERPRET_OK;
}