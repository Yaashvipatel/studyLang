#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "common.h"
#include "ast.h"

#define MAX_TAC 512

typedef struct {
    int  label;
    char op    [32];
    char arg1  [64];
    char arg2  [64];
    char result[16];
} TACInstr;

static TACInstr tac[MAX_TAC];
static int      tac_count = 0;
static int      temp_n    = 0; 

static void new_temp(char *buf, size_t len) {
    snprintf(buf, len, "t%d", ++temp_n);
}

static void emit(const char *op,
                 const char *a1,
                 const char *a2,
                 const char *res) {
    if (tac_count >= MAX_TAC) {
        fprintf(stderr, "ICG: instruction buffer full\n");
        return;
    }
    TACInstr *ins = &tac[tac_count++];
    ins->label = tac_count;
    strncpy(ins->op,    op  ? op  : "",  31);
    strncpy(ins->arg1,  a1  ? a1  : "",  63);
    strncpy(ins->arg2,  a2  ? a2  : "",  63);
    strncpy(ins->result,res ? res : "",  15);
}

static void to_upper(const char *src, char *dst, size_t n) {
    size_t i;
    for (i = 0; src[i] && i < n - 1; i++)
        dst[i] = (char)toupper((unsigned char)src[i]);
    dst[i] = '\0';
}

#define MAX_TASKS 64

typedef struct {
    char action  [64];
    char modifier[64];
    char subject [64];
    char duration[32];
    char timeref [64];
    char day     [32];
    char priority[32];
} StudyTask;

static StudyTask tasks[MAX_TASKS];
static int       task_count = 0;

static void print_study_plan(void) {
    printf("  +------+--------------+--------------------+----------+---------------+----------+\n");
    printf("  | Task | Action       | Subject            | Duration | Time Ref      | Priority |\n");
    printf("  +------+--------------+--------------------+----------+---------------+----------+\n");

    for (int i = 0; i < task_count; i++) {
        StudyTask *t = &tasks[i];

     
        char action_full[80] = "";
        if (t->modifier[0])
            snprintf(action_full, sizeof(action_full), "%s %s",
                     t->modifier, t->action);
        else
            strncpy(action_full, t->action, sizeof(action_full) - 1);

       
        char timeref_full[80] = "-";
        if (t->timeref[0] && t->day[0])
            snprintf(timeref_full, sizeof(timeref_full), "%s, %s",
                     t->timeref, t->day);
        else if (t->timeref[0])
            strncpy(timeref_full, t->timeref, sizeof(timeref_full) - 1);
        else if (t->day[0])
            strncpy(timeref_full, t->day, sizeof(timeref_full) - 1);

        printf("  | %-4d | %-12s | %-18s | %-8s | %-13s | %-8s |\n",
               i + 1,
               action_full[0] ? action_full : "-",
               t->subject[0]  ? t->subject  : "-",
               t->duration[0] ? t->duration : "-",
               timeref_full,
               t->priority[0] ? t->priority : "-");
    }

    printf("  +------+--------------+--------------------+----------+---------------+----------+\n");
}


