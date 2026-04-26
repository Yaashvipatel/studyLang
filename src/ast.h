/*
 * ast.h  —  Abstract Syntax Tree node definition and helper functions.
 */

#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum {
    NT_PROGRAM,
    NT_STMT_LIST,
    NT_STATEMENT,
    NT_MODIFIER,
    NT_ACTION,
    NT_SUBJECT,
    NT_TIMED_BLOCK,
    NT_TIME_REF,
    NT_EVENT,
    NT_TIMEOF,
    NT_PRIORITY,
    NT_CONNECTOR,
    NT_ERROR
} NodeType;

#define MAX_CHILDREN 16

typedef struct ASTNode {
    NodeType        type;
    char            label[64];
    char            val1[64];
    char            val2[64];
    struct ASTNode *children[MAX_CHILDREN];
    int             child_count;
} ASTNode;

static inline ASTNode *make_node(NodeType type, const char *label,
                                  const char *val1, const char *val2) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!n) { fprintf(stderr, "AST: out of memory\n"); exit(1); }
    n->type = type;
    if (label) strncpy(n->label, label, 63);
    if (val1)  strncpy(n->val1,  val1,  63);
    if (val2)  strncpy(n->val2,  val2,  63);
    return n;
}

static inline void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    if (parent->child_count < MAX_CHILDREN)
        parent->children[parent->child_count++] = child;
}

/* Print tree with box-drawing characters */
static inline void ast_print(ASTNode *node, int depth, int last[]) {
    if (!node) return;
    for (int i = 0; i < depth - 1; i++)
        printf("%s", last[i] ? "    " : "|   ");
    if (depth > 0)
        printf("%s", last[depth - 1] ? "L-- " : "|-- ");

    if (node->val1[0] && node->child_count == 0) {
        printf("[%s: \"%s\"%s%s]\n",
               node->label, node->val1,
               node->val2[0] ? " / " : "",
               node->val2[0] ? node->val2 : "");
    } else if (node->val1[0]) {
        printf("<%s: %s>\n", node->label, node->val1);
    } else {
        printf("<%s>\n", node->label);
    }

    for (int i = 0; i < node->child_count; i++) {
        last[depth] = (i == node->child_count - 1);
        ast_print(node->children[i], depth + 1, last);
    }
}

static inline void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++)
        ast_free(node->children[i]);
    free(node);
}

#endif /* AST_H */
