#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "translate.h"

void translate(TreeNode *root, SymbolTable *st, IrTable *irt)
{
	if (root == NULL) {
		return;
	}
	if (strcmp(root->name, "ExtDef") == 0) {
		translateExtDef(root, st, irt);
	}
	translate(root->firstChild, st, irt);
	translate(root->nextSibling, st, irt);
}

void translateExtDef(TreeNode *extDef, SymbolTable *st, IrTable *irt)
{
	TreeNode *p = extDef->firstChild;  // specifier
	p = p->nextSibling;  // ExtDecList | FunDec | SEMI
	if (strcmp(p->name, "ExtDecList") == 0) {
		TreeNode *q = p->firstChild;  // VarDec
		while (q != NULL) {
			translateVarDec(q, st, irt);
			q = q->nextSibling;  // COMMA | NULL
			if (q != NULL) {
				q = q->nextSibling;  // ExtDecList
				q = q->firstChild;  // VarDec
			}
		}
	} else if (strcmp(p->name, "FunDec") == 0) {
		translateFunDec(p, st, irt);
		p = p->nextSibling;  // CompSt
		translateCompSt(p, st, irt);
	} else { // SEMI
		return;
	}
}

void translateVarDec(TreeNode *varDec, SymbolTable *st, IrTable *irt)
{
	TreeNode *p = varDec->firstChild;  // ID | VarDec LB INT RB
	if (p->nextSibling == NULL) {  // ID
		return;
	} else {  // VarDec LB INT RB
		TreeNode *q = varDec->firstChild;  // VarDec
		while (q->type != TOKEN_ID) {  // find id
			q = q->firstChild;
		}
		FieldList *sym = findSymbol(st, q->sval);
		char *var = genDec(sym->type->u.array.size, irt);
		sym->varName = var;
	}
}

void translateFunDec(TreeNode *funDec, SymbolTable *st, IrTable *irt)
{
	TreeNode *p = funDec->firstChild;  // ID
	FieldList *sym = findSymbol(st, p->sval);
	FieldList *pSym = sym->type->u.function.paramList;
	genFunction(sym->name, irt);
	for (int i = 0; i < sym->type->u.function.paramNum; i++) {
		char *var = genParam(irt);
		pSym->varName = var;
		pSym = pSym->tail;
	}
}

void translateCompSt(TreeNode *compSt, SymbolTable *st, IrTable *irt)
{
	TreeNode *p = compSt->firstChild;  // LC
	p = p->nextSibling;  // DefList | StmtList
	if (strcmp(p->name, "DefList") == 0) {
		TreeNode *q = p->firstChild;  // Def | NULL
		while (q != NULL) {
			translateDef(q, st, irt);
			q = q->nextSibling;  // DefList
			if (q != NULL) {
				q = q->firstChild;  // Def | NULL
			}
		}
		p = p->nextSibling;  // StmtList
	}
	TreeNode *q = p->firstChild;  // Stmt | NULL
	while (q != NULL) {
		translateStmt(q, st, irt);
		q = q->nextSibling;  // StmtList
		if (q != NULL) {
			q = q->firstChild;  // Stmt
		}
	}
}

void translateDef(TreeNode *def, SymbolTable *st, IrTable *irt)
{
	TreeNode *p = def->firstChild;  // specifier
	p = p->nextSibling;  // DecList
	TreeNode *q = p->firstChild;  // Dec
	while (q != NULL) {
		TreeNode *varDec = q->firstChild;  // VarDec
		translateVarDec(varDec, st, irt);
		if (varDec->nextSibling != NULL) {  // Dec : VarDec ASSIGNOP Exp
			TreeNode *exp = varDec->nextSibling->nextSibling;  // Exp
			char *temp = newTemp();
			translateExp(exp, st, irt, temp);
			char *var = newVar();
			genAssign(var, temp, irt);
			FieldList *sym = findSymbol(st, varDec->firstChild->sval);
			sym->varName = var;
		}
		q = q->nextSibling;  // COMMA | NULL
		if (q != NULL) {
			q = q->nextSibling;  // DecList
			q = q->firstChild;  // Dec
		}
	}
}

void translateStmt(TreeNode *stmt, SymbolTable *st, IrTable *irt)
{
	TreeNode *p = stmt->firstChild;
	if (strcmp(p->name, "Exp") == 0) {
		translateExp(p, st, irt, NULL);
	} else if (strcmp(p->name, "CompSt") == 0) {
		translateCompSt(p, st, irt);
	} else if (strcmp(p->name, "RETURN") == 0) {
		TreeNode *exp = p->nextSibling;
		char *temp = newTemp();
		translateExp(exp, st, irt, temp);
		genReturn(temp, irt);
	} else if (strcmp(p->name, "IF") == 0) {
		translateIf(stmt, st, irt);
	} else if (strcmp(p->name, "WHILE") == 0) {
		translateWhile(stmt, st, irt);
	}
}

