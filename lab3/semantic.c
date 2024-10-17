#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "semantic.h"

unsigned int hash_pjw(char *name)
{
	unsigned int val = 0, i;
	for (; *name; ++name) {
		val = (val << 2) + *name;
		if (i = val & ~HASH_MAX) {
			val = (val ^ (i >> 12)) & HASH_MAX;
		}
	}
	return val;
}

void initSymbolTable(SymbolTable *st)
{
	for (int i = 0; i <= HASH_MAX; ++i) {
		st->symbols[i] = NULL;
	}
	FieldList *read = createRead();
	FieldList *write = createWrite();
	insertSymbol(st, read);
	insertSymbol(st, write);
}

FieldList *findSymbol(SymbolTable *st, char *name)
{
	unsigned int hash = hash_pjw(name);
	return st->symbols[hash];
}

void insertSymbol(SymbolTable *st, FieldList *fieldList)
{
	unsigned int hash = hash_pjw(fieldList->name);
	st->symbols[hash] = fieldList;
}

void traverseTree(TreeNode *root, SymbolTable *st)
{
	if (root == NULL) {
		return;
	}
	if (strcmp(root->name, "ExtDef") == 0) {
		analyseExtDef(root, st);
	}
	traverseTree(root->firstChild, st);
	traverseTree(root->nextSibling, st);
}

void analyseExtDef(TreeNode *extDef, SymbolTable *st)
{
	/* ExtDef : Specifier ExtDecList SEMI
	 *     	  | Specifier SEMI
	 *     	  | Specifier FunDec CompSt
	 *     	  ;
	 * ExtDecList : VarDec
	 * 			  | VarDec COMMA ExtDecList
	 * 			  ;
	 * FunDec : ID LP VarList RP
	 * 		  | ID LP RP
	 * 		  ;
	 */
	TreeNode *specifier = extDef->firstChild;
	TreeNode *node = specifier->nextSibling;  // ExtDecList | SEMI | FunDec
	Type *specifierType;
	if (strcmp(node->name, "SEMI") == 0) {
		specifierType = analyseSpecifier(specifier, st, 1);
	} else {
		specifierType = analyseSpecifier(specifier, st, 0);
	}
	if (specifierType == NULL) return;
	if (strcmp(node->name, "ExtDecList") == 0) {
		TreeNode *decList = node->firstChild;  // VarDec
		while (decList != NULL) {
			analyseVarDec(decList, specifierType, NULL, st);  // NULL-无需填充结构体字段
			decList = decList->nextSibling;  // COMMA | NULL
			if (decList != NULL) {
				decList = decList->nextSibling;  // ExtDecList
				decList = decList->firstChild;  // VarDec
			}
		}
	} else if (strcmp(node->name, "FunDec") == 0) {
		FieldList *funcField = (FieldList *)malloc(sizeof(FieldList));
		Type *t = (Type *)malloc(sizeof(Type));
		funcField->type = t;
		t->kind = FUNCTION;
		t->u.function.returnType = specifierType;
		TreeNode *funNode = node->firstChild;  // ID
		FieldList *redefined = findSymbol(st, funNode->sval);
		if (redefined != NULL && redefined->type->kind == FUNCTION) {
			// ERROR 4: 函数出现重复定义
			char msg[SERROR_MSG_LEN];
			sprintf(msg, "Redefined function \"%s\".", funNode->sval);
			serror(funNode, 4, msg);
			free(funcField);
			free(t);
			return;
		}
		strcpy(funcField->name, funNode->sval);
		funNode = funNode->nextSibling;  // LP
		funNode = funNode->nextSibling;  // VarList | RP
		if (strcmp(funNode->name, "RP") == 0) {
			t->u.function.paramNum = 0;
		} else {  // VarList
			Type *paramTypes = getVarTypes(funNode, st);
			t->u.function.paramList = paramTypes->u.function.paramList;
			t->u.function.paramNum = paramTypes->u.function.paramNum;
		}
		insertSymbol(st, funcField);
		node = node->nextSibling;  // CompSt
		analyseCompSt(node, funcField->type->u.function.returnType, st);
	}
}

