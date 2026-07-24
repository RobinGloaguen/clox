#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,  // or
    PREC_AND, // and
    PREC_EQUALITY, // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,  // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // ! -
    PREC_CALL,  // . ()
    PREC_PRIMARY
} Precedence;

/*That ParseFn type is a simple typedef for a function type that takes no
arguments and returns nothing.
*/
typedef void (*ParseFn)();


typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;

Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}


static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

//On avance jusqu'au prochain token valide ou jusqu'a ce qu'on hit un EOF
static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        
        errorAtCurrent(parser.current.start);
    }

    
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}


static void emitReturn() {
    emitByte(OP_RETURN);
}

/*
Most of the work happens in addConstant() , which we defined back in an
earlier chapter. That adds the given value to the end of the chunk’s constant
table and returns its index. The new function’s job is mostly to make sure we
don’t have too many constants. Since the OP_CONSTANT instruction uses a
single byte for the index operand, we can store and load only up to 256
constants in a chunk.
*/
//Insert an entry in the constant table
//Le addConstant est implémenté dans Chunk
static uint8_t makeConstant(Value value) {
    //On rajoute dans le chunk->constant qui est un array 
    // la constante puis on nous donne l'index d'où se trouve cette constante dans le tableau
    //la variable de retour est l'indexe
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
    #ifdef DEBUG_PRINT_CODE
        if (!parser.hadError) {
            disassembleChunk(currentChunk(), "code");
        }
    #endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);


static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    //+1 sert a etre associatif a gauche (4+6)+5
    parsePrecedence((Precedence)(rule->precedence + 1)); //Si je récupère + je dois parser tout ce qui a une précédence supérieur a +
    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:  emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:  emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:
            emitByte(OP_ADD); break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:
             emitByte(OP_DIVIDE); break;
        default: return; // Unreachable.
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/*
We assume the token for the number literal has already been consumed and is
stored in previous . We take that lexeme and use the C standard library to
convert it to a double value. Then we generate the code to load that value using emitConstant
*/
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void unary() {
    TokenType operatorType = parser.previous.type;
    // Compile the operand.
    parsePrecedence(PREC_UNARY);
    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

/*
Parce qu'en C, le nom d'une fonction est automatiquement converti en pointeur vers cette fonction 
lorsqu'il apparaît dans une expression.

ici grouping, binary, unary désignent les fonctions elles même, cad les pointeurs
grouping() est l'execution
*/
ParseRule rules[] = {
[TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
[TOKEN_RIGHT_PAREN]  = {NULL, NULL, PREC_NONE},
[TOKEN_LEFT_BRACE] = {NULL,  NULL,  PREC_NONE},
[TOKEN_RIGHT_BRACE] = {NULL,  NULL, PREC_NONE},
[TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
[TOKEN_DOT] = {NULL, NULL, PREC_NONE},
[TOKEN_MINUS] = {unary, binary, PREC_TERM},
[TOKEN_PLUS] = {NULL, binary, PREC_TERM},
[TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
[TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
[TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
[TOKEN_BANG] = {unary, NULL, PREC_NONE},
[TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
[TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
[TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
[TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
[TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
[TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
[TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
[TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
[TOKEN_STRING] = {NULL, NULL, PREC_NONE},
[TOKEN_NUMBER] = {number, NULL, PREC_NONE},
[TOKEN_AND] = {NULL, NULL, PREC_NONE},
[TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
[TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
[TOKEN_FALSE] = {literal,  NULL, PREC_NONE},
[TOKEN_FOR] = {NULL, NULL, PREC_NONE},
[TOKEN_FUN] = {NULL, NULL, PREC_NONE},
[TOKEN_IF] = {NULL, NULL, PREC_NONE},
[TOKEN_NIL] = {literal,NULL, PREC_NONE},
[TOKEN_OR] = {NULL, NULL, PREC_NONE},
[TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
[TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
[TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
[TOKEN_THIS] = {NULL, NULL, PREC_NONE},
[TOKEN_TRUE] = {literal, NULL, PREC_NONE},
[TOKEN_VAR] ={NULL, NULL, PREC_NONE},
[TOKEN_WHILE] ={NULL, NULL, PREC_NONE},
[TOKEN_ERROR] ={NULL, NULL, PREC_NONE},
[TOKEN_EOF] ={NULL, NULL, PREC_NONE},
};

/*
l'opérateur d'appel de fonction () a une règle spéciale : 
si son opérande est un pointeur vers une fonction, il le déréférence implicitement.
*/

// Dans 2 + 3 le premier precedence envoyé = PREC_ASSIGNMENT
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    
    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
  
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;
    
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");

    endCompiler(); 
    return !parser.hadError;
}