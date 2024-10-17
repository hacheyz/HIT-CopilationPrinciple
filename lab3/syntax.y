%{
    #include <stdio.h>
    #include "tree.h"
    #include "lex.yy.c"
    
    extern int syntaxError;
    extern int errorLine;
    TreeNode *root = NULL;
    void yyerror(char *msg);
%}

%union {
    TreeNode *node;
}

// Define the tokens
%token <node> INT FLOAT ID
%token <node> SEMI COMMA
%token <node> ASSIGNOP RELOP
%token <node> PLUS MINUS STAR DIV
%token <node> AND OR DOT NOT TYPE
%token <node> LP RP LB RB LC RC
%token <node> STRUCT RETURN IF ELSE WHILE

// Define the non-terminals
%type <node> Program ExtDefList ExtDef ExtDecList  // High-level Definitions
%type <node> Specifier StructSpecifier OptTag Tag  // Specifiers
%type <node> VarDec FunDec VarList ParamDec        // Declarators
%type <node> CompSt StmtList Stmt                  // Statements
%type <node> DefList Def DecList Dec               // Local Definitions
%type <node> Exp Args                              // Expressions

// Define the precedence and associativity
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

/* High-level Definitions */
Program : ExtDefList                            { $$ = newInnerNode("Program", @$.first_line, 1, $1); root = $$; }
    ;
ExtDefList : ExtDef ExtDefList                  { $$ = newInnerNode("ExtDefList", @$.first_line, 2, $1, $2); }
    |                                           { $$ = NULL; }
    ;
ExtDef : Specifier ExtDecList SEMI              { $$ = newInnerNode("ExtDef", @$.first_line, 3, $1, $2, $3); }
    | Specifier SEMI                            { $$ = newInnerNode("ExtDef", @$.first_line, 2, $1, $2); }
    | Specifier error SEMI                      { syntaxError = 1; }
    | Specifier FunDec CompSt                   { $$ = newInnerNode("ExtDef", @$.first_line, 3, $1, $2, $3); }
    ;
ExtDecList : VarDec                             { $$ = newInnerNode("ExtDecList", @$.first_line, 1, $1); }
    | VarDec COMMA ExtDecList                   { $$ = newInnerNode("ExtDecList", @$.first_line, 3, $1, $2, $3); }
    ;

