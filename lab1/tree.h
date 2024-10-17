#ifndef TREE_H
#define TREE_H

#include "nodeType.h"

typedef struct _tree_node {
    char *name;
    int lineno;  // line number
    NodeType type;
    int cnum;
	struct _tree_node *firstChild;
    struct _tree_node *nextSibling;
    union {
        int ival;
        float fval;
        char *sval;
    };
} TreeNode;

TreeNode *newInterNode(char *name, int lineno, int argc, ...);
TreeNode *newLeafNode(char *name, int lineno, NodeType type, char *text);
void printTree(TreeNode *root, int depth);

#endif