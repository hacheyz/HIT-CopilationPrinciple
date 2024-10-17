#ifndef NODETYPE_H
#define NODETYPE_H

typedef enum {
    INTERNAL,
    TOKEN_OCT,
    TOKEN_DEC,
    TOKEN_HEX,
    TOKEN_FLOAT,
    TOKEN_ID,
    TOKEN_TYPE,
    TOKEN_KEY,
    TOKEN_SEP,
    TOKEN_OP,
    TOKEN_PARENS,
} NodeType;

#endif