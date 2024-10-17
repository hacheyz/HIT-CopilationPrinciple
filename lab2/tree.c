#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "tree.h"
#include "utils.h"

TreeNode *newInnerNode(char *name, int lineno, int argc, ...)
{
	TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
	if (node == NULL) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	node->name = (char *)malloc(strlen(name) + 1);
	strcpy(node->name, name);
	node->lineno = lineno;
	node->type = INTERNAL;
	node->cnum = argc;

	va_list ap;
	va_start(ap, argc);
	TreeNode *p = va_arg(ap, TreeNode *);
	node->firstChild = p;

	for (int i = 1; i < argc; i++) {
		p->nextSibling = va_arg(ap, TreeNode *);
		if (p->nextSibling != NULL) {
			p = p->nextSibling;
		}
	}
	va_end(ap);

	return node;
}

TreeNode *newLeafNode(char *name, int lineno, NodeType type, char *text)
{
	TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
	if (node == NULL) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	node->name = (char *)malloc(strlen(name) + 1);
	strcpy(node->name, name);
	node->lineno = lineno;
	node->type = type;
	node->cnum = 0;
	node->firstChild = NULL;
	node->nextSibling = NULL;
	switch (type) {
	case TOKEN_DEC:
		node->ival = dec_atoi(text);
		break;
	case TOKEN_OCT:
		node->ival = oct_atoi(text);
		break;
	case TOKEN_HEX:
		node->ival = hex_atoi(text);
		break;
	case TOKEN_FLOAT:
		node->fval = fl_atof(text);
		break;
	case TOKEN_ID:
	case TOKEN_TYPE:
	case TOKEN_KEY:
	case TOKEN_SEP:
	case TOKEN_OP:
	case TOKEN_PARENS:
		node->sval = (char *)malloc(strlen(text) + 1);
		strcpy(node->sval, text);
		break;
	default:
		fprintf(stderr, "Unknown token type.\n");
		exit(1);
	}
	return node;
}

void printTree(TreeNode *root, int depth)
{
	if (root == NULL) {
		return;
	}

	for (int i = 0; i < depth; i++) {
		printf("  ");
	}
	printf("%s", root->name);
	switch (root->type) {
	case INTERNAL:
		printf(" (%d)\n", root->lineno);
		break;
	case TOKEN_DEC:
	case TOKEN_OCT:
	case TOKEN_HEX:
		printf(": %d\n", root->ival);
		break;
	case TOKEN_FLOAT:
		printf(": %f\n", root->fval);
		break;
	case TOKEN_ID:
	case TOKEN_TYPE:
		printf(": %s\n", root->sval);
		break;
	default:
		printf("\n");
		break;
	}
	printTree(root->firstChild, depth + 1);
	printTree(root->nextSibling, depth);
}