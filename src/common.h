#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKENS     512
#define MAX_LEN         64
#define MAX_LEX_ERRORS  64
#define MAX_SYN_ERRORS  64
#define MAX_SEM_ERRORS  64

typedef enum {
    TT_ACTION,
    TT_SUBJECT,
    TT_EVENT,
    TT_TIMEOF,
    TT_PREPOSITION,
    TT_CONNECTOR,
    TT_TIMEUNIT,
    TT_MODIFIER,
    TT_PRIORITY,
    TT_NUMBER,
    TT_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char      value[MAX_LEN];
    int       line;
} Token;

typedef struct {
    int  line;
    char message[256];
} PhaseError;

extern Token      token_table[];
extern int        token_count;
extern int        lex_errors;

extern PhaseError lex_error_list[];
extern int        lex_error_count;

extern PhaseError syn_error_list[];
extern int        syn_error_count;

extern PhaseError sem_error_list[];
extern int        sem_error_count;

void lex_error_add(int line, const char *word);
void syn_error_add(int line, const char *msg);
void sem_error_add(const char *msg);


static inline const char *token_type_name(TokenType t) {
    switch (t) {
        case TT_ACTION:      return "ACTION";
        case TT_SUBJECT:     return "SUBJECT";
        case TT_EVENT:       return "EVENT";
        case TT_TIMEOF:      return "TIMEOF/DAY";
        case TT_PREPOSITION: return "PREPOSITION";
        case TT_CONNECTOR:   return "CONNECTOR";
        case TT_TIMEUNIT:    return "TIMEUNIT";
        case TT_MODIFIER:    return "MODIFIER";
        case TT_PRIORITY:    return "PRIORITY";
        case TT_NUMBER:      return "NUMBER";
        default:             return "UNKNOWN";
    }
}

#endif 