static void gen_statement(ASTNode *stmt) {
    char t1[16] = "", t2[16] = "", t3[16] = "", tmp[16] = "";
    char action_str[128] = "";

    ASTNode *modifier = NULL;
    ASTNode *action   = NULL;
    ASTNode *subject  = NULL;
    ASTNode *timed    = NULL;
    ASTNode *timeref  = NULL;
    ASTNode *dayref   = NULL;
    ASTNode *priority = NULL;

    
    for (int i = 0; i < stmt->child_count; i++) {
        ASTNode *c = stmt->children[i];
        if (!c) continue;
        switch (c->type) {
            case NT_MODIFIER:    modifier = c; break;
            case NT_ACTION:      action   = c; break;
            case NT_SUBJECT:     subject  = c; break;
            case NT_TIMED_BLOCK: timed    = c; break;
            case NT_TIME_REF:
               
                if (strcmp(c->label, "day_ref") == 0)
                    dayref  = c;
                else
                    timeref = c;
                break;
            case NT_PRIORITY:    priority = c; break;
            default: break;
        }
    }

   
    if (task_count < MAX_TASKS) {
        StudyTask *task = &tasks[task_count++];
        memset(task, 0, sizeof(StudyTask));

        if (modifier) strncpy(task->modifier, modifier->val1, 63);
        if (action)   strncpy(task->action,   action->val1,   63);
        if (subject)  strncpy(task->subject,  subject->val1,  63);
        if (priority) strncpy(task->priority, priority->val1, 31);

        if (timed) {
            snprintf(task->duration, sizeof(task->duration),
                     "%s %s", timed->val1, timed->val2);
        }

        if (timeref) {
            const char *ref_val  = "";
            const char *prep_val = timeref->val1;
            for (int i = 0; i < timeref->child_count; i++) {
                ASTNode *c = timeref->children[i];
                if (c && (c->type == NT_EVENT || c->type == NT_TIMEOF))
                    ref_val = c->val1;
            }
            snprintf(task->timeref, sizeof(task->timeref),
                     "%s %s", prep_val, ref_val);
        }

        if (dayref) {
            const char *day_val  = "";
            const char *prep_val = dayref->val1;
            for (int i = 0; i < dayref->child_count; i++) {
                ASTNode *c = dayref->children[i];
                if (c) day_val = c->val1;
            }
            snprintf(task->day, sizeof(task->day),
                     "%s %s", prep_val, day_val);
        }
    }


    if (modifier) {
        char up[64];
        to_upper(modifier->val1, up, sizeof(up));
        snprintf(action_str, sizeof(action_str), "%s ", up);
    }
    if (action) {
        char up[64];
        to_upper(action->val1, up, sizeof(up));
        strncat(action_str, up,
                sizeof(action_str) - strlen(action_str) - 1);
        new_temp(t1, sizeof(t1));
        emit("LOAD_ACTION", action_str, "", t1);
    }


    if (subject) {
        new_temp(t2, sizeof(t2));
        emit("LOAD_SUBJECT", subject->val1, "", t2);
    }


    if (timed) {
        char dur[64];
        snprintf(dur, sizeof(dur), "%s %s", timed->val1, timed->val2);
        new_temp(t3, sizeof(t3));
        emit("LOAD_DUR", dur, "", t3);
        new_temp(tmp, sizeof(tmp));
        emit("SET_DUR", t1, t3, tmp);
        strncpy(t1, tmp, sizeof(t1) - 1);
    }


    if (timeref) {
        const char *ref_val  = "";
        const char *prep_val = timeref->val1;
        for (int i = 0; i < timeref->child_count; i++) {
            ASTNode *c = timeref->children[i];
            if (c && (c->type == NT_EVENT || c->type == NT_TIMEOF))
                ref_val = c->val1;
        }
        char tref[128];
        snprintf(tref, sizeof(tref), "%s %s", prep_val, ref_val);
        new_temp(t3, sizeof(t3));
        emit("LOAD_TREF", tref, "", t3);
        new_temp(tmp, sizeof(tmp));
        emit("SET_TREF", t1, t3, tmp);
        strncpy(t1, tmp, sizeof(t1) - 1);
    }

  
    if (dayref) {
        const char *day_val  = "";
        const char *prep_val = dayref->val1;
        for (int i = 0; i < dayref->child_count; i++) {
            ASTNode *c = dayref->children[i];
            if (c) day_val = c->val1;
        }
        char dref[128];
        snprintf(dref, sizeof(dref), "%s %s", prep_val, day_val);
        new_temp(t3, sizeof(t3));
        emit("LOAD_DAY", dref, "", t3);
        new_temp(tmp, sizeof(tmp));
        emit("SET_DAY", t1, t3, tmp);
        strncpy(t1, tmp, sizeof(t1) - 1);
    }


    if (priority) {
        new_temp(t3, sizeof(t3));
        emit("SET_PRIORITY", priority->val1, "", t3);
        new_temp(tmp, sizeof(tmp));
        emit("BIND_PRIORITY", t1, t3, tmp);
        strncpy(t1, tmp, sizeof(t1) - 1);
    }

  
    new_temp(tmp, sizeof(tmp));
    emit("SCHEDULE", t1, t2, tmp);
}


static void gen_node(ASTNode *node, int *stmt_count) {
    if (!node) return;
    switch (node->type) {
        case NT_PROGRAM:
        case NT_STMT_LIST:
            for (int i = 0; i < node->child_count; i++)
                gen_node(node->children[i], stmt_count);
            break;
        case NT_STATEMENT:
            if (*stmt_count > 0)
                emit("SEQ_POINT", "", "", "");
            gen_statement(node);
            (*stmt_count)++;
            break;
        case NT_CONNECTOR:
            break; 
        default:
            break;
    }
}


static void print_tac(void) {
    printf("\n  %-5s  %-18s  %-22s  %-22s  %-10s\n",
           "L#", "OPERATION", "ARG1", "ARG2", "RESULT");
    printf("  %-5s  %-18s  %-22s  %-22s  %-10s\n",
           "-----", "------------------",
           "----------------------", "----------------------", "----------");

    for (int i = 0; i < tac_count; i++) {
        TACInstr *ins = &tac[i];
        if (strcmp(ins->op, "SEQ_POINT") == 0) {
            printf("  %-5d  %s\n", ins->label,
                   "----------- SEQ_POINT -----------");
        } else {
            printf("  %-5d  %-18s  %-22s  %-22s  %-10s\n",
                   ins->label,
                   ins->op,
                   ins->arg1[0]   ? ins->arg1   : "-",
                   ins->arg2[0]   ? ins->arg2   : "-",
                   ins->result[0] ? ins->result : "-");
        }
    }
}

void generateICG(ASTNode *root) {
    printf("\n=== PHASE 4: INTERMEDIATE CODE GENERATION (3-Address Code) ===\n");

    tac_count  = 0;
    temp_n     = 0;
    task_count = 0;

    if (!root) {
        fprintf(stderr, "  ICG ERROR: no AST available.\n");
        return;
    }

    int stmt_count = 0;
    gen_node(root, &stmt_count);

    print_tac();
    printf("\n  Total TAC instructions : %d\n", tac_count);
    printf("  Statements compiled    : %d\n",  stmt_count);

    printf("\n=== GENERATED STUDY PLAN SUMMARY ===\n");
    print_study_plan();
}
