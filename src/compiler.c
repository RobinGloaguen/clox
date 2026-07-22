#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
    initScanner(source);
    
    int line = -1;
    for (;;) {
        Token token = scanToken();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("         | ");
        }
        //Ici le *s permet de donner la longueur du token a afficher en argument avec token.length
        //Car le token n'a pas de /0 pour s'ignifier l'arrêt
        //Le token type est la valeur numéric du token dans l'énumération de TOKEN
        printf("%2d '%.*s'\n", token.type, token.length, token.start);
        if (token.type == TOKEN_EOF) break;
    }
}