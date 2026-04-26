/*
 * semantic.c  —  StudyLang Semantic Analyser
 *
 * Walks the AST and checks that instructions make sense.
 * Errors are collected in sem_error_list[], NOT printed here.
 * main.c prints them per phase.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "ast.h"

/* ---- Global error lists (defined here, declared extern in common.h) */
PhaseError lex_error_list[MAX_LEX_ERRORS];
int        lex_error_count = 0;

PhaseError syn_error_list[MAX_SYN_ERRORS];
int        syn_error_count = 0;

PhaseError sem_error_list[MAX_SEM_ERRORS];
int        sem_error_count = 0;

void lex_error_add(int line, const char *word) {
    if (lex_error_count >= MAX_LEX_ERRORS) return;
    lex_error_list[lex_error_count].line = line;
    snprintf(lex_error_list[lex_error_count].message,
             sizeof(lex_error_list[0].message),
             "Unknown token %s - not in the StudyLang vocabulary. Check spelling or see the Grammar Guide.", word);
    lex_error_count++;
}
void syn_error_add(int line, const char *msg) {
    if (syn_error_count >= MAX_SYN_ERRORS) return;
    syn_error_list[syn_error_count].line = line;
    snprintf(syn_error_list[syn_error_count].message, sizeof(syn_error_list[0].message), "%s", msg);
    syn_error_count++;
}
void sem_error_add(const char *msg) {
    if (sem_error_count >= MAX_SEM_ERRORS) return;
    sem_error_list[sem_error_count].line = 0;
    snprintf(sem_error_list[sem_error_count].message, sizeof(sem_error_list[0].message), "%s", msg);
    sem_error_count++;
}
/* ---- Symbol table ------------------------------------------- */
#define MAX_SYMBOLS 256

typedef enum {
    SYM_SUBJECT,
    SYM_EVENT,
    SYM_DURATION,
    SYM_ACTION,
    SYM_PRIORITY
} SymKind;

typedef struct {
    char    name[64];
    SymKind kind;
    int     refs;
} Symbol;

static Symbol symtab[MAX_SYMBOLS];
static int    sym_count = 0;

static const char *sym_kind_name(SymKind k) {
    switch (k) {
        case SYM_SUBJECT:  return "SUBJECT";
        case SYM_EVENT:    return "EVENT";
        case SYM_DURATION: return "DURATION";
        case SYM_ACTION:   return "ACTION";
        case SYM_PRIORITY: return "PRIORITY";
        default:           return "UNKNOWN";
    }
}

static void sym_insert(const char *name, SymKind kind) {
    if (!name || name[0] == '\0') return;
    for (int i = 0; i < sym_count; i++) {
        if (symtab[i].kind == kind && strcmp(symtab[i].name, name) == 0) {
            symtab[i].refs++;
            return;
        }
    }
    if (sym_count >= MAX_SYMBOLS) return;
    strncpy(symtab[sym_count].name, name, 63);
    symtab[sym_count].kind = kind;
    symtab[sym_count].refs = 1;
    sym_count++;
}

void print_symbol_table_to(FILE *fp) {
    fprintf(fp, "\n  Symbol Table:\n");
    fprintf(fp, "  +------------------------------+--------------+-------+\n");
    fprintf(fp, "  | %-28s | %-12s | %-5s |\n", "Name", "Kind", "Refs");
    fprintf(fp, "  +------------------------------+--------------+-------+\n");
    for (int i = 0; i < sym_count; i++) {
        fprintf(fp, "  | %-28s | %-12s | %-5d |\n",
               symtab[i].name,
               sym_kind_name(symtab[i].kind),
               symtab[i].refs);
    }
    fprintf(fp, "  +------------------------------+--------------+-------+\n");
}

/* ---- Analysis context (reset per compile call) ------------- */
typedef struct {
    int  errors;
    int  warnings;
    char seen_pairs[64][128];
    int  seen_pair_count;
    int  total_minutes;
    char event_name[32][64];
    char event_prep[32][8];
    int  event_count;

    /* Collected warning strings for reporting */
    char warnings_text[32][256];
    int  warning_count;
} SemCtx;

