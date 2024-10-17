#include <stdio.h>
#include "tree.h"
#include "syntax.tab.h"

extern TreeNode *root;
extern void yyerror(char *s);
extern void yyrestart(FILE *f);
extern int yyparse();

int lexicalError = 0;
int syntaxError = 0;

int main(int argc, char **argv)
{
    if (argc <= 1) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if (!lexicalError && !syntaxError) {
        printTree(root, 0);
    }
        
    return 0;
}