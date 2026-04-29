#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "ast.h"

extern int      yyparse(void);
extern void     yy_scan_string(const char *);
extern void     yylex_destroy(void);
extern ASTNode *parse_tree_root;
extern int      parse_errors;
extern int      lex_errors;

int  semanticAnalyzer(ASTNode *root);
void generateICG(ASTNode *root);
void print_phase3(FILE *fp);

static void json_escape(FILE *fp, const char *s) {
    fputc('"', fp);
    for (; *s; s++) {
        switch (*s) {
            case '"':  fputs("\\\"", fp); break;
            case '\\': fputs("\\\\", fp); break;
            case '\n': fputs("\\n",  fp); break;
            case '\r': fputs("\\r",  fp); break;
            case '\t': fputs("\\t",  fp); break;
            default:   fputc(*s, fp);     break;
        }
    }
    fputc('"', fp);
}

static void emit_errors_json(FILE *fp,
                              const char *tag,
                              PhaseError *list,
                              int count) {
    fprintf(fp, "\n@@%s_ERRORS@@[", tag);
    for (int i = 0; i < count; i++) {
        if (i) fputc(',', fp);
        fprintf(fp, "{\"line\":%d,\"msg\":", list[i].line);
        json_escape(fp, list[i].message);
        fputc('}', fp);
    }
    fprintf(fp, "]\n");
}

