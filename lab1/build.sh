bison -d syntax.y
flex lexical.l
gcc main.c syntax.tab.c tree.c utils.c -lfl -ly -o parser
