%{
    #include "syntax.tab.h"

    extern int lexicalError;
    int errorLine = -1;

    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
%}

%option yylineno
%option noyywrap

delim       [ \t]
WS          {delim}+
digit       [0-9]
OCT         0[1-7][0-7]*
DEC         0|[1-9]{digit}*
HEX         0[xX][0-9a-fA-F]+
base        ({digit}*\.{digit}+|{digit}+\.{digit}*)
expo        [+-]?{digit}+
FLOAT       {DEC}\.{digit}+|{base}[eE]{expo}
ID          [_a-zA-Z][_a-zA-Z0-9]*
SEMI        ;
COMMA       ,
ASSIGNOP    =
RELOP       <|>|<=|>=|==|!=
PLUS        \+
MINUS       \-
STAR        \*
DIV         \/
AND         &&
OR          \|\|
DOT         \.
NOT         \!
TYPE        int|float
LP          \(
RP          \)
LB          \[
RB          \]
LC          \{
RC          \}
STRUCT      struct
RETURN      return
IF          if
ELSE        else
WHILE       while

%%

\n|\r       { yycolumn = 1; }
"//"        { char c = input(); 
              while (c != '\n') c = input(); }
"/*"        { char c1 = input(); char c2 = input(); 
              while (c2 != 0 && !(c1 == '*' && c2 == '/')) { c1 = c2; c2 = input(); } }
{WS}        { /* skip white space */ }
{OCT}       { yylval.node = newLeafNode("INT", yylineno, TOKEN_OCT, yytext);     return INT; }
{DEC}       { yylval.node = newLeafNode("INT", yylineno, TOKEN_DEC, yytext);     return INT; }
{HEX}       { yylval.node = newLeafNode("INT", yylineno, TOKEN_HEX, yytext);     return INT; }
{FLOAT}     { yylval.node = newLeafNode("FLOAT", yylineno, TOKEN_FLOAT, yytext); return FLOAT; }
{SEMI}      { yylval.node = newLeafNode("SEMI", yylineno, TOKEN_SEP, yytext);    return SEMI; }
{COMMA}     { yylval.node = newLeafNode("COMMA", yylineno, TOKEN_SEP, yytext);   return COMMA; }
{ASSIGNOP}  { yylval.node = newLeafNode("ASSIGNOP", yylineno, TOKEN_OP, yytext); return ASSIGNOP; }
{RELOP}     { yylval.node = newLeafNode("RELOP", yylineno, TOKEN_OP, yytext);    return RELOP; }
{PLUS}      { yylval.node = newLeafNode("PLUS", yylineno, TOKEN_OP, yytext);     return PLUS; }
{MINUS}     { yylval.node = newLeafNode("MINUS", yylineno, TOKEN_OP, yytext);    return MINUS; }
{STAR}      { yylval.node = newLeafNode("STAR", yylineno, TOKEN_OP, yytext);     return STAR; }
{DIV}       { yylval.node = newLeafNode("DIV", yylineno, TOKEN_OP, yytext);      return DIV; }
{AND}       { yylval.node = newLeafNode("AND", yylineno, TOKEN_OP, yytext);      return AND; }
{OR}        { yylval.node = newLeafNode("OR", yylineno, TOKEN_OP, yytext);       return OR; }
{DOT}       { yylval.node = newLeafNode("DOT", yylineno, TOKEN_OP, yytext);      return DOT; }
{NOT}       { yylval.node = newLeafNode("NOT", yylineno, TOKEN_OP, yytext);      return NOT; }
{TYPE}      { yylval.node = newLeafNode("TYPE", yylineno, TOKEN_TYPE, yytext);   return TYPE; }
{LP}        { yylval.node = newLeafNode("LP", yylineno, TOKEN_PARENS, yytext);   return LP; }
{RP}        { yylval.node = newLeafNode("RP", yylineno, TOKEN_PARENS, yytext);   return RP; }
{LB}        { yylval.node = newLeafNode("LB", yylineno, TOKEN_PARENS, yytext);   return LB; }
{RB}        { yylval.node = newLeafNode("RB", yylineno, TOKEN_PARENS, yytext);   return RB; }
{LC}        { yylval.node = newLeafNode("LC", yylineno, TOKEN_PARENS, yytext);   return LC; }
{RC}        { yylval.node = newLeafNode("RC", yylineno, TOKEN_PARENS, yytext);   return RC; }
{STRUCT}    { yylval.node = newLeafNode("STRUCT", yylineno, TOKEN_KEY, yytext);  return STRUCT; }
{RETURN}    { yylval.node = newLeafNode("RETURN", yylineno, TOKEN_KEY, yytext);  return RETURN; }
{IF}        { yylval.node = newLeafNode("IF", yylineno, TOKEN_KEY, yytext);      return IF; }
{ELSE}      { yylval.node = newLeafNode("ELSE", yylineno, TOKEN_KEY, yytext);    return ELSE; }
{WHILE}     { yylval.node = newLeafNode("WHILE", yylineno, TOKEN_KEY, yytext);   return WHILE; }
{ID}        { yylval.node = newLeafNode("ID", yylineno, TOKEN_ID, yytext);       return ID; }
.           { lexicalError = 1; errorLine = yylineno; printf("Error type A at line %d: Mysterious character \'%s\'\n", yylineno, yytext); }

%%