/* Specifiers */
Specifier : TYPE                                { $$ = newInnerNode("Specifier", @$.first_line, 1, $1); }
    | StructSpecifier                           { $$ = newInnerNode("Specifier", @$.first_line, 1, $1); }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = newInnerNode("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5); }
    | STRUCT Tag                                { $$ = newInnerNode("StructSpecifier", @$.first_line, 2, $1, $2); }
    ;
OptTag : ID                                     { $$ = newInnerNode("OptTag", @$.first_line, 1, $1); }
    |                                           { $$ = NULL; }
    ;
Tag : ID                                        { $$ = newInnerNode("Tag", @$.first_line, 1, $1); }
    ;

/* Declarators */
VarDec : ID                                     { $$ = newInnerNode("VarDec", @$.first_line, 1, $1); }
    | VarDec LB INT RB                          { $$ = newInnerNode("VarDec", @$.first_line, 4, $1, $2, $3, $4); }
    | VarDec LB error RB                        { syntaxError = 1; }
    | error RB                                  { syntaxError = 1; }
    ;
FunDec : ID LP VarList RP                       { $$ = newInnerNode("FunDec", @$.first_line, 4, $1, $2, $3, $4); }
    | ID LP RP                                  { $$ = newInnerNode("FunDec", @$.first_line, 3, $1, $2, $3); }
    | ID LP error RP                            { syntaxError = 1; }
    | error LP VarList RP                       { syntaxError = 1; }
    ;
VarList : ParamDec COMMA VarList                { $$ = newInnerNode("VarList", @$.first_line, 3, $1, $2, $3); }
    | ParamDec                                  { $$ = newInnerNode("VarList", @$.first_line, 1, $1); }
    ;
ParamDec : Specifier VarDec                     { $$ = newInnerNode("ParamDec", @$.first_line, 2, $1, $2); }
    ;

/* Statements */
CompSt : LC DefList StmtList RC                 { $$ = newInnerNode("CompSt", @$.first_line, 4, $1, $2, $3, $4); }
    | error RC                                  { syntaxError = 1; }
    ;
StmtList : Stmt StmtList                        { $$ = newInnerNode("StmtList", @$.first_line, 2, $1, $2); }
    |                                           { $$ = NULL; }
    ;
Stmt : Exp SEMI                                 { $$ = newInnerNode("Stmt", @$.first_line, 2, $1, $2); }
    | CompSt                                    { $$ = newInnerNode("Stmt", @$.first_line, 1, $1); }
    | RETURN Exp SEMI                           { $$ = newInnerNode("Stmt", @$.first_line, 3, $1, $2, $3); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$ = newInnerNode("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt               { $$ = newInnerNode("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt                      { $$ = newInnerNode("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
    | WHILE LP error RP Stmt                    { syntaxError = 1; }
    | error SEMI                                { syntaxError = 1; }
    | RETURN Exp error                          { syntaxError = 1; }
    | RETURN error SEMI                         { syntaxError = 1; }
    ;

/* Local Definitions */
DefList : Def DefList                           { $$ = newInnerNode("DefList", @$.first_line, 2, $1, $2); }
    |                                           { $$ = NULL; }
    ;
Def : Specifier DecList SEMI                    { $$ = newInnerNode("Def", @$.first_line, 3, $1, $2, $3); }
    | Specifier error SEMI                      { syntaxError = 1; }
    ;
DecList : Dec                                   { $$ = newInnerNode("DecList", @$.first_line, 1, $1); }
    | Dec COMMA DecList                         { $$ = newInnerNode("DecList", @$.first_line, 3, $1, $2, $3); }
    ;
Dec : VarDec                                    { $$ = newInnerNode("Dec", @$.first_line, 1, $1); }
    | VarDec ASSIGNOP Exp                       { $$ = newInnerNode("Dec", @$.first_line, 3, $1, $2, $3); }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp                          { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp AND Exp                               { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp OR Exp                                { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp RELOP Exp                             { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp PLUS Exp                              { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp MINUS Exp                             { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp STAR Exp                              { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp DIV Exp                               { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | LP Exp RP                                 { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | MINUS Exp                                 { $$ = newInnerNode("Exp", @$.first_line, 2, $1, $2); }
    | NOT Exp                                   { $$ = newInnerNode("Exp", @$.first_line, 2, $1, $2); }
    | ID LP Args RP                             { $$ = newInnerNode("Exp", @$.first_line, 4, $1, $2, $3, $4); }
    | ID LP RP                                  { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp LB Exp RB                             { $$ = newInnerNode("Exp", @$.first_line, 4, $1, $2, $3, $4); }
    | Exp DOT ID                                { $$ = newInnerNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | ID                                        { $$ = newInnerNode("Exp", @$.first_line, 1, $1); }
    | INT                                       { $$ = newInnerNode("Exp", @$.first_line, 1, $1); }
    | FLOAT                                     { $$ = newInnerNode("Exp", @$.first_line, 1, $1); }
    | Exp LB error RB                           { syntaxError = 1; }
    | Exp MINUS error                           { syntaxError = 1; }
    ;
Args : Exp COMMA Args                           { $$ = newInnerNode("Args", @$.first_line, 3, $1, $2, $3); }
    | Exp                                       { $$ = newInnerNode("Args", @$.first_line, 1, $1); }
    ;

%%

void yyerror(char *msg)
{
    static int prev_line = -1;
    if (yylineno != prev_line && yylineno != errorLine)
    {
        syntaxError = 1;
        fprintf(stderr, "Error type B at line %d: %s.\n", yylineno, msg);
        prev_line = yylineno;
    }
}