void translateExp(TreeNode *exp, SymbolTable *st, IrTable *irt, char *place)
{
	TreeNode *first = exp->firstChild;
	TreeNode *second = first->nextSibling;
	if (second == NULL) {
		if (place != NULL) {
			if (first->type == TOKEN_ID) {
				FieldList *sym = findSymbol(st, first->sval);
				genAssign(place, sym->varName, irt);
			} else if (first->type == TOKEN_DEC || first->type == TOKEN_HEX || first->type == TOKEN_OCT) {
				char *val = genConstVal(first->ival);
				genAssign(place, val, irt);
				free(val);
			}
		}
	} else {
		if (strcmp(first->name, "Exp") != 0) {
			if (strcmp(first->name, "LP") == 0) {  // LP Exp RP
				translateExp(second, st, irt, place);
			} else if (strcmp(first->name, "MINUS") == 0) {  // MINUS Exp
				char *temp = newTemp();
				translateExp(second, st, irt, temp);
				char *constZero = genConstVal(0);
				char *rhs = (char *)malloc(MAX_IR_LEN);
				sprintf(rhs, "%s - %s", constZero, temp);
				genAssign(place, rhs, irt);
				free(constZero);
				free(rhs);
			} else if (strcmp(first->name, "NOT") == 0) {  // NOT Exp
				char *label1 = newLabel();
				char *label2 = newLabel();
				char *constTrue = genConstVal(1);
				char *constFalse = genConstVal(0);
				if (place != NULL) {
					genAssign(place, constFalse, irt);
				}
				translateCond(exp, label1, label2, st, irt);
				genLabel(label1, irt);
				if (place != NULL) {
					genAssign(place, constTrue, irt);
				}
				genLabel(label2, irt);
				free(label1);
				free(label2);
				free(constTrue);
				free(constFalse);
			} else if (strcmp(first->name, "ID") == 0) {  // ID LP Args RP | ID LP RP
				FieldList *func = findSymbol(st, first->sval);
				if (strcmp(func->name, "read") == 0) {
					genRead(place, irt);
					return;
				}
				if (strcmp(func->name, "write") == 0) {
					TreeNode *arg = second->nextSibling->firstChild;
					char *temp = newTemp();
					translateExp(arg, st, irt, temp);
					genWrite(temp, irt);
					return;
				}
				TreeNode *args = second->nextSibling;
				char *argNames[MAX_PARAM_NUM];
				int argCnt = 0;
				if (strcmp(args->name, "Args") == 0) {
					TreeNode *arg = args->firstChild;
					while (arg != NULL) {
						TreeNode *child = arg->firstChild;
						if (child->type == TOKEN_ID) {
							FieldList *sym = findSymbol(st, child->sval);
							argNames[argCnt++] = sym->varName;
						} else if (child->type == TOKEN_DEC || child->type == TOKEN_HEX || child->type == TOKEN_OCT) {
							char *val = genConstVal(child->ival);
							argNames[argCnt++] = val;
						} else {
							char *temp = newTemp();
							translateExp(arg, st, irt, temp);
							argNames[argCnt++] = temp;
						}
						arg = arg->nextSibling;
						if (arg != NULL) {
							arg = arg->nextSibling;  // Args
							arg = arg->firstChild;  // Exp
						}
					}
					for (int i = argCnt - 1; i >= 0; i--) {
						genArg(argNames[i], irt);
					}
				}
				char *call = (char *)malloc(MAX_IR_LEN);
				sprintf(call, "CALL %s", func->name);
				genAssign(place, call, irt);
			}
		} else {  // first->name == "Exp"
			if (strcmp(second->name, "LB") == 0) {
				FieldList *array = findSymbol(st, first->firstChild->sval);
				TreeNode *index = second->nextSibling;
				char *tempIndex = newTemp();
				translateExp(index, st, irt, tempIndex);  // get index
				char *offset = newTemp();
				char *size = genConstVal(4);
				char *mul = (char *)malloc(MAX_IR_LEN);
				sprintf(mul, "%s * %s", tempIndex, size);
				genAssign(offset, mul, irt);  // get offset
				char *tempAddr = newTemp();
				char *add = (char *)malloc(MAX_IR_LEN);
				sprintf(add, "&%s + %s", array->varName, offset);
				genAssign(tempAddr, add, irt);  // get address
				char *fetch = (char *)malloc(MAX_IR_LEN);
				sprintf(fetch, "*%s", tempAddr);
				genAssign(place, fetch, irt);  // fetch value
				free(tempIndex);
				free(offset);
				free(tempAddr);
				free(add);
				free(fetch);
			} else if (strcmp(second->name, "AND") == 0 || strcmp(second->name, "OR") == 0 ||\
                       strcmp(second->name, "RELOP") == 0 ) {
				char *label1 = newLabel();
				char *label2 = newLabel();
				char *constTrue = genConstVal(1);
				char *constFalse = genConstVal(0);
				if (place != NULL) {
					genAssign(place, constFalse, irt);
				}
				translateCond(exp, label1, label2, st, irt);
				genLabel(label1, irt);
				if (place != NULL) {
					genAssign(place, constTrue, irt);
				}
				genLabel(label2, irt);
				free(label1);
				free(label2);
				free(constTrue);
				free(constFalse);
			} else {
				TreeNode *exp1 = first;
				TreeNode *exp2 = second->nextSibling;
				if (strcmp(second->name, "ASSIGNOP") == 0) {
					FieldList *var;
					if (exp1->firstChild->type == TOKEN_ID) {
						var = findSymbol(st, exp1->firstChild->sval);
					} else {  // ARRAY
						var = findSymbol(st, exp1->firstChild->firstChild->sval);
					}
					if (var->varName == NULL) {
						var->varName = newVar();
					}
					if (var->type->kind == ARRAY) {
						TreeNode *index = exp1->firstChild->nextSibling->nextSibling;
						char *tempIndex = newTemp();
						translateExp(index, st, irt, tempIndex);  // get index
						char *offset = newTemp();
						char *size = genConstVal(4);
						char *mul = (char *)malloc(MAX_IR_LEN);
						sprintf(mul, "%s * %s", tempIndex, size);
						genAssign(offset, mul, irt);  // get offset
						char *tempAddr = newTemp();
						char *add = (char *)malloc(MAX_IR_LEN);
						sprintf(add, "&%s + %s", var->varName, offset);
						genAssign(tempAddr, add, irt);  // get address
						char *temp = newTemp();
						translateExp(exp2, st, irt, temp);
						char *store = (char *)malloc(MAX_IR_LEN);
						sprintf(store, "*%s", tempAddr);
						genAssign(store, temp, irt);  // store value
						if (place != NULL) {
							genAssign(place, temp, irt);
						}
						free(tempIndex);
						free(offset);
						free(tempAddr);
						free(add);
						free(store);
					} else {
						char *temp = newTemp();
						translateExp(exp2, st, irt, temp);
						genAssign(var->varName, temp, irt);
						if (place != NULL) {
							genAssign(place, var->varName, irt);
						}
					}
				} else {  // PLUS, MINUS, STAR, DIV
					char *t1 = newTemp();
					char *t2 = newTemp();
					translateExp(exp1, st, irt, t1);
					translateExp(exp2, st, irt, t2);
					char *op = (char *)malloc(MAX_IR_LEN);
					if (strcmp(second->name, "PLUS") == 0) {
						sprintf(op, "%s + %s", t1, t2);
					} else if (strcmp(second->name, "MINUS") == 0) {
						sprintf(op, "%s - %s", t1, t2);
					} else if (strcmp(second->name, "STAR") == 0) {
						sprintf(op, "%s * %s", t1, t2);
					} else if (strcmp(second->name, "DIV") == 0) {
						sprintf(op, "%s / %s", t1, t2);
					}
					genAssign(place, op, irt);
					free(op);
				}
			}
		}
	}
}

