bison -d syntax.y
flex lexical.l
gcc main.c syntax.tab.c tree.c utils.c semantic.c translate.c -lfl -ly -o main