static void compile(const char *input) {
   
    token_count     = 0;
    lex_errors      = 0;
    parse_errors    = 0;
    parse_tree_root = NULL;
    lex_error_count = 0;
    syn_error_count = 0;
    sem_error_count = 0;

   
    yy_scan_string(input);
    int parse_ret = yyparse();
    yylex_destroy();

    
    printf("\n@@PHASE1_START@@\n");
    printf("=== PHASE 1: LEXICAL ANALYSIS ===\n\n");
    printf("  Input: %s\n\n", input);

    if (token_count == 0) {
        printf("  No tokens recognised.\n");
    } else {
        printf("  Token Stream:\n");
        printf("  %-5s  %-18s  %s\n", "Tok#", "Type", "Value");
        printf("  %-5s  %-18s  %s\n", "----", "------------------", "-------");
        for (int i = 0; i < token_count; i++) {
            printf("  %-5d  %-18s  %s\n",
                   i + 1,
                   token_type_name(token_table[i].type),
                   token_table[i].value);
        }
    }

    if (lex_errors > 0) {
        printf("\n  [FAILED] %d lexical error(s) found.\n", lex_errors);
        printf("  Unknown words were encountered. Check spelling and\n");
        printf("  refer to the Grammar Guide for valid keywords.\n");
    } else {
        printf("\n  [PASSED] Lexical analysis complete. %d token(s) recognised.\n", token_count);
    }
    printf("@@PHASE1_END@@\n");

    emit_errors_json(stdout, "LEX", lex_error_list, lex_error_count);

    if (lex_errors > 0) {
        printf("\n@@PHASE2_START@@\n");
        printf("=== PHASE 2: SYNTAX ANALYSIS ===\n\n");
        printf("  [SKIPPED] Cannot proceed — lexical errors must be fixed first.\n");
        printf("@@PHASE2_END@@\n");
        emit_errors_json(stdout, "SYN", syn_error_list, 0);

        printf("\n@@PHASE3_START@@\n");
        printf("=== PHASE 3: SEMANTIC ANALYSIS ===\n\n");
        printf("  [SKIPPED] Cannot proceed — fix earlier phase errors first.\n");
        printf("@@PHASE3_END@@\n");
        emit_errors_json(stdout, "SEM", sem_error_list, 0);

        printf("\n@@PHASE4_START@@\n");
        printf("=== PHASE 4: INTERMEDIATE CODE GENERATION ===\n\n");
        printf("  [SKIPPED] Cannot proceed — fix earlier phase errors first.\n");
        printf("@@PHASE4_END@@\n");
        return;
    }


    printf("\n@@PHASE2_START@@\n");
    printf("=== PHASE 2: SYNTAX ANALYSIS ===\n\n");

    printf("  Grammar (BNF):\n");
    printf("  ------------------------------------------------------------------\n");
    printf("  program        ->  statement_list\n");
    printf("  statement_list ->  statement\n");
    printf("                 |   statement_list CONNECTOR statement\n");
    printf("  statement      ->  simple_stmt\n");
    printf("  simple_stmt    ->  opt_modifier ACTION SUBJECT\n");
    printf("                     opt_timed opt_timeref opt_day opt_priority\n");
    printf("  opt_modifier   ->  e  |  MODIFIER\n");
    printf("  opt_timed      ->  e  |  'for' NUMBER TIMEUNIT\n");
    printf("  opt_timeref    ->  e  |  PREP_TIME timeref_target\n");
    printf("  opt_day        ->  e  |  PREP_OTHER TIMEOF\n");
    printf("  timeref_target ->  EVENT  |  TIMEOF\n");
    printf("  opt_priority   ->  e  |  PRIORITY\n");
    printf("  ------------------------------------------------------------------\n\n");

    printf("  Token Stream (for grammar matching):\n");
    printf("  %-5s  %-18s  %s\n", "Tok#", "Type", "Value");
    printf("  %-5s  %-18s  %s\n", "----", "------------------", "-------");
    for (int i = 0; i < token_count; i++) {
        printf("  %-5d  %-18s  %s\n",
               i + 1,
               token_type_name(token_table[i].type),
               token_table[i].value);
    }

    if (parse_ret != 0 || parse_errors > 0) {
        printf("\n  [FAILED] %d syntax error(s) found.\n", parse_errors);
        printf("  The token sequence does not match any valid grammar rule.\n");
        printf("@@PHASE2_END@@\n");
        emit_errors_json(stdout, "SYN", syn_error_list, syn_error_count);

        printf("\n@@PHASE3_START@@\n");
        printf("=== PHASE 3: SEMANTIC ANALYSIS ===\n\n");
        printf("  [SKIPPED] Cannot proceed — fix syntax errors first.\n");
        printf("@@PHASE3_END@@\n");
        emit_errors_json(stdout, "SEM", sem_error_list, 0);

        printf("\n@@PHASE4_START@@\n");
        printf("=== PHASE 4: INTERMEDIATE CODE GENERATION ===\n\n");
        printf("  [SKIPPED] Cannot proceed — fix syntax errors first.\n");
        printf("@@PHASE4_END@@\n");
        return;
    }

    printf("\n  Parse Tree (AST):\n");
    int flags[64] = {0};
    ast_print(parse_tree_root, 0, flags);
    printf("\n  [PASSED] Syntax analysis complete. Program recognised successfully.\n");
    printf("@@PHASE2_END@@\n");
    emit_errors_json(stdout, "SYN", syn_error_list, 0);


    printf("\n@@PHASE3_START@@\n");
    int sem_ok = semanticAnalyzer(parse_tree_root);
    print_phase3(stdout);
    printf("@@PHASE3_END@@\n");
    emit_errors_json(stdout, "SEM", sem_error_list, sem_error_count);

    if (!sem_ok) {
        ast_free(parse_tree_root);
        parse_tree_root = NULL;

        printf("\n@@PHASE4_START@@\n");
        printf("=== PHASE 4: INTERMEDIATE CODE GENERATION ===\n\n");
        printf("  [SKIPPED] Cannot proceed — fix semantic errors first.\n");
        printf("@@PHASE4_END@@\n");
        return;
    }

    printf("\n@@PHASE4_START@@\n");
    generateICG(parse_tree_root);
    printf("\n  [PASSED] Intermediate code generation complete.\n");
    printf("@@PHASE4_END@@\n");

    ast_free(parse_tree_root);
    parse_tree_root = NULL;
}

int main(int argc, char *argv[]) {
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        FILE *fp = fopen(argv[2], "r");
        if (!fp) {
            fprintf(stderr, "Error: cannot open '%s'\n", argv[2]);
            return 1;
        }
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (line[0] == '\0' || line[0] == '#') continue;
            printf("\n@@INPUT@@%s@@END_INPUT@@\n", line);
            compile(line);
        }
        fclose(fp);
        return 0;
    }

    char line[1024];
    if (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = '\0';
        printf("\n@@INPUT@@%s@@END_INPUT@@\n", line);
        compile(line);
    }
    return 0;
}