void translateCond(TreeNode *exp, char *labelTrue, char *labelFalse, SymbolTable *st, IrTable *irt)
{
	TreeNode *first = exp->firstChild;
	TreeNode *second = first->nextSibling;
	if (strcmp(first->name, "NOT") == 0) {
		translateCond(second, labelFalse, labelTrue, st, irt);
	} else if (second != NULL) {
		TreeNode *exp1 = first;
		TreeNode *exp2 = second->nextSibling;
		if (exp2 != NULL) {
			if (strcmp(second->name, "RELOP") == 0) {
				char *t1 = newTemp();
				char *t2 = newTemp();
				translateExp(exp1, st, irt, t1);
				translateExp(exp2, st, irt, t2);
				genIfGoto(t1, t2, second->sval, labelTrue, irt);
				genGoto(labelFalse, irt);
			} else if (strcmp(second->name, "AND") == 0) {
				char *label = newLabel();
				translateCond(exp1, label, labelFalse, st, irt);
				genLabel(label, irt);
				translateCond(exp2, labelTrue, labelFalse, st, irt);
			} else if (strcmp(second->name, "OR") == 0) {
				char *label = newLabel();
				translateCond(exp1, labelTrue, label, st, irt);
				genLabel(label, irt);
				translateCond(exp2, labelTrue, labelFalse, st, irt);
			}
		}
	} else {  // other cases
		char *temp = newTemp();
		translateExp(exp, st, irt, temp);
		genIfGoto(temp, "0", "!=", labelTrue, irt);
		genGoto(labelFalse, irt);
	}
}