void analyseCompSt(TreeNode *compSt, Type *returnType, SymbolTable *st)
{
	/* CompSt : LC DefList StmtList RC
	 * 	  	  ;
	 */
	TreeNode *node = compSt->firstChild;  // LC
	node = node->nextSibling;  // DefList | StmtList
	if (strcmp(node->name, "DefList") == 0) {  // DefList
		TreeNode *defList = node->firstChild;
		while (defList != NULL) {
			analyseDef(defList, NULL, st);  // NULL-无需填充结构体字段
			defList = defList->nextSibling;  // DefList
			if (defList != NULL) {
				defList = defList->firstChild;  // Def
			}
		}
		node = node->nextSibling;  // StmtList
	}
	TreeNode *stmtList = node->firstChild;  // Stmt | NULL
	while (stmtList != NULL) {
		analyseStmt(stmtList, returnType, st);
		stmtList = stmtList->nextSibling;  // StmtList
		if (stmtList != NULL) {
			stmtList = stmtList->firstChild;  // Stmt
		}
	}
}

void analyseStmt(TreeNode *stmt, Type *returnType, SymbolTable *st)
{
	/* Stmt : Exp SEMI
	 * 		| CompSt
	 * 		| RETURN Exp SEMI
	 * 		| IF LP Exp RP Stmt
	 * 		| IF LP Exp RP Stmt ELSE Stmt
	 * 		| WHILE LP Exp RP Stmt
	 * 		;
	 */
	TreeNode *node = stmt->firstChild;
	if (strcmp(node->name, "Exp") == 0) {
		analyseExp(node, st);
	} else if (strcmp(node->name, "CompSt") == 0) {
		analyseCompSt(node, NULL, st);
	} else if (strcmp(node->name, "RETURN") == 0) {
		node = node->nextSibling;  // Exp
		Type *retType = analyseExp(node, st);
		if (retType != NULL && !isSameType(retType, returnType)) {
			// ERROR 8: return 语句的返回类型与函数定义的返回类型不匹配
			serror(node, 8, "Type mismatched for return.");
		}
	} else if (strcmp(node->name, "IF") == 0) {
		node = node->nextSibling;  // LP
		node = node->nextSibling;  // Exp
		Type *condType = analyseExp(node, st);
		if (condType != NULL && !(condType->kind == BASIC && condType->u.basic == _INT)) {
			// ERROR 7: 只有 INT 类型可以作为条件表达式
			serror(node, 7, "Type mismatched for condition.");
		}
		node = node->nextSibling;  // RP
		node = node->nextSibling;  // Stmt
		analyseStmt(node, returnType, st);
		node = node->nextSibling;  // ELSE | NULL
		if (node != NULL) {
			node = node->nextSibling;  // Stmt
			analyseStmt(node, returnType, st);
		}
	} else if (strcmp(node->name, "WHILE") == 0) {
		node = node->nextSibling;  // LP
		node = node->nextSibling;  // Exp
		Type *condType = analyseExp(node, st);
		if (condType != NULL && !(condType->kind == BASIC && condType->u.basic == _INT)) {
			// ERROR 7: 只有 INT 类型可以作为条件表达式
			serror(node, 7, "Type mismatched for condition.");
		}
		node = node->nextSibling;  // RP
		node = node->nextSibling;  // Stmt
		analyseStmt(node, returnType, st);
	}
}

