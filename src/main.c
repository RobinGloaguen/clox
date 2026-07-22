#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        /* 
        string : un buffer pemettant de recueillir la chaîne à lire.
        maxLength : permet de spécifier le nombre de caractères maximal pouvant être utilisé au sein du buffer (uniquement dans le cas d'une utilisation de la fonction fgets).
        stream : ce paramètre permet d'indiquer le flux de caractères à utiliser pour la lecture. 
        */
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret(line);
    }
}

//The difficult part is that we want to allocate a big enough string to read the whole file, but we don’t know how big the file is until we’ve read it.
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    /*
    fseek(FILE* stream, long offset, int origine) déplace le curseur de position dans le fichier, sans rien lire.
    SEEK_END = se positionner à partir de la fin du fichier
    0L = un décalage de 0 par rapport à cette fin
    */
    fseek(file, 0L, SEEK_END);
    /*ftell(FILE* stream) retourne la position actuelle du curseur, en octets, depuis le début du fichier.
    Fonctionne car un char = 1 octet en C
    */
    size_t fileSize = ftell(file);
    //rewind : remettre le curseur au début
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    /*size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
    ptr	    Où stocker les données lues
    size	Taille (en octets) d'un seul élément
    nmemb	Nombre d'éléments à lire
    stream	Le fichier source

    fread retourne le nombre réel d'éléments effectivement lus
    */
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {

    initVM();

    if (argc == 1) {
        repl(); //REPL
    } else if (argc == 2) {
        runFile(argv[1]); //Path to a script to run
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();


    return 0;
}