void translateIf(TreeNode *stmt, SymbolTable *st, IrTable *irt)
{
	TreeNode *ifNode = stmt->firstChild;  // IF
	TreeNode *expNode = ifNode->nextSibling->nextSibling;
	TreeNode *stmt1Node = expNode->nextSibling->nextSibling;
	TreeNode *elseNode = stmt1Node->nextSibling;
	if (elseNode == NULL) {  // IF LP Exp RP Stmt1
		char *label1 = newLabel();
		char *label2 = newLabel();
		translateCond(expNode, label1, label2, st, irt);
		genLabel(label1, irt);
		translateStmt(stmt1Node, st, irt);
		genLabel(label2, irt);
	} else {  // IF LP Exp RP Stmt1 ELSE Stmt2
		TreeNode *stmt2Node = elseNode->nextSibling;
		char *label1 = newLabel();
		char *label2 = newLabel();
		char *label3 = newLabel();
		translateCond(expNode, label1, label2, st, irt);
		genLabel(label1, irt);
		translateStmt(stmt1Node, st, irt);
		genGoto(label3, irt);
		genLabel(label2, irt);
		translateStmt(stmt2Node, st, irt);
		genLabel(label3, irt);
	}
}

void translateWhile(TreeNode *stmt, SymbolTable *st, IrTable *irt)
{
	TreeNode *whileNode = stmt->firstChild;  // WHILE
	TreeNode *expNode = whileNode->nextSibling->nextSibling;
	TreeNode *stmtNode = expNode->nextSibling->nextSibling;
	char *label1 = newLabel();
	char *label2 = newLabel();
	char *label3 = newLabel();
	genLabel(label1, irt);
	translateCond(expNode, label2, label3, st, irt);
	genLabel(label2, irt);
	translateStmt(stmtNode, st, irt);
	genGoto(label1, irt);
	genLabel(label3, irt);
}

char *newVar()
{
	static int varCnt = 1;
	char *var = (char *)malloc(MAX_VAR_LEN);
	sprintf(var, "v%d", varCnt++);
	return var;
}

char *newTemp()
{
	static int tempCnt = 1;
	char *temp = (char *)malloc(MAX_TEMP_LEN);
	sprintf(temp, "t%d", tempCnt++);
	return temp;
}

char *newLabel()
{
	static int labelCnt = 1;
	char *label = (char *)malloc(MAX_TEMP_LEN);
	sprintf(label, "label%d", labelCnt++);
	return label;
}

char *genConstVal(int val)
{
	char *valStr = (char *)malloc(MAX_VAR_LEN);
	sprintf(valStr, "#%d", val);
	return valStr;
}

char *genDec(int size, IrTable *irt)
{
	char *var = newVar();
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "DEC %s %d", var, size);
	irt->irs[irt->num++] = ir;
	return var;
}

void genFunction(char *name, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "FUNCTION %s :", name);
	irt->irs[irt->num++] = ir;
}

char *genParam(IrTable *irt)
{
	char *var = newVar();
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "PARAM %s", var);
	irt->irs[irt->num++] = ir;
	return var;
}

void genAssign(char *dst, char *src, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "%s := %s", dst, src);
	irt->irs[irt->num++] = ir;
}

void genArg(char *name, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "ARG %s", name);
	irt->irs[irt->num++] = ir;
}

void genRead(char *place, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "READ %s", place);
	irt->irs[irt->num++] = ir;
}

void genWrite(char *arg, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "WRITE %s", arg);
	irt->irs[irt->num++] = ir;
}

void genIfGoto(char *x, char *y, char *relop, char *label, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "IF %s %s %s GOTO %s", x, relop, y, label);
	irt->irs[irt->num++] = ir;
}

void genGoto(char *label, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "GOTO %s", label);
	irt->irs[irt->num++] = ir;
}

void genLabel(char *label, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "LABEL %s :", label);
	irt->irs[irt->num++] = ir;
}

void genReturn(char *place, IrTable *irt)
{
	char *ir = (char *)malloc(MAX_IR_LEN);
	sprintf(ir, "RETURN %s", place);
	irt->irs[irt->num++] = ir;
}

void saveIrs(IrTable *irt, char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		perror(filename);
		return;
	}
	for (int i = 0; i < irt->num; i++) {
		fprintf(fp, "%s\n", irt->irs[i]);
	}
	fclose(fp);
}


