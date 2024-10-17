#ifndef LAB3_TRANSLATE_H
#define LAB3_TRANSLATE_H

#include "tree.h"
#include "semantic.h"

#define MAX_IR_NUM 1024
#define MAX_IR_LEN 128
#define MAX_VAR_LEN 16
#define MAX_TEMP_LEN 16

typedef char *Ir;

typedef struct _irTable
{
	Ir irs[MAX_IR_NUM];
	int num;
} IrTable;

void translate(TreeNode *root, SymbolTable *st, IrTable *irt);
void translateExtDef(TreeNode *extDef, SymbolTable *st, IrTable *irt);
void translateVarDec(TreeNode *varDec, SymbolTable *st, IrTable *irt);
void translateFunDec(TreeNode *funDec, SymbolTable *st, IrTable *irt);
void translateCompSt(TreeNode *compSt, SymbolTable *st, IrTable *irt);
void translateDef(TreeNode *def, SymbolTable *st, IrTable *irt);
void translateExp(TreeNode *exp, SymbolTable *st, IrTable *irt, char *place);
void translateStmt(TreeNode *stmt, SymbolTable *st, IrTable *irt);
void translateCond(TreeNode *exp, char *labelTrue, char *labelFalse, SymbolTable *st, IrTable *irt);
void translateIf(TreeNode *stmt, SymbolTable *st, IrTable *irt);
void translateWhile(TreeNode *stmt, SymbolTable *st, IrTable *irt);

char *newVar();
char *newTemp();
char *newLabel();
char *genConstVal(int val);  // 假设 1：不会出现浮点常量
char *genDec(int size, IrTable *irt);
void genFunction(char *name, IrTable *irt);
char *genParam(IrTable *irt);
void genAssign(char *dst, char *src, IrTable *irt);
void genArg(char *name, IrTable *irt);
void genRead(char *place, IrTable *irt);
void genWrite(char *arg, IrTable *irt);
void genIfGoto(char *x, char *y, char *relop, char *label, IrTable *irt);
void genGoto(char *label, IrTable *irt);
void genLabel(char *label, IrTable *irt);
void genReturn(char *place, IrTable *irt);

void saveIrs(IrTable *irt, char *filename);

#endif //LAB3_TRANSLATE_H