Type *analyseSpecifier(TreeNode *specifier, SymbolTable *st, bool dec)
{
	/* Specifier : TYPE
	 *           | StructSpecifier
	 *           ;
	 */
	Type *t = (Type *)malloc(sizeof(Type));
	TreeNode *type = specifier->firstChild;
	if (strcmp(type->name, "TYPE") == 0) {
		t->kind = BASIC;
		if (strcmp(type->sval, "int") == 0) {
			t->u.basic = _INT;
		} else if (strcmp(type->sval, "float") == 0) {
			t->u.basic = _FLOAT;
		}
	} else if (strcmp(type->name, "StructSpecifier") == 0) {
		t = analyseStructSpecifier(type, st, dec);
	}
	return t;
}

Type *analyseStructSpecifier(TreeNode *type, SymbolTable *st, bool dec)
{
	/* StructSpecifier : STRUCT OptTag LC DefList RC
	 *                 | STRUCT Tag
	 *                 ;
	 */
	Type *t = (Type *)malloc(sizeof(Type));  // 结构体类型
	t->kind = STRUCTURE;
	FieldList *structureField = (FieldList *)malloc(sizeof(FieldList));  // 结构体名
	t->u.structure = structureField;
	strcpy(structureField->name, "");  // 结构体名默认为空，有 OptTag 时再填充
	TreeNode *node = type->firstChild;  // STRUCT
	node = node->nextSibling;  // OptTag | Tag | LC
	if (strcmp(node->name, "OptTag") == 0) {
		// OptTag : ID
		TreeNode *nameNode = node->firstChild;  // ID
		FieldList *redefined = findSymbol(st, nameNode->sval);
		if (redefined != NULL && (redefined->type->kind != STRUCTURE || redefined->type->u.structure != NULL)) {
			// ERROR 16: 结构体名字和前面定义过的结构体或变量的名字重复
			char msg[SERROR_MSG_LEN];
			sprintf(msg, "Duplicated name \"%s\".", nameNode->sval);
			serror(nameNode, 16, msg);
			free(structureField);
			free(t);
			return NULL;
		}
		strcpy(structureField->name, nameNode->sval);  // 填充结构体名字段
		if (redefined != NULL) {  // 之前有过声明，现在是定义
			free(redefined);  // 释放之前的声明
		}
		structureField->type = t;
		insertSymbol(st, structureField);  // 有了名字就可以插入符号表了
		node = node->nextSibling;  // LC
		node = node->nextSibling;  // DefList
		TreeNode *defList = node->firstChild;
		while (defList != NULL) {
			analyseDef(defList, structureField, st);
			defList = defList->nextSibling;  // DefList
			if (defList != NULL) {
				defList = defList->firstChild;  // Def
			}
		}
	} else if (strcmp(node->name, "Tag") == 0) {
		// Tag : ID
		TreeNode *idNode = node->firstChild;
		if (idNode->type == TOKEN_ID) {
			if (dec) {  // 声明
				FieldList *declared = findSymbol(st, idNode->sval);
				if (declared != NULL) {  // ERROR 16: 结构体名字和前面定义过的结构体或变量的名字重复
					char msg[SERROR_MSG_LEN];
					sprintf(msg, "Duplicated name \"%s\".", idNode->sval);
					serror(idNode, 16, msg);
					free(structureField);
					free(t);
					return NULL;
				} else {
					FieldList *decField = (FieldList *)malloc(sizeof(FieldList));
					strcpy(decField->name, idNode->sval);
					t->u.structure = NULL;  // 用 NULL 表示这是一个结构体声明而非定义
					decField->type = t;
					insertSymbol(st, decField);
					free(structureField);
					return t;
				}
			} else {  // 定义
				FieldList *defined = findSymbol(st, idNode->sval);
				if (defined != NULL && defined->type->kind == STRUCTURE) {
					t->u.structure = defined;  // 填充结构体类型字段
					free(structureField);
					return t;
				} else {  // ERROR 17: 直接使用未定义过的结构体来定义变量
					char msg[SERROR_MSG_LEN];
					sprintf(msg, "Undefined structure \"%s\".", idNode->sval);
					serror(idNode, 17, msg);
					free(structureField);
					free(t);
					return NULL;
				}
			}
		}
	} else if (strcmp(node->name, "LC") == 0) {
		node = node->nextSibling;  // DefList
		/* DefList : Def DefList
		 * 	       |
		 * 	       ;
		 */
		TreeNode *defNode = node->firstChild;  // Def | NULL
		while (defNode != NULL) {
			analyseDef(defNode, structureField, st);
			defNode = defNode->nextSibling;  // DefList
			defNode = defNode->firstChild;  // Def | NULL
		}
	}
	return t;
}

