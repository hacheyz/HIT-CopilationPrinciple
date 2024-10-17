#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdbool.h>
#include "tree.h"

#define HASH_MAX 0x3fff
#define SYMBOL_TABLE_SIZE (HASH_MAX + 1)
#define FIELD_NAME_LEN 32
#define SERROR_MSG_LEN 256
#define MAX_PARAM_NUM 8

struct _type;
struct _fieldList;

typedef struct _type
{
	enum
	{
		BASIC, ARRAY, STRUCTURE, FUNCTION
	} kind;
	union
	{
		enum
		{
			_INT, _FLOAT
		} basic;  // 基本类型信息包括 int 与 float
		struct  // 数组类型信息包括元素类型与数组大小构成
		{
			struct _type *elem;
			int size;
		} array;
		struct _fieldList *structure;  // 结构体类型信息是一个链表
		struct  // 函数类型信息包括返回值类型与参数列表
		{
			struct _type *returnType;
			int paramNum;
			struct _fieldList *paramList;
		} function;
	} u;
} Type;

typedef struct _fieldList
{
	char name[FIELD_NAME_LEN];  // 域的名字
	char *varName;  // 中间代码生成时的变量名
	Type *type;  // 域的类型
	struct _fieldList *tail;  // 下一个域
} FieldList;

typedef struct _symbolTable
{
	FieldList *symbols[SYMBOL_TABLE_SIZE];
} SymbolTable;

void serror(TreeNode *node, int typeId, char *msg);  // 打印错误信息
unsigned int hash_pjw(char *name);  // PJW 哈希函数，将字符串转换为哈希值
void initSymbolTable(SymbolTable *st);  // 初始化符号表
FieldList *findSymbol(SymbolTable *st, char *name);  // 在符号表中查找符号，找到返回符号的指针，否则返回 NULL
void insertSymbol(SymbolTable *st, FieldList *fieldList);  // 在符号表中插入符号
void traverseTree(TreeNode *root, SymbolTable *st);  // 遍历语法树，进行语义分析
void analyseExtDef(TreeNode *extDef, SymbolTable *st);
Type *analyseSpecifier(TreeNode *specifier, SymbolTable *st, bool dec);  // dec 为 true 时表示声明，为 false 时表示定义
Type *analyseStructSpecifier(TreeNode *type, SymbolTable *st, bool dec);  // dec 为 true 时表示声明，为 false 时表示定义
void analyseDef(TreeNode *def, FieldList *structField, SymbolTable *st);
FieldList *analyseVarDec(TreeNode *varDec, Type *baseType, FieldList *structField, SymbolTable *st);
Type *analyseVarDecType(TreeNode *varDec, Type *baseType, SymbolTable *st);
Type *analyseExp(TreeNode *root, SymbolTable *st);
void analyseCompSt(TreeNode *compSt, Type *returnType, SymbolTable *st);
void analyseStmt(TreeNode *stmt, Type *returnType, SymbolTable *st);
int getArgTypes(TreeNode *args, SymbolTable *st, Type *argTypes);
Type *getVarTypes(TreeNode *varList, SymbolTable *st);
bool isSameType(Type *t1, Type *t2);
bool isLeftVal(TreeNode *exp);  // 判断一个结点是否为左值
FieldList *createRead();
FieldList *createWrite();

#endif //SEMANTIC_H
