#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[]) {

    initVM();

    Chunk chunk;
    initChunk(&chunk);
    //On ajoute la constante dans le arrayValue du chunk
    //ON écrit l'op Code
    //On point l'opérande suivant sur la constante en lui donnant son index.

    int constant = addConstant(&chunk, 1.2); //index d'où se trouve la constant dans le chunk
    //On ajoute l'opCode au chunk
    //on ajoute l'array l'index de son opérande au chunk. 
    // Qui doit etre la premiere valeur du tableau normalement

    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    constant = addConstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    
    writeChunk(&chunk, OP_ADD, 123);
    
    constant = addConstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    
    writeChunk(&chunk, OP_DIVIDE, 123);

    writeChunk(&chunk, OP_NEGATE, 123); //L'opérateur de négation se trouve après la valeur qu'il rend négative
    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");

    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);


    return 0;
}