void analyseDef(TreeNode *def, FieldList *structField, SymbolTable *st)
{
	/* Def : Specifier DecList SEMI
	 * 	   ;
	 *
	 * DecList : Dec
	 * 	   	   | Dec COMMA DecList
	 * 	   	   ;
	 *
	 * Dec : VarDec
	 * 	   | VarDec ASSIGNOP Exp
	 * 	   ;
	 */
	TreeNode *specifier = def->firstChild;
	Type *specifierType = analyseSpecifier(specifier, st, 0);
	if (specifierType == NULL) {
		return;
	}
	TreeNode *decList = specifier->nextSibling;
	TreeNode *decNode = decList->firstChild;
	while (decNode != NULL) {
		TreeNode *varDec = decNode->firstChild;
		FieldList *varDecField = analyseVarDec(varDec, specifierType, structField, st);
		if (varDecField != NULL && varDec->nextSibling != NULL) {  // Dec : VarDec ASSIGNOP Exp
			Type *varDecType = varDecField->type;
			TreeNode *node = varDec->nextSibling;  // ASSIGNOP
			node = node->nextSibling;  // Exp
			Type *expType = analyseExp(node, st);
			if (expType != NULL && !isSameType(varDecType, expType)) {  // ERROR 5: 赋值号两边的表达式类型不匹配
				serror(node, 5, "Type mismatched for assignment.");
			}
		}
		decNode = decNode->nextSibling;  // COMMA | NULL
		if (decNode != NULL) {
			decNode = decNode->nextSibling;  // DecList
			decNode = decNode->firstChild;  // Dec
		}
	}
	return;
}

FieldList *analyseVarDec(TreeNode *varDec, Type *baseType, FieldList *structField, SymbolTable *st)
{
	/* VarDec : ID
	 * 		  | VarDec LB INT RB
	 * 		  ;
	 */
	FieldList *field = (FieldList *)malloc(sizeof(FieldList));
	// 获取类型
	Type *t = analyseVarDecType(varDec, baseType, st);
	// 获取 ID
	TreeNode *node = varDec->firstChild;
	while (node->type != TOKEN_ID) {
		node = node->firstChild;
	}
	FieldList *redefined = findSymbol(st, node->sval);
	if (redefined != NULL) {
		if (structField == NULL) {  // ERROR 3: 变量名重复定义
			char msg[SERROR_MSG_LEN];
			sprintf(msg, "Redefined variable \"%s\".", node->sval);
			serror(node, 3, msg);
			free(field);
			free(t);
			return NULL;
		} else {  // ERROR 15: 结构体中域名重复定义
			char msg[SERROR_MSG_LEN];
			sprintf(msg, "Redefined field \"%s\".", node->sval);
			serror(node, 15, msg);
			free(field);
			free(t);
			return NULL;
		}
	}
	strcpy(field->name, node->sval);
	field->type = t;
	insertSymbol(st, field);
	if (structField != NULL) {  // 尾插法
		FieldList *tail = structField;
		while (tail->tail != NULL) {
			tail = tail->tail;
		}
		tail->tail = field;
	}
	return field;
}

