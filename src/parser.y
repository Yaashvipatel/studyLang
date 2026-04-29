//parser.y
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "ast.h"

extern int  yylex(void);
extern int  yylineno;
extern char *yytext;
extern int  lex_errors;


void yyerror(const char *msg);

ASTNode *parse_tree_root = NULL;
int      parse_errors    = 0;
static int error_reported = 0;

typedef struct ASTNode ASTNode;
%}

%union {
    int      ival;
    char     sval[128];
    ASTNode *node;
}

%token <sval> TOK_ACTION
%token <sval> TOK_SUBJECT
%token <sval> TOK_EVENT
%token <sval> TOK_TIMEOF
%token <sval> TOK_MODIFIER
%token <sval> TOK_PRIORITY
%token <sval> TOK_TIMEUNIT
%token <sval> TOK_CONNECTOR
%token <sval> TOK_FOR
%token <sval> TOK_PREP_TIME
%token <sval> TOK_PREP_OTHER
%token <ival> TOK_NUMBER

%type <node> program statement_list statement simple_stmt
%type <node> opt_modifier opt_timed opt_timeref opt_priority
%type <node> timeref_target opt_day

%start program

%%

program
    : statement_list
        {
            $$ = make_node(NT_PROGRAM, "program", NULL, NULL);
            ast_add_child($$, $1);
            parse_tree_root = $$;
        }
    ;

statement_list
    : statement
        {
            $$ = make_node(NT_STMT_LIST, "statement_list", NULL, NULL);
            ast_add_child($$, $1);
        }
    | statement_list TOK_CONNECTOR statement
        {
            $$ = $1;
            ASTNode *conn = make_node(NT_CONNECTOR, "connector", $2, NULL);
            ast_add_child($$, conn);
            ast_add_child($$, $3);
        }
    ;

statement
    : simple_stmt   { $$ = $1; }
    | error {
    if (!error_reported) {
        char errbuf[256];
        snprintf(errbuf, sizeof(errbuf),
            "Malformed statement near line %d. Expected: "
            "[MODIFIER] ACTION SUBJECT ...", yylineno);
        syn_error_add(yylineno, errbuf);
        parse_errors++;
        error_reported = 1;
    }
    yyclearin;
    $$ = make_node(NT_ERROR, "error_stmt", NULL, NULL);
}
    ;

simple_stmt
    : opt_modifier TOK_ACTION TOK_SUBJECT opt_timed opt_timeref opt_day opt_priority
        {
            $$ = make_node(NT_STATEMENT, "statement", NULL, NULL);
            if ($1) ast_add_child($$, $1);

            ASTNode *act = make_node(NT_ACTION,  "action",  $2, NULL);
            ASTNode *sub = make_node(NT_SUBJECT, "subject", $3, NULL);
            ast_add_child($$, act);
            ast_add_child($$, sub);

            if ($4) ast_add_child($$, $4);
            if ($5) ast_add_child($$, $5);
            if ($6) ast_add_child($$, $6);
            if ($7) ast_add_child($$, $7);
        }
    ;

opt_modifier
    : /* empty */   { $$ = NULL; }
    | TOK_MODIFIER  { $$ = make_node(NT_MODIFIER, "modifier", $1, NULL); }
    ;

opt_timed
    : /* empty */   { $$ = NULL; }
    | TOK_FOR TOK_NUMBER TOK_TIMEUNIT
        {
            char dur[16];
            snprintf(dur, sizeof(dur), "%d", $2);
            $$ = make_node(NT_TIMED_BLOCK, "timed_block", dur, $3);
        }
    ;

opt_timeref
    : /* empty */   { $$ = NULL; }
    | TOK_PREP_TIME timeref_target
        {
            $$ = make_node(NT_TIME_REF, "time_ref", $1, NULL);
            ast_add_child($$, $2);
        }
    ;

opt_day
    : /* empty */   { $$ = NULL; }
    | TOK_PREP_OTHER TOK_TIMEOF
        {
            $$ = make_node(NT_TIME_REF, "day_ref", $1, NULL);
            ASTNode *day = make_node(NT_TIMEOF, "day", $2, NULL);
            ast_add_child($$, day);
        }
    ;

timeref_target
    : TOK_EVENT  { $$ = make_node(NT_EVENT,  "event",  $1, NULL); }
    | TOK_TIMEOF { $$ = make_node(NT_TIMEOF, "timeof", $1, NULL); }
    ;

opt_priority
    : /* empty */   { $$ = NULL; }
    | TOK_PRIORITY  { $$ = make_node(NT_PRIORITY, "priority", $1, NULL); }
    ;

%%

void yyerror(const char *msg) {
    (void)msg;  /* errors collected in the 'error' rule */
}
