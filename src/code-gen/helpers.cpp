#include "helpers.h"
#include "../utils.h"
#include "memory.h"
#include "../ast/ast.h"
#include <sstream>
#include "../ast/misc.h"

#define LABEL_HASH uniqueId(labelCount++, labelCount * 3, labelCount - 10, labelCount * 7)

#define REG_PREFIX "--"

int labelCount = 0;

extern int currParamCount;

extern Stack *callStack;

extern int currScope;

string newLabel(string prefix) {
    return prefix + "__" + to_string(LABEL_HASH).substr(0, 3);
}

string move(string rdst, string rsrc) {
    return "move " + rdst + ", " + rsrc + "\n";
}

string clearScreen() {
    string code = "\n# screen setup\n";

    code += "li $a0, BRIGHT_WHITE\n";
    code += "li $a1, BLACK\n";
    code += "jal set_color\n";
    code += "jal clear_screen\n";

    return code;
}

string prologue() {
    string code = "\n# prologue\n";

    code += stackAlloc();
    code += sw("$fp", 0, "$sp");
    code += move("$fp", "$sp");

    return code + "\n";
}

string epilogue() {
    string code = "\n# epilogue\n";

    code += move("$sp", "$fp");
    code += lw("$fp", 0, "$sp");
    code += stackFree();

    return code + "\n";
}

string stackAlloc(int howMuch) {
    return "addi $sp, $sp, -" + to_string(howMuch) + "\n";
}

string stackFree(int howMuch) {
    return "addi $sp, $sp, " + to_string(howMuch) + "\n";
}

string addi(string dst, string a, int im) {
    stringstream strm;
    strm << "addi " << dst << ", " << a << ", " << im << endl;
    return strm.str();
}

string la(string reg, string id) {
    return "la " + reg + ", " + id + "\n";
}

string io(string op, string src, int offset, string dst) {
    return op + " " + src + ", " + to_string(offset) + "(" + dst + ")\n";
}

string sw(string src, int offset, string dst) {
    return io("sw", src, offset, dst);
}

string sb(string src, int offset, string dst) {
    return io("sb", src, offset, dst);
}

string lw(string src, int offset, string dst) {
    return io("lw", src, offset, dst);
}

string lb(string src, int offset, string dst) {
    return io("lb", src, offset, dst);
}

string toRegStr(int i, char rt) {
    string str = "$";
    str.push_back(rt);
    return str + to_string(i);
}

string globalSection(map<string, string> *globals) {
    string str;
    map<string, string>::iterator it = globals->begin();

    while (it != globals->end()) {
        str += (it++)->second;
        if (it !=globals->end())
            str += "\n";
    }

    return str;
}

string dataSection(map<string, DataElement> *data) {
    string str;
    map<string, DataElement>::iterator it = data->begin();

    while (it != data->end()) {
        str += (it++)->second.code;
        if (it != data->end())
            str += "\n";
    }

    return str;
}

string pushParams(vector<Parameter*> *params) {
    string code;

    if (params->size())
        code += "\n# Arg Pushing\n";

    for (int i = 0; i < params->size(); i++) {
        string reg = toRegStr(i, 'a');
        Parameter *param = (*params)[i];
        bool isByte = param->dataType == CHAR && !(dynamic_cast<ArrayDeclarator*>(param->declarator));
        int siz = isByte ? 1 : 4;
        callStack->push(param->declarator->id, siz);
        code += stackAlloc(siz);
        code += siz == 4 ? sw(reg, 0, "$sp") : sb(reg, 0, "$sp");
    }

    return code;
}

string pushReturnAddr() {
    string code;

    callStack->push(REG_PREFIX+string("$ra"), 4);
    code += stackAlloc();
    code += sw("$ra", 0, "$sp");

    return code;
}

string popReturnAddr() {
    string code;

    code += lw("$ra", -4, "$fp");

    return code;
}

string functionPrologue(vector<Parameter*> *params) {
    string code;

    callStack->pushFrame();
    code += prologue();
    code += pushReturnAddr();
    code += pushParams(params);

    return code;
}

string functionEpilogue(bool pop) {
    string code;

    int csCopy = currScope;

    while (csCopy-- > 1)
        code += epilogue();

    code += popReturnAddr();
    code += epilogue();

    code += "\n";

    code += "jr $ra\n";

    return code;
}

string jr(Expression *expr) {
    string code;

    if (expr) {
        code += expr->genCode();
        code += move("$v0", toRegStr(expr->place));
        freeTemp(expr->place);
    }

    code += functionEpilogue(currParamCount);

    return code;
}

string stackPushReg(int i, char rt) {
    string code, reg = toRegStr(i, rt);

    callStack->push(REG_PREFIX+reg, 4);
    code += stackAlloc();
    code += sw(reg, 0, "$sp");

    return code;
}

string mapType(int t) {
    switch (t) {
        case INT:
            return "int";
        case CHAR:
            return "char";
        case VOID:
            return "void";
        case INT_PTR:
            return "int *";
        case CHAR_PTR:
            return "char *";
        case VOID_PTR:
            return "void *";
        default:
            return "";
    }
}

string join(vector<ASTNode*>* ls, string delim) {
    string str;
    int last = ls->size() - 1;

    for (int i = 0; i < ls->size(); i++) {
        ASTNode *node = (*ls)[i];

        str += node->toString();

        if (i != last)
            str += delim;
    }

    return str;
}

string lreg(int i, char rt) {
    string reg = toRegStr(i, rt);
    return lw(reg, callStack->getStackOffset(REG_PREFIX+reg), "$sp");
}

string useSaved(int *place) {
    *place = newSaved();
    return stackPushReg(*place, 's');
}

string releaseSaved(int place) {
    freeSaved(place);
    return lreg(place, 's');
}

string arrayAccessToStr(string id, int i) {
    return id + "[" + to_string(i) + "]";
}

string allocArray(string id, int dims, int siz) {
    string code;
    int temp = newTemp();
    string tempStr = toRegStr(temp);
    
    // Array Pointer
    code += stackAlloc(4);
    code += "addi " + tempStr + ", $sp, -4\n"; // Points to arr[0]
    code += sw(tempStr, 0, "$sp");
    freeTemp(temp);

    // Actual Array
    code += stackAlloc(siz * dims);

    callStack->push(id, 4);

    for (int i = 0; i < dims; i++)
        callStack->push(arrayAccessToStr(id, i), siz);

    return code;
}

string allocIntArray(string id, int dims) {
    return allocArray(id, dims, 4);
}

string allocCharArray(string id, int dims) {
    return allocArray(id, dims, 1);
}