Type *analyseVarDecType(TreeNode *varDec, Type *baseType, SymbolTable *st)
{
	Type *t = (Type *)malloc(sizeof(Type));
	TreeNode *node = varDec->firstChild;
	if (node->type == TOKEN_ID) {
		t->kind = baseType->kind;
		t->u = baseType->u;
		return t;
	} else {
		t->kind = ARRAY;
		Type *elemType = analyseVarDecType(node, baseType, st);
		t->u.array.elem = elemType;
		node = node->nextSibling;  // LB
		node = node->nextSibling;  // INT
		if (elemType->kind == ARRAY) {
			t->u.array.size = node->ival * elemType->u.array.size;
		} else {
			t->u.array.size = node->ival * 4;
		}
		return t;
	}
}

Type *analyseExp(TreeNode *root, SymbolTable *st)
{
	/* Exp : Exp ASSIGNOP Exp
	 * 	   | Exp AND Exp
	 * 	   | Exp OR Exp
	 * 	   | Exp RELOP Exp
	 * 	   | Exp PLUS Exp
	 * 	   | Exp MINUS Exp
	 * 	   | Exp STAR Exp
	 * 	   | Exp DIV Exp
	 * 	   | LP Exp RP
	 * 	   | MINUS Exp
	 * 	   | NOT Exp
	 * 	   | ID LP Args RP
	 * 	   | ID LP RP
	 * 	   | Exp LB Exp RB
	 * 	   | Exp DOT ID
	 * 	   | ID
	 * 	   | INT
	 * 	   | FLOAT
	 * 	   ;
	 */
	Type *t = (Type *)malloc(sizeof(Type));
	TreeNode *node = root->firstChild;
	TreeNode *next = node->nextSibling;
	if (next == NULL) {
		if (node->type == TOKEN_ID) {
			FieldList *field = findSymbol(st, node->sval);
			if (field == NULL) {  // ERROR 1: 变量在使用时未经定义
				char msg[SERROR_MSG_LEN];
				sprintf(msg, "Undefined variable \"%s\".", node->sval);
				serror(node, 1, msg);
				free(t);
				return NULL;
			}
			t->kind = field->type->kind;
			t->u = field->type->u;
			return t;
		} else if (node->type == TOKEN_DEC || node->type == TOKEN_OCT || node->type == TOKEN_HEX) {
			t->kind = BASIC;
			t->u.basic = _INT;
			return t;
		} else if (node->type == TOKEN_FLOAT) {
			t->kind = BASIC;
			t->u.basic = _FLOAT;
			return t;
		}
	} else {
		if (strcmp(node->name, "Exp") != 0) {
			if (strcmp(node->name, "LP") == 0) {  // LP Exp RP
				TreeNode *node1 = node->nextSibling;  // Exp
				Type *t1 = analyseExp(node1, st);
				if (t1 == NULL) return NULL;
				t->kind = t1->kind;
				t->u = t1->u;
			} else if (strcmp(node->name, "MINUS") == 0) {  // MINUS Exp
				TreeNode *node1 = node->nextSibling;  // Exp
				Type *t1 = analyseExp(node1, st);
				if (t1 == NULL) return NULL;
				if (t1->kind != BASIC) {  // ERROR 7: 操作数类型不匹配，只有 INT 和 FLOAT 类型可以进行负号运算
					serror(next, 7, "Type mismatched for operand.");
					free(t);
					return NULL;
				}
				t->kind = t1->kind;
				t->u = t1->u;
				return t;
			} else if (strcmp(node->name, "NOT") == 0) {  // EXP: NOT Exp
				TreeNode *node1 = node->nextSibling;  // Exp
				Type *t1 = analyseExp(node1, st);
				if (t1 == NULL) return NULL;
				if (!(t1->kind == BASIC && t1->u.basic == _INT)) {  // ERROR 7: 操作数类型不匹配，只有 INT 类型可以进行逻辑运算
					serror(next, 7, "Type mismatched for operand.");
					free(t);
					return NULL;
				}
			} else if (strcmp(node->name, "ID") == 0) {  // ID LP Args RP | ID LP RP
				FieldList *field = findSymbol(st, node->sval);
				if (field == NULL) {  // ERROR 2: 函数在使用时未经定义
					char msg[SERROR_MSG_LEN];
					sprintf(msg, "Undefined function \"%s\".", node->sval);
					serror(node, 2, msg);
					free(t);
					return NULL;
				}
				if (field->type->kind != FUNCTION) {  // ERROR 11: 对非过程名使用过程调用操作符
					char msg[SERROR_MSG_LEN];
					sprintf(msg, "\"%s\" is not a function.", node->sval);
					serror(node, 11, msg);
					free(t);
					return NULL;
				}
				TreeNode *node1 = node->nextSibling;  // LP
				node1 = node1->nextSibling;  // Args | RP
				if (strcmp(node1->name, "RP") == 0) {  // ID LP RP
					if (field->type->u.function.paramNum != 0) {  // ERROR 9: 函数调用参数个数不匹配
						char msg[SERROR_MSG_LEN];
						sprintf(msg, "Function \"%s\" is not applicable with 0 arguments.", node->sval);
						serror(node, 9, msg);
						free(t);
						return NULL;
					} else {
						t->kind = field->type->u.function.returnType->kind;
						t->u = field->type->u.function.returnType->u;
						return t;
					}
				} else if (strcmp(node1->name, "Args") == 0) {  // ID LP Args RP
					TreeNode *args = node1;
					Type argTypes[MAX_PARAM_NUM];
					int argNum = getArgTypes(args, st, argTypes);
					if (argNum != field->type->u.function.paramNum) {  // ERROR 9: 函数调用参数个数不匹配
						char msg[SERROR_MSG_LEN];
						sprintf(msg, "Function \"%s\" is not applicable with %d arguments.", node->sval, argNum);
						serror(node, 9, msg);
						free(t);
						return NULL;
					}
					FieldList *var = field->type->u.function.paramList;
					for (int i = 0; i < argNum; i++) {
						if (!isSameType(&argTypes[i], var->type)) {  // ERROR 9: 函数调用参数类型不匹配
							char msg[SERROR_MSG_LEN];
							sprintf(msg, "Function \"%s\" is not applicable with #%d argument.", node->sval, i + 1);
							serror(node, 9, msg);
							free(t);
							return NULL;
						}
						var = var->tail;
					}
					t->kind = field->type->u.function.returnType->kind;
					t->u = field->type->u.function.returnType->u;
					return t;
				}
			}
		} else {  // node->name == "Exp"
			Type *t1 = analyseExp(node, st);
			if (t1 == NULL) return NULL;
			if (strcmp(next->name, "LB") == 0) {  // Exp LB Exp RB
				if (t1->kind != ARRAY) {  // ERROR 10: 数组访问的对象不是数组
					char msg[SERROR_MSG_LEN];
					sprintf(msg, "\"%s\" is not an array.", node->firstChild->sval);
					serror(next, 10, msg);
					free(t);
					return NULL;
				}
				TreeNode *node2 = next->nextSibling;  // Exp
				Type *t2 = analyseExp(node2, st);
				if (t2 == NULL) return NULL;
				if (t2->kind != BASIC || t2->u.basic != _INT) {  // ERROR 12: 数组访问的下标不是整数
					char msg[SERROR_MSG_LEN];
					if (t2->kind == BASIC && t2->u.basic == _FLOAT) {
						sprintf(msg, "\"%f\" is not an integer.", node2->firstChild->fval);
					} else {
						sprintf(msg, "\"%s\" is not an integer.", node2->firstChild->sval);
					}
					serror(next, 12, msg);
					free(t);
					return NULL;
				}
				t->kind = t1->u.array.elem->kind;
				t->u = t1->u.array.elem->u;
				return t;
			} else if (strcmp(next->name, "DOT") == 0) {  // Exp DOT ID
				if (t1->kind != STRUCTURE) {  // ERROR 13: 对非结构体类型变量使用“.”操作符
					char msg[SERROR_MSG_LEN];
					sprintf(msg, "Illegal use of \".\".");
					serror(next, 13, msg);
					free(t);
					return NULL;
				}
				TreeNode *node2 = next->nextSibling;  // ID
				FieldList *field = t1->u.structure;
				while (field != NULL) {
					if (strcmp(field->name, node2->sval) == 0) {
						t->kind = field->type->kind;
						t->u = field->type->u;
						return t;
					}
					field = field->tail;
				}
				// ERROR 14: 访问结构体中未定义过的域
				char msg[SERROR_MSG_LEN];
				sprintf(msg, "Non-existent field \"%s\".", node2->sval);
				serror(next, 14, msg);
				free(t);
				return NULL;
			} else {
				if (strcmp(next->name, "ASSIGNOP") == 0) {  // Exp ASSIGNOP Exp
					bool isLeft = isLeftVal(node);
					if (!isLeft) {  // ERROR 6: 赋值号左边出现一个只有右值的表达式
						serror(node, 6, "The left-hand side of an assignment must be a variable.");
						free(t);
						return NULL;
					}
					TreeNode *node2 = next->nextSibling;  // Exp
					Type *t2 = analyseExp(node2, st);
					if (t2 == NULL) return NULL;
					if (!isSameType(t1, t2)) {  // ERROR 5: 赋值号两边的表达式类型不匹配
						serror(next, 5, "Type mismatched for assignment.");
						free(t);
						return NULL;
					}
					t->kind = t1->kind;
					t->u = t1->u;
					return t;
				} else if (strcmp(next->name, "AND") == 0 || strcmp(next->name, "OR") == 0) {
					TreeNode *node2 = next->nextSibling;  // Exp
					Type *t2 = analyseExp(node2, st);
					if (t2 == NULL) return NULL;
					bool t1IsInt = t1->kind == BASIC && t1->u.basic == _INT;
					bool t2IsInt = t2->kind == BASIC && t2->u.basic == _INT;
					if (!t1IsInt || !t2IsInt) {  // ERROR 7: 操作数类型与操作符不匹配，只有 INT 类型可以进行逻辑运算
						serror(next, 7, "Type mismatched for operands.");
						free(t);
						return NULL;
					}
					t->kind = t1->kind;
					t->u = t1->u;
					return t;
				} else {  // PLUS, MINUS, STAR, DIV, RELOP
					TreeNode *node2 = next->nextSibling;  // Exp
					Type *t2 = analyseExp(node2, st);
					if (t2 == NULL) return NULL;
					if (!isSameType(t1, t2)) {  // ERROR 7: 操作数类型与操作符不匹配，只有 INT 和 FLOAT 类型可以进行算术和比较运算
						serror(next, 7, "Type mismatched for operands.");
						free(t);
						return NULL;
					}
					t->kind = t1->kind;
					t->u = t1->u;
					return t;
				}
			}
		}
	}
	return NULL;
}

