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
Program : ExtDefList                            { $$ = newInterNode("Program", @$.first_line, 1, $1); root = $$; }
    ;
ExtDefList : ExtDef ExtDefList                  { $$ = newInterNode("ExtDefList", @$.first_line, 2, $1, $2); }
    |                                           { $$ = NULL; }
    ;
ExtDef : Specifier ExtDecList SEMI              { $$ = newInterNode("ExtDef", @$.first_line, 3, $1, $2, $3); }
    | Specifier SEMI                            { $$ = newInterNode("ExtDef", @$.first_line, 2, $1, $2); }
    | Specifier error SEMI                      { syntaxError = 1; }
    | Specifier FunDec CompSt                   { $$ = newInterNode("ExtDef", @$.first_line, 3, $1, $2, $3); }
    ;
ExtDecList : VarDec                             { $$ = newInterNode("ExtDecList", @$.first_line, 1, $1); }
    | VarDec COMMA ExtDecList                   { $$ = newInterNode("ExtDecList", @$.first_line, 3, $1, $2, $3); }
    ;

/* Specifiers */
Specifier : TYPE                                { $$ = newInterNode("Specifier", @$.first_line, 1, $1); }
    | StructSpecifier                           { $$ = newInterNode("Specifier", @$.first_line, 1, $1); }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = newInterNode("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5); }
    | STRUCT Tag                                { $$ = newInterNode("StructSpecifier", @$.first_line, 2, $1, $2); }
    ;
OptTag : ID                                     { $$ = newInterNode("OptTag", @$.first_line, 1, $1); }
    |                                           { $$ = NULL; }
    ;
Tag : ID                                        { $$ = newInterNode("Tag", @$.first_line, 1, $1); }
    ;

/* Declarators */
VarDec : ID                                     { $$ = newInterNode("VarDec", @$.first_line, 1, $1); } 
    | VarDec LB INT RB                          { $$ = newInterNode("VarDec", @$.first_line, 4, $1, $2, $3, $4); }
    | VarDec LB error RB                        { syntaxError = 1; }
    | error RB                                  { syntaxError = 1; }
    ;
FunDec : ID LP VarList RP                       { $$ = newInterNode("FunDec", @$.first_line, 4, $1, $2, $3, $4); }
    | ID LP RP                                  { $$ = newInterNode("FunDec", @$.first_line, 3, $1, $2, $3); }
    | ID LP error RP                            { syntaxError = 1; }
    | error LP VarList RP                       { syntaxError = 1; }
    ;
VarList : ParamDec COMMA VarList                { $$ = newInterNode("VarList", @$.first_line, 3, $1, $2, $3); } 
    | ParamDec                                  { $$ = newInterNode("VarList", @$.first_line, 1, $1); }
    ;
ParamDec : Specifier VarDec                     { $$ = newInterNode("ParamDec", @$.first_line, 2, $1, $2); }
    ;

/* Statements */
CompSt : LC DefList StmtList RC                 { $$ = newInterNode("CompSt", @$.first_line, 4, $1, $2, $3, $4); }
    | error RC                                  { syntaxError = 1; }
    ;
StmtList : Stmt StmtList                        { $$ = newInterNode("StmtList", @$.first_line, 2, $1, $2); } 
    |                                           { $$ = NULL; }
    ;
Stmt : Exp SEMI                                 { $$ = newInterNode("Stmt", @$.first_line, 2, $1, $2); }
    | CompSt                                    { $$ = newInterNode("Stmt", @$.first_line, 1, $1); }
    | RETURN Exp SEMI                           { $$ = newInterNode("Stmt", @$.first_line, 3, $1, $2, $3); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$ = newInterNode("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt               { $$ = newInterNode("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt                      { $$ = newInterNode("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
    | WHILE LP error RP Stmt                    { syntaxError = 1; }
    | error SEMI                                { syntaxError = 1; }
    | RETURN Exp error                          { syntaxError = 1; }
    | RETURN error SEMI                         { syntaxError = 1; }
    ;

/* Local Definitions */
DefList : Def DefList                           { $$ = newInterNode("DefList", @$.first_line, 2, $1, $2); }
    |                                           { $$ = NULL; }
    ;
Def : Specifier DecList SEMI                    { $$ = newInterNode("Def", @$.first_line, 3, $1, $2, $3); }
    | Specifier error SEMI                      { syntaxError = 1; }
    ;
DecList : Dec                                   { $$ = newInterNode("DecList", @$.first_line, 1, $1); }
    | Dec COMMA DecList                         { $$ = newInterNode("DecList", @$.first_line, 3, $1, $2, $3); }
    ;
Dec : VarDec                                    { $$ = newInterNode("Dec", @$.first_line, 1, $1); }
    | VarDec ASSIGNOP Exp                       { $$ = newInterNode("Dec", @$.first_line, 3, $1, $2, $3); }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp                          { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp AND Exp                               { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp OR Exp                                { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp RELOP Exp                             { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp PLUS Exp                              { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp MINUS Exp                             { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp STAR Exp                              { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp DIV Exp                               { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | LP Exp RP                                 { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | MINUS Exp                                 { $$ = newInterNode("Exp", @$.first_line, 2, $1, $2); }
    | NOT Exp                                   { $$ = newInterNode("Exp", @$.first_line, 2, $1, $2); }
    | ID LP Args RP                             { $$ = newInterNode("Exp", @$.first_line, 4, $1, $2, $3, $4); }
    | ID LP RP                                  { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp LB Exp RB                             { $$ = newInterNode("Exp", @$.first_line, 4, $1, $2, $3, $4); }
    | Exp DOT ID                                { $$ = newInterNode("Exp", @$.first_line, 3, $1, $2, $3); }
    | ID                                        { $$ = newInterNode("Exp", @$.first_line, 1, $1); }
    | INT                                       { $$ = newInterNode("Exp", @$.first_line, 1, $1); }
    | FLOAT                                     { $$ = newInterNode("Exp", @$.first_line, 1, $1); }
    | Exp LB error RB                           { syntaxError = 1; }
    | Exp MINUS error                           { syntaxError = 1; }
    ;
Args : Exp COMMA Args                           { $$ = newInterNode("Args", @$.first_line, 3, $1, $2, $3); }
    | Exp                                       { $$ = newInterNode("Args", @$.first_line, 1, $1); }
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