static void sem_warn(SemCtx *ctx, const char *msg) {
    if (ctx->warning_count < 32)
        strncpy(ctx->warnings_text[ctx->warning_count++], msg, 255);
    ctx->warnings++;
}

static int pair_seen(SemCtx *ctx, const char *action, const char *subject) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%s:%s", action, subject);
    for (int i = 0; i < ctx->seen_pair_count; i++)
        if (strcmp(ctx->seen_pairs[i], buf) == 0) return 1;
    return 0;
}

static void pair_mark(SemCtx *ctx, const char *action, const char *subject) {
    if (ctx->seen_pair_count >= 64) return;
    char buf[128];
    snprintf(buf, sizeof(buf), "%s:%s", action, subject);
    strncpy(ctx->seen_pairs[ctx->seen_pair_count++], buf, 127);
}

static void check_time_conflict(SemCtx *ctx, const char *prep, const char *ref) {
    for (int i = 0; i < ctx->event_count; i++) {
        if (strcmp(ctx->event_name[i], ref) == 0) {
            if (strcmp(ctx->event_prep[i], prep) != 0) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                    "Conflicting time references for '%s' ('%s %s' vs '%s %s').",
                    ref, ctx->event_prep[i], ref, prep, ref);
                sem_warn(ctx, msg);
            }
            return;
        }
    }
    if (ctx->event_count < 32) {
        strncpy(ctx->event_name[ctx->event_count], ref,  63);
        strncpy(ctx->event_prep[ctx->event_count], prep, 7);
        ctx->event_count++;
    }
}

static void check_statement(ASTNode *stmt, SemCtx *ctx) {
    ASTNode *action   = NULL;
    ASTNode *subject  = NULL;
    ASTNode *timed    = NULL;
    ASTNode *timeref  = NULL;
    ASTNode *priority = NULL;

    for (int i = 0; i < stmt->child_count; i++) {
        ASTNode *c = stmt->children[i];
        if (!c) continue;
        switch (c->type) {
            case NT_ACTION:      action   = c; break;
            case NT_SUBJECT:     subject  = c; break;
            case NT_TIMED_BLOCK: timed    = c; break;
            case NT_TIME_REF:    timeref  = c; break;
            case NT_PRIORITY:    priority = c; break;
            default: break;
        }
    }

    /* Rule 1: need an action */
    if (!action) {
        sem_error_add("Statement has no action verb. Every instruction must start with a verb like 'study', 'revise', 'practice', etc.");
        ctx->errors++;
        return;
    }

    /* Rule 2: need a subject */
    if (!subject) {
        char msg[256];
        snprintf(msg, sizeof(msg),
            "Action '%s' has no subject. Specify what to study, e.g. 'study compiler'.",
            action->val1);
        sem_error_add(msg);
        ctx->errors++;
        return;
    }

    sym_insert(action->val1,  SYM_ACTION);
    sym_insert(subject->val1, SYM_SUBJECT);

    /* Rule 7: duplicate */
    if (pair_seen(ctx, action->val1, subject->val1)) {
        char msg[256];
        snprintf(msg, sizeof(msg),
            "'%s %s' already appears in this plan — duplicate task detected.",
            action->val1, subject->val1);
        sem_warn(ctx, msg);
    } else {
        pair_mark(ctx, action->val1, subject->val1);
    }

    /* Rule 3/4/5/6: duration */
    if (timed) {
        int dur = atoi(timed->val1);
        const char *unit = timed->val2;
        char dur_label[64];
        snprintf(dur_label, sizeof(dur_label), "%d_%s", dur, unit);
        sym_insert(dur_label, SYM_DURATION);

        if (dur <= 0) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                "Duration must be a positive number (got %d).", dur);
            sem_error_add(msg);
            ctx->errors++;
            return;
        }

        int dur_mins;
        if (strcmp(unit, "hours") == 0 || strcmp(unit, "hour") == 0 ||
            strcmp(unit, "hrs")   == 0 || strcmp(unit, "hr")   == 0 ||
            strcmp(unit, "h")     == 0) {
            dur_mins = dur * 60;
        } else {
            dur_mins = dur;
        }

        if (dur_mins < 5) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                "%d %s is very short (less than 5 minutes). Consider a longer session.", dur, unit);
            sem_warn(ctx, msg);
        }
        if (dur_mins > 480) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                "%d %s is a very long single session (over 8 hours). Consider breaking it up.", dur, unit);
            sem_warn(ctx, msg);
        }
        ctx->total_minutes += dur_mins;
        if (ctx->total_minutes > 960) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                "Total planned time exceeds 16 hours (%d min) — not sustainable!", ctx->total_minutes);
            sem_warn(ctx, msg);
            ctx->total_minutes = 0;
        }
    }

    /* Rule 8: time conflict */
    if (timeref) {
        const char *prep = timeref->val1;
        for (int i = 0; i < timeref->child_count; i++) {
            ASTNode *c = timeref->children[i];
            if (!c) continue;
            if (c->type == NT_EVENT || c->type == NT_TIMEOF) {
                sym_insert(c->val1, SYM_EVENT);
                check_time_conflict(ctx, prep, c->val1);
            }
        }
    }

    if (priority)
        sym_insert(priority->val1, SYM_PRIORITY);
}

static void check_program(ASTNode *node, SemCtx *ctx) {
    for (int i = 0; i < node->child_count; i++) {
        ASTNode *child = node->children[i];
        if (!child) continue;
        if (child->type == NT_STMT_LIST) {
            for (int j = 0; j < child->child_count; j++) {
                ASTNode *c2 = child->children[j];
                if (c2 && c2->type == NT_STATEMENT)
                    check_statement(c2, ctx);
            }
        }
    }
}

/* Stored ctx for phase 3 output */
static SemCtx last_ctx;
static int    last_sym_count_snap = 0;

int semanticAnalyzer(ASTNode *root) {
    sym_count = 0;
    sem_error_count = 0;
    memset(&last_ctx, 0, sizeof(last_ctx));

    if (!root) {
        sem_error_add("No AST available — parse failed before semantic analysis.");
        return 0;
    }

    check_program(root, &last_ctx);

    /* Copy warnings into sem_error_list as separate entries tagged [W] */
    for (int i = 0; i < last_ctx.warning_count; i++) {
        if (sem_error_count < MAX_SEM_ERRORS) {
            sem_error_list[sem_error_count].line = -1; /* -1 = warning */
            snprintf(sem_error_list[sem_error_count].message,
                     sizeof(sem_error_list[0].message),
                     "[WARNING] %s", last_ctx.warnings_text[i]);
            sem_error_count++;
        }
    }

    last_sym_count_snap = sym_count;
    return (last_ctx.errors == 0);
}

/* Called by main.c to print phase 3 output */
void print_phase3(FILE *fp) {
    fprintf(fp, "\n=== PHASE 3: SEMANTIC ANALYSIS ===\n");

    print_symbol_table_to(fp);

    fprintf(fp, "\n  Results:\n");
    fprintf(fp, "    Errors   : %d\n", last_ctx.errors);
    fprintf(fp, "    Warnings : %d\n", last_ctx.warnings);

    if (last_ctx.total_minutes > 0) {
        int h = last_ctx.total_minutes / 60;
        int m = last_ctx.total_minutes % 60;
        if (h > 0)
            fprintf(fp, "    Total planned study time : %d hr %d min\n", h, m);
        else
            fprintf(fp, "    Total planned study time : %d min\n", m);
    }

    if (last_ctx.errors == 0 && last_ctx.warnings == 0)
        fprintf(fp, "\n  Result: SEMANTIC VALID\n");
    else if (last_ctx.errors == 0)
        fprintf(fp, "\n  Result: SEMANTIC VALID (with %d warning(s))\n", last_ctx.warnings);
    else
        fprintf(fp, "\n  Result: SEMANTIC INVALID (%d error(s))\n", last_ctx.errors);
}