Type *getVarTypes(TreeNode *varList, SymbolTable *st)
{
	/* VarList : ParamDec COMMA VarList
	 * 		   | ParamDec
	 * 		   ;
	 * ParamDec : Specifier VarDec
	 * 			;
	 */
	Type *t = (Type *)malloc(sizeof(Type));
	t->kind = FUNCTION;
	FieldList *head = (FieldList *)malloc(sizeof(FieldList));
	head->tail = NULL;
	FieldList *p = head;
	TreeNode *paramDec = varList->firstChild;
	int paramCnt = 0;
	while (paramDec != NULL) {
		TreeNode *specifier = paramDec->firstChild;
		Type *specifierType = analyseSpecifier(specifier, st, 0);
		if (specifierType == NULL) {
			t->u.function.paramNum = paramCnt;
			t->u.function.paramList = head->tail;
			return t;
		}
		TreeNode *varDec = specifier->nextSibling;
		FieldList *varDecField = analyseVarDec(varDec, specifierType, NULL, st);
		p->tail = varDecField;
		paramCnt++;
		p = p->tail;
		paramDec = paramDec->nextSibling;  // COMMA | NULL
		if (paramDec != NULL) {
			paramDec = paramDec->nextSibling;  // VarList
			paramDec = paramDec->firstChild;  // ParamDec
		}
	}
	t->u.function.paramNum = paramCnt;
	t->u.function.paramList = head->tail;
	free(head);
	return t;
}

int getArgTypes(TreeNode *args, SymbolTable *st, Type *argTypes)
{
	TreeNode *exp = args->firstChild;
	int argCnt = 0;
	while (exp != NULL) {
		Type *expType = analyseExp(exp, st);
		if (expType == NULL) return argCnt;
		argTypes[argCnt++] = *expType;
		free(expType);
		exp = exp->nextSibling;  // COMMA | NULL
		if (exp != NULL) {
			exp = exp->nextSibling;  // Args
			exp = exp->firstChild;  // Exp
		}
	}
	return argCnt;
}

void serror(TreeNode *node, int typeId, char *msg)
{
	fprintf(stderr, "Error type %d at Line %d: %s\n", typeId, node->lineno, msg);
}

bool isSameType(Type *t1, Type *t2)
{
	if (t1->kind != t2->kind) {
		return false;
	}
	if (t1->kind == BASIC) {
		return t1->u.basic == t2->u.basic;
	} else if (t1->kind == ARRAY) {
		return isSameType(t1->u.array.elem, t2->u.array.elem);
	} else if (t1->kind == STRUCTURE) {
		return strcmp(t1->u.structure->name, t2->u.structure->name) == 0;  // 结构体间的类型等价机制为名等价
	}
	return false;
}

bool isLeftVal(TreeNode *exp)
{
	if (exp->firstChild->type == TOKEN_ID) {
		return true;
	} else {
		TreeNode *node = exp->firstChild;
		if (node == NULL) return false;  // 常量
		if (strcmp(node->name, "Exp") == 0) {
			node = node->nextSibling;
			if (strcmp(node->name, "LB") == 0) {
				return true;
			} else if (strcmp(node->name, "DOT") == 0) {
				return true;
			}
		}
	}
	return false;
}

FieldList *createRead()
{
	FieldList *read = (FieldList *)malloc(sizeof(FieldList));
	strcpy(read->name, "read");
	Type *readType = (Type *)malloc(sizeof(Type));
	read->type = readType;
	readType->kind = FUNCTION;
	Type *retType = (Type *)malloc(sizeof(Type));
	retType->kind = BASIC;
	retType->u.basic = _INT;
	readType->u.function.returnType = retType;
	readType->u.function.paramNum = 0;
	readType->u.function.paramList = NULL;
	read->tail = NULL;
	return read;
}

FieldList *createWrite()
{
	FieldList *write = (FieldList *)malloc(sizeof(FieldList));
	strcpy(write->name, "write");
	Type *writeType = (Type *)malloc(sizeof(Type));
	write->type = writeType;
	writeType->kind = FUNCTION;
	Type *retType = (Type *)malloc(sizeof(Type));
	retType->kind = BASIC;
	retType->u.basic = _INT;
	writeType->u.function.returnType = retType;
	writeType->u.function.paramNum = 1;
	FieldList *param = (FieldList *)malloc(sizeof(FieldList));
	strcpy(param->name, "output");
	Type *paramType = (Type *)malloc(sizeof(Type));
	paramType->kind = BASIC;
	paramType->u.basic = _INT;
	param->type = paramType;
	param->tail = NULL;
	writeType->u.function.paramList = param;
	write->tail = NULL;
	return write;
}
