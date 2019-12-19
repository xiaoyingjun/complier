#include<iostream>
#include<list>
#include<string>
#include<iomanip>
#include<stack>
#include<set>
#include<unordered_map>
#include "lexicalAnalyzer.h"
using namespace std;

#define VOID_T -1
#define BOOL_T 1
#define INT_T 2
#define FLOAT_T 3
#define STRUCT_T 4
#define INTARRAY_T 5
#define FLAOTARRAY_T 6
#define STRUCTARRAY_T 7
#define MAX_NUM 9999999
#define MAX_SYM 1000


struct propertyNode {
	string type;
	string pname;
	int intValue;
	float floatValue;
	propertyNode(string t, string p) {
		type = t;
		pname = p;
	}
};

struct complexType {
	string name;
	list<propertyNode*> property;
};

// 符号表
struct tableEntry {
	int type;
	string word;
	int intValue;
	float floatValue;
	int offset;
	tableEntry * arrayValue;
	complexType* structValue;
};

struct treeNode {
	int id;
	string type;
	string word;
	int node_line;
	set<int> truelist;
	set<int> falselist;
	set<int> nextlist;
	set<int> breaklist;
	list<treeNode*> children;
	tableEntry * entry;
};

// 函数信息
struct funcInfo {
	int paramNum;
	vector<int> paramTypes;
	string name;
	int returnType;
	int inAddr; // 函数所在代码开始的标号
	int outAddr; // 函数执行后返回标号
	tableEntry* returnValue;
	unordered_map<string, tableEntry> paramList;
};

vector<funcInfo* > funcList;
stack<struct funcInfo*> cs;
struct funcInfo* current;

struct funcInfo* isFunction(string fn) {
	vector<struct funcInfo*>::iterator it = funcList.begin();
	while (it != funcList.end()) {
		if ((*it)->name == fn)
			return (*it);
		it++;
	}
	return nullptr;
};


struct env {
	struct env* prev;
	unordered_map<string, tableEntry> symbolTable;
};

struct addrCode {
	int ins_lable;
	string op;
	tableEntry* arg1;
	tableEntry* arg2;
	tableEntry* result;
	int go_to;
};



list<complexType*> structList;

extern string type[];
extern string keyWord[];
extern string generfunc[];
extern string sepName[SYMBOLNUM];
extern string sepSymbol[SYMBOLNUM];
extern int CountLines(const char *filename);
addrCode * addr = new addrCode[MAX_NUM]; // 四元式数组

int error_flag = 0;//语法分析过程中是否有错误产生
int line_num = 0;
int node_index = 1;
int nextinstr = 1; //下一条指令的标号
struct env* top; // 当前符号表
int tempnum = 0;
int memOffset = 0;

// 根据变量名从符号表中获取元素，如果在当前作用域中没找到会向上寻找
struct tableEntry* get(struct env* e, string s) {
	for (env* tempEnv = e; tempEnv != NULL; tempEnv = tempEnv->prev) {
		unordered_map<string, tableEntry>::iterator it = tempEnv->symbolTable.find(s);
		if (it != tempEnv->symbolTable.end()) {
			tableEntry* entryPtr = &(it->second);
			return entryPtr;
		}
		unordered_map<string, tableEntry>::iterator itp = current->paramList.find(s);
		if (itp != current->paramList.end()) {
			tableEntry* entryPtr = &(itp->second);
			return entryPtr;
		}
	}
	return NULL;
}

// 像符号表中插入项
void insert(struct env* e, string s, struct tableEntry t) {
	pair<string, struct tableEntry> newEntry(s, t);
	(e->symbolTable).insert(newEntry);
}

int getIntValue(treeNode* root) {
	if (root == NULL) {
		return 0;
	}
	else {
		return 10;
	}
}

float getFloatValue(treeNode* root) {
	if (root == NULL) {
		return 0.0;
	}
	else {
		return 100.0;
	}
}

int getTypeValue(string type) {
	if (type == "int") {
		return INT_T;
	}
	else if (type == "float") {
		return FLOAT_T;
	}
	else {
		return STRUCT_T;
	}
}

set<int> merge(set<int> a, set<int> b) {
	set<int>::iterator it;
	for (it = b.begin(); it != b.end(); it++)  //使用迭代器进行遍历 
	{
		a.insert(*it);
	}
	return a;
}

// 回填
void backpatch(set<int> p, int l) {
	set<int>::iterator it;
	for (it = p.begin(); it != p.end(); it++) { //使用迭代器进行遍历 
		addr[*it].go_to = l;
	}
}

// 根据语句i建立一个新表
set<int> makelist(int i) {
	set<int> t;
	t.insert(i);
	return t;
}

// 计算某个节点的非空孩子节点个数
int numChildren(treeNode*  root) {
	int count = 0;
	list<treeNode*>::iterator it = root->children.begin();
	while (it != root->children.end()) {
		if ((*it) != NULL)
			count++;
		it++;
	}
	return count;
}

// 得到第 index 个孩子节点
treeNode* getNthChild(treeNode* root, int index) {
	int count = 0;
	list<treeNode*>::iterator it = root->children.begin();
	while (count < index) {
		it++;
		count++;
	}
	return (*it);
}

// 是否是 逻辑运算符
bool islogic(string s) {
	int n = 9;
	string logic[] = {
		"||", "&&", "==", "!=", "<", "<=", ">", ">=", "!"
	};
	for (int i = 0; i < n; i++) {
		if (s == logic[i])
			return true;
	}
	return false;
}

// 是否是 数学运算符
bool isArith(string s) {
	int n = 4;
	string arith[] = {
		"+", "-", "*", "/"
	};
	for (int i = 0; i < n; i++) {
		if (s == arith[i])
			return true;
	}
	return false;
}

// 类型转换，bool -> int, int -> float
int maxType(int type1, int type2) {
	return type1 > type2 ? type1 : type2;
}

// 在生成四元式时根据 两个操作数、当前操作符 判断 结果类型，处理未声明变量引用以及操作符类型不匹配错误，必要时完成类型转换
int getResultType(string s, tableEntry* arg1, tableEntry* arg2) {
	// 逻辑运算符
	if (islogic(s)) {
		// 对 NULL 的处理保证当出现引用未声明变量时会报告哪个变量未声明，在四元式中将其置为 null, 还可以继续向下分析
		if (arg1 == NULL || arg2 == NULL) {
			if (s == "!") {
				if (arg1 != NULL) {
					return BOOL_T;
				}
				else {
					return 0;
				}
			}
			return 0;
		}
		else {
			if ((arg1->type == INT_T || arg1->type == BOOL_T) && (arg2->type == INT_T || arg2->type == BOOL_T)) {
				return BOOL_T;
			}
			else {
				// 逻辑操作运算符两侧运算数类型不匹配
				cout << "operator type mismatch: " << arg1->word << "(" << arg1->type << ") " << s << " " << arg2->word << "(" << arg2->type << ") " << endl;
				return 0;
			}
		}
	}
	// ����������
	else if (isArith(s)) {
		if (arg1 == NULL || arg2 == NULL) {
			if (s == "-") {
				if (arg1 != NULL) {
					return arg1->type;
				}
				else {
					return 0;
				}
			}
			return 0;
		}
		else {
			if ((arg1->type == INT_T || arg1->type == BOOL_T || arg1->type == FLOAT_T) && (arg2->type == INT_T || arg2->type == BOOL_T || arg2->type == FLOAT_T)) {
				return maxType(arg1->type, arg2->type);
			}
			else {
				// ���������Ͳ�ƥ��
				cout << "operator type mismatch: " << arg1->word << "(" << arg1->type << ") " << s << " " << arg2->word << "(" << arg2->type << ") " << endl;;
				return 0;
			}
		}
	}
	// ��ֵ����
	else { // ������������������Ϊ׼���Ƿ��漰������
		if (arg1 == NULL)
			return 0;
		return arg1->type;
	}
}

// �����ĵ�ַ��
tableEntry * gen(string s, tableEntry * arg1, tableEntry * arg2, int go_to) {
	addrCode t;
	// 表达式
	if (s == "=") {
		t.arg1 = arg2;
		t.arg2 = NULL;
		t.op = "=";
		t.result = arg1;
		t.ins_lable = nextinstr;
		t.go_to = NULL;
		addr[nextinstr] = t;
		nextinstr++;
		return arg1;
	}
	// if
	else if (s == "j" || s == "j>" || s == "j<" || s == "j==" || s == "j!="
		|| s == "j>=" || s == "j<=") {
		t.ins_lable = nextinstr;
		t.op = s;
		t.arg1 = arg1;
		t.arg2 = arg2;
		t.go_to = go_to;
		t.result = NULL;
		addr[nextinstr] = t;
		nextinstr++;
		return t.result;
	}
	else if (s == "param") {
		t.ins_lable = nextinstr;
		t.op = s;
		t.arg1 = arg1;
		t.arg2 = NULL;
		t.result = NULL;
		t.go_to = NULL;
		addr[nextinstr] = t;
		nextinstr++;
		return t.result;
	}
	else if (s == "call") {
		t.ins_lable = nextinstr;
		t.op = s;
		t.arg1 = arg1;
		t.arg2 = arg2;

		string fname = arg1->word;
		struct funcInfo* fi = isFunction(fname);
		t.result = new tableEntry();
		t.result->type = fi->returnType;

		t.go_to = go_to;
		addr[nextinstr] = t;
		nextinstr++;
		return t.result;
	}
	else if (s == "return") {
		t.ins_lable = nextinstr;
		t.op = s;
		t.arg1 = arg1;
		t.arg2 = NULL;
		t.result = NULL;
		t.go_to = NULL;
		addr[nextinstr] = t;
		nextinstr++;
		return t.result;
	}
	// 数组
	else if (s == "[]") {
		t.ins_lable = nextinstr;
		t.op = s;
		t.arg1 = arg1;
		t.arg2 = arg2;
		t.result = &arg1->arrayValue[arg2->intValue];
		t.result->word = arg1->word + "[" + to_string(arg2->intValue) + "]";
		t.go_to = NULL;
		addr[nextinstr] = t;
		nextinstr++;
		return t.result;
	}
	// 表达式
	else {
		t.arg1 = arg1;
		t.arg2 = arg2;
		t.result = new tableEntry();
		t.op = s;
		t.ins_lable = nextinstr;
		t.go_to = NULL;
		t.result->word = "t" + to_string(tempnum);
		t.result->type = getResultType(s, arg1, arg2);
		tempnum++;
		addr[nextinstr] = t;
		nextinstr++;
		return t.result;
	}
}
tableEntry* gen(tableEntry* arg1, propertyNode* property) {
	addrCode t;
	t.arg1 = arg1;
	tableEntry* temp = new tableEntry();
	temp->word = property->pname;
	t.arg2 = temp;
	t.result = new tableEntry();
	t.op = '.';
	t.ins_lable = nextinstr;
	t.go_to = NULL;
	t.result->word = "t" + to_string(tempnum);
	t.result->type = getTypeValue(property->type);
	tempnum++;
	addr[nextinstr] = t;
	nextinstr++;
	return t.result;

}
//在结构表中寻找有没有属性
/*
string findProperty(string structName, string propertyName) {
complexType* ctype = findStructName(structName);
list<propertyNode*>::iterator it = ctype->property.begin();
while (it != ctype->property.end()) {
if ((*it)->pname == propertyName) {
return (*it)->type;
}
}
cout << "Type have no attribute" + propertyName << endl;
return ;
}
*/

propertyNode* findProperty(complexType*structName, string propertyName) {
	list<propertyNode*>::iterator it = structName->property.begin();
	while (it != structName->property.end()) {
		if ((*it)->pname == propertyName) {
			return *it;
		}
		it++;
	}
	cout << "Type " + structName->name + "have no attribute" + propertyName << endl;
	return NULL;
}


//结构体是否已经定义过
complexType* findStructName(string structName) {
	list<complexType*>::iterator it = structList.begin();
	while (it != structList.end())
	{
		if ((*it)->name == structName)
		{
			return (*it);
		}
		it++;
	}
	cout << "This struct is not defined" << endl;
	return NULL;
}
/*
//重载findStructName,用于通过type值寻找已经被定义过的结构体
complexType* findStructName(int structName) {

structName = structName - 3;
list<complexType*>::iterator it = structList.begin();

while (structName > 0)
{
it++;
structName--;
}
return (*it);
}
*/

//计算struct的offset
int getOffset(complexType* structName) {
	int offset = 0;
	list<propertyNode*>::iterator it = structName->property.begin();
	while (it != structName->property.end())
	{
		if ((*it)->type == "int") {
			offset += 8;
		}
		if ((*it)->type == "float") {
			offset += 16;
		}
		it++;
	}
	return offset;
}

lexicalUnit look;
list<lexicalUnit> lst;
list<lexicalUnit>::iterator p;

stack<treeNode> tree;

void error() {
	cout << "error" << endl;
}

void error_judge()
{
	if (error_flag == 1)
	{
		system("pause");
	}
}
void move() { // ���¶�һλ
	p++;
	look = *p;
}

void match(string type, int l) {
	if (look.type == type) {
		move(); // match �Ļ�move, ��Ȼerror
	}
	else {
		error_flag = 1;
		if (type == "SEMICOLON")
		{
			cout << "error: missing semicolon! ';'\n";
		}
		if (type == "LBRACE")//��������
		{
			cout << "error: missing left brace! '{'\n";
		}
		if (type == "RBRACE")
		{
			cout << "error: missing right brace! '}'\n";
		}
		if (type == "LP")
		{
			cout << "error: missing left parenthesis! '('\n";
		}
		if (type == "RP")
		{
			cout << "error: missing right parenthesis! ')'\n";
		}
		if (type == "ID")
		{
			cout << "error: missing variable name!\n";
		}
		if (type == "COMPLEXTYPE")
		{
			cout << "error: missing struct name!\n";
		}
		cout << "The error line number is " << l - 1 << "." << endl;
		cout << endl;
	}
}

void initNode(treeNode* root, string type, string word, int line) {
	root->id = node_index++;
	root->type = type;
	root->word = word;
	root->node_line = line;
}

treeNode* program();
treeNode* structs();
treeNode* mystruct();//
treeNode* functions();
treeNode* argvs(struct funcInfo* fi);
treeNode* block();
treeNode* decls(complexType*);
treeNode* mytype(lexicalUnit t);
treeNode* stmt();
treeNode* halfassign(string type, lexicalUnit id, complexType*ctype, int num = 0);
treeNode* stmt();
treeNode* mybool();
treeNode* assign(int for_right = 0);
treeNode* join();
treeNode* equality();
treeNode* rel();
treeNode* expr(treeNode* subroot);
treeNode* term();
treeNode* unary();
treeNode* factor();
treeNode* N();
void display(treeNode* root);


treeNode* program() {
	treeNode* root = new treeNode();
	while (look.type != "$") {
		if (look.type == "STRUCT") {
			root->children.push_back(structs());
		}
		else if (look.type == "TYPE" || look.type == "COMPLEXTYPE") {
			root->children.push_back(functions());
		}
		else {
			return NULL;
		}
	}
	initNode(root, "Programs", "INNER", look.line);
	//treeNode* root = functions();
	//cout << look.type << endl;
	if (look.type == "$") {
		//cout << "success" << endl;
		return root;
	}
	else {
		return NULL; // error;
	}
}
treeNode* structs() {
	treeNode* root = new treeNode();
	while (look.type == "STRUCT") {
		root->children.push_back(mystruct());
	}
	initNode(root, "StructDeclarations", "INNER", look.line);
	return root;
}


treeNode* functions() {
	treeNode* root = new treeNode();
	while (look.type == "TYPE" || look.type == "COMPLEXTYPE") {
		struct funcInfo * fi = new funcInfo();

		treeNode* subroot = new treeNode();
		treeNode* t = mytype(look);
		if (t->word == "int") {
			fi->returnType = INT_T;
		}
		else if (t->word == "float") {
			fi->returnType = FLOAT_T;
		}
		else if (t->word == "void") {
			fi->returnType = VOID_T;
		}
		subroot->children.push_back(t);
		string function_name = look.word;
		fi->name = function_name;
		match("ID", look.line); // main 匹配
		match("LP", look.line);
		treeNode* argv = argvs(fi);
		subroot->children.push_back(argv);
		match("RP", look.line);

		top = NULL;
		tempnum = 0;
		fi->inAddr = nextinstr;
		funcList.push_back(fi);
		current = fi;

		treeNode* myblock = block();

		subroot->children.push_back(myblock);

		initNode(subroot, "Function", function_name, look.line);
		root->children.push_back(subroot);
	}
	initNode(root, "FunctionList", "INNER", look.line);
	return root;
}

treeNode* argvs(struct funcInfo* fi) {

	treeNode* root = new treeNode();
	int n = 0;
	fi->paramTypes.resize(10);
	while (look.type == "TYPE" || look.type == "COMPLEXTYPE") {
		struct tableEntry *newEntry = new tableEntry();
		if (look.word == "int") {
			fi->paramTypes.push_back(INT_T);
			newEntry->type = INT_T;
		}
		else if (look.word == "float") {
			fi->paramTypes.push_back(FLOAT_T);
			newEntry->type = FLOAT_T;
		}
		else {
			//pass
		}


		treeNode* subroot = new treeNode();
		treeNode* my_type = mytype(look);
		string argv_name = look.word;
		subroot->children.push_back(my_type);
		newEntry->word = argv_name;
		pair<string, struct tableEntry> ne(newEntry->word, *newEntry);
		fi->paramList.insert(ne);
		match("ID", look.line);
		if (look.type == "COMMA") {
			match("COMMA", look.line);
		}
		initNode(subroot, "Argv", argv_name, look.line);
		root->children.push_back(subroot);
		n++;
	}
	fi->paramNum = n;
	initNode(root, "ArgvList", "INNER", look.line);
	return root;
}

string typeToString(int t) {
	if (t == INT_T)
		return "int";
	else if (t == FLOAT_T)
		return "float";
	else if (t == VOID_T)
		return "void";
	else
		return "other type";
}


treeNode* funCall(struct funcInfo* fi) {
	treeNode* root = new treeNode();
	treeNode* funName = new treeNode();
	treeNode* funParmList = new treeNode();
	list<treeNode* > parameters;
	string fn = look.word;

	initNode(funName, "FunctionName", look.word, look.line);
	match("ID", look.line);

	match("LP", look.line);
	// 函数参数个数和类型匹配检查
	int n = 0;
	while (look.type != "RP" && n <= fi->paramNum) {
		treeNode* parm = mybool();
		if (parm->entry->type != fi->paramTypes[n]) {
			string s1 = typeToString(parm->entry->type);
			string s2 = typeToString(fi->paramTypes[n]);
			cout << "mismatch type error in " << fi->name << " : expected " << s2 << " but get " << s1 << endl;
		}
		parameters.push_back(parm);
		n++;
		funParmList->children.push_back(parm);
		if (look.type == "RP") {
			break;
		}
		match("COMMA", look.line);
	}

	if (n != fi->paramNum) {
		cout << "parameter number error in " << fi->name << " : expected " << fi->paramNum << " but get " << n << endl;
		while (look.type != "RP")
		{
			move();
		}
	}

	match("RP", look.line);
	initNode(funParmList, "CallParmList", "INNER", look.line);
	root->children.push_back(funName);
	root->children.push_back(funParmList);
	initNode(root, "FunCall", "INNER", look.line);

	list<treeNode*>::iterator it = parameters.begin();
	while (it != parameters.end()) {
		gen("param", (*it)->entry, 0, 0);
		it++;
	}
	tableEntry * e1 = new tableEntry(); // 函数名
	e1->word = fn;
	tableEntry * e2 = new tableEntry();  // 参数个数
	e2->word = to_string(n);

	// 转移活动记录
	cs.push(fi);
	current = cs.top();
	current->outAddr = nextinstr;

	gen("call", e1, e2, current->inAddr);
	// gen("j", 0, 0, fi->inAddr);

	// 返回值的处理
	return root;
}


treeNode* block() {

	match("LBRACE", look.line);
	struct env* saved = top;
	stack<int> left_brace;

	top = new env();
	top->prev = saved;

	left_brace.push(1);
	treeNode* root = new treeNode();
	treeNode* lastStmt = new treeNode();

	while (!left_brace.empty()) {

		if (look.type == "TYPE" || look.type == "COMPLEXTYPE") {
			root->children.push_back(decls(NULL));
		}
		else if (look.type == "STRUCT") {
			root->children.push_back(mystruct());
		}
		else if (look.type == "IF" || look.type == "WHILE" || look.type == "DO" || look.type == "ID" || look.type == "FOR" || look.type == "BREAK") {
			// L -> L1 M S
			treeNode * l = stmt(); // S
			root->nextlist = l->nextlist;
			root->children.push_back(l);
			backpatch(root->nextlist, nextinstr);
			// 下一条语句回填至 root->nextlist中
			root->breaklist = merge(root->breaklist, l->breaklist);
		}
		else if (look.type == "PRINTF" || look.type == "SCANF") {
			root->children.push_back(stmt());
		}
		else if (look.type == "RETURN") {
			match("RETURN", look.line);
			treeNode * l = mybool();
			tableEntry* temp = gen("RETURN", l->entry, NULL, 0);
			l->entry = temp;
			root->entry = temp;
			match("SEMICOLON", look.line);
		}
		else {
			match("RBRACE", look.line);
			left_brace.pop();
		}
	}
	initNode(root, "CompoundStatement", "INNER", look.line);

	top = saved;

	return root;
}

treeNode* halfassign(string type, lexicalUnit id, complexType* ctype, int num) {
	//ctype用于在结构体定义时为属性赋初值，将初值存入结构定义表中
	treeNode* property = NULL;
	struct tableEntry* newEntry = new tableEntry();
	// newEntry->type = type;
	newEntry->word = id.word;
	int l = look.line;

	if (look.type == "DOT") {
		match("DOT", look.line);
		treeNode* property = new treeNode();
		initNode(property, look.type, look.word, look.line);
		//此处可能需要添加gen，如果出现赋值语句的话需要添加gen，如果没有出现赋值语句则不需要添加gen
	}
	if (look.type == "ASSIGN") {
		match("ASSIGN", look.line);
		treeNode* root = new treeNode();
		treeNode* left = new treeNode();
		initNode(left, id.type, id.word, look.line);
		left->children.push_back(property);
		treeNode* right = mybool();
		root->children.push_back(left);
		root->children.push_back(right);
		initNode(root, "Assign", "=", look.line);


		if (type == "int"&&ctype == NULL) {
			newEntry->type = INT_T;
			newEntry->intValue = 0;
			newEntry->offset = memOffset + 32;
			memOffset += 32;
		}
		else if (type == "float"&&ctype == NULL) {
			newEntry->type = FLOAT_T;
			newEntry->floatValue = 0.0;
			newEntry->offset = memOffset + 32;
			memOffset += 32;
		}
		//有问题，如果在定义中给属性赋了复杂类型的值
		else if (ctype != NULL) {
			// pass A a A b; a=b;
			propertyNode* p = findProperty(ctype, id.word);
			if (p->type == "int") {
				p->intValue = getIntValue(right);
			}
			else if (p->type == "float") {
				p->floatValue = getFloatValue(right);
			}
			else {
				//如果在复杂类型内定义复杂类型
			}
		}
		// 判断有没有重复定义,如果有的话报错，没有的话向当前符号表中插入一个新的条目
		if (get(top, left->word)) {
			cout << left->word << " is redefined.\n";
			cout << "the error line number is " + to_string(l) << endl;
		}
		else if (ctype == NULL) {
			insert(top, left->word, *newEntry); //
		}
		return root;
	}
	else if (look.type == "COMMA" || look.type == "SEMICOLON") {
		treeNode* root = new treeNode();
		initNode(root, id.type, id.word, look.line);
		root->children.push_back(NULL);

		// 如果是数组类型
		if (num != 0) {
			if (num < 0) {
				// 报错数组的大小必须大于0
			}
			else {
				if (type == "int[]") {
					newEntry->type = INTARRAY_T;
					newEntry->offset = 8 * num;
					newEntry->arrayValue = new tableEntry[num];
					for (int i = 0; i < num; i++) {
						newEntry->arrayValue[i].type = INT_T;
						newEntry->arrayValue[i].offset = 8;
						newEntry->arrayValue[i].intValue = 0;
					}
				}
				else if (type == "float[]") {
					newEntry->type = FLAOTARRAY_T;
					newEntry->arrayValue = new tableEntry[num];
					newEntry->offset = 16 * num;
					for (int i = 0; i < num; i++) {
						newEntry->arrayValue[i].type = FLOAT_T;
						newEntry->arrayValue[i].offset = 16;
						newEntry->arrayValue[i].floatValue = 0.0;
					}
				}
				// 结构体的数组
				else {
					newEntry->type = STRUCTARRAY_T;
					newEntry->arrayValue = new tableEntry[num];
					newEntry->offset = getOffset(findStructName(type))*num;
					for (int i = 0; i < num; i++) {
						newEntry->arrayValue[i].type = STRUCT_T;
						newEntry->arrayValue[i].offset = getOffset(findStructName(type));
					}
				}
			}
		}
		else {
			if (type == "int") {
				newEntry->type = INT_T;
				newEntry->intValue = 0;
				newEntry->offset = 8;
			}
			else if (type == "float") {
				newEntry->type = FLOAT_T;
				newEntry->floatValue = 0.0;
				newEntry->offset = 16;
			}
			else {
				//对自定义结构体的声明
				newEntry->type = STRUCT_T;
				complexType* temp = new complexType();
				temp = findStructName(type);
				newEntry->structValue = temp;
				newEntry->offset = getOffset(temp);
			}
		}
		if (get(top, id.word)) {
			cout << id.word << " is redefined.\n";
			cout << "the error line number is " + l << endl;
		}
		else if (ctype == NULL) {
			insert(top, id.word, *newEntry); //
		}
		return root;
	}
	// 判断有没有重复定义,如果有的话报错，没有的话向当前符号表中插入一个新的条目
	
	//如果是复杂类型声明，要初始化complexType*

	
}


treeNode* decls(complexType* ctype) {
	treeNode* root = new treeNode();
	while (look.type == "TYPE" || look.type == "COMPLEXTYPE") {
		treeNode* subroot = new treeNode();
		lexicalUnit t = look;
		struct treeNode* currentType = mytype(t);
		subroot->children.push_back(currentType);
		//添加在声明结构体变量时把变量放入符号表
		lexicalUnit temp = look;
		match("ID", look.line);
		treeNode* r = new treeNode();
		//treeNode r有什么作用
		if (ctype != NULL) {
			propertyNode* pnode = new propertyNode(t.word, temp.word);
			ctype->property.push_back(pnode);
			r = halfassign(currentType->word, temp, ctype);
			subroot->children.push_back(r);
			match("SEMICOLON", look.line);
			continue;
		}
		
		// 如果定义数组
		if (look.type == "LBRACKET") {
			match("LBRACKET", look.line);
			treeNode * num = mybool();
			match("RBRACKET", look.line);
			// 添加Array新节点
			treeNode * my_array = new treeNode();
			initNode(my_array, "ARRAY", "INNER", look.line);
			my_array->children.push_back(num);
			string type; // 数组的类型
			if (currentType->word == "int") {
				type = "int[]";
			}
			else if (currentType->word == "float") {
				type = "float[]";
			}
			// 结构体
			else {
				type = currentType->word;
			}
			my_array->children.push_back(halfassign(type, temp, ctype, num->entry->intValue));
			subroot->children.push_back(my_array);
		}
		else {
			r = halfassign(currentType->word, temp, NULL);
			if (r != NULL)
				subroot->children.push_back(r);
		}

		while (look.type == "COMMA") {
			match("COMMA", look.line);
			if (look.type == "ID") {
				lexicalUnit temp = look;
				match("ID", look.line);
				r = halfassign(currentType->word, temp, NULL);
				if (r != NULL)
					subroot->children.push_back(r);
				//如果是在定义结构体的成员，将成员添加到结构体属性表中
				if (ctype != NULL) {
					propertyNode* pnode = new propertyNode(t.word, temp.word);
					ctype->property.push_back(pnode);
				}
				// 如果定义数组
				if (look.type == "LBRACKET") {
					match("LBRACKET", look.line);
					treeNode * num = mybool();
					match("RBRACKET", look.line);
					// 添加Array新节点
					treeNode * my_array = new treeNode();
					my_array->children.push_back(num);
					my_array->children.push_back(halfassign(currentType->word, temp, ctype));
					subroot->children.push_back(my_array);
				}
				else {
					subroot->children.push_back(halfassign(currentType->word, temp, ctype));
				}
			}
			else {
				error();
				return NULL;
			}
		}
		if (ctype != NULL) {
			structList.push_back(ctype);
		}
		initNode(subroot, "VarDeclaration", "INNER", look.line);
		root->children.push_back(subroot);
		match("SEMICOLON", look.line);
	}
	initNode(root, "VarDeclarationList", "INNER", look.line);
	return root;
}


treeNode* mytype(lexicalUnit t) {
	if (t.type != "COMPLEXTYPE") {
		match("TYPE", look.line);
	}
	else {
		match("COMPLEXTYPE", look.line);
	}
	treeNode* root = new treeNode();
	initNode(root, t.type, t.word, look.line);
	root->children.push_back(NULL);
	return root;
}

treeNode *N() {
	treeNode * root = new treeNode();
	root->nextlist = makelist(nextinstr);
	gen("j", 0, 0, 0);
	return root;
}

treeNode* stmt() {
	treeNode* root = new treeNode();
	if (look.type == "SEMICOLON") {
		move();
		root = NULL;
		return root;
	}
	if (look.type == "IF") {
		match("IF", look.line);
		match("LP", look.line);
		treeNode *b = mybool(); // B
		root->children.push_back(b);
		int instr1 = nextinstr; // M1.instr
		match("RP", look.line);
		treeNode *s1 = new treeNode(); // S1
									   // ƥ��û�л�����ֻ��һ��ִ�������� if �����л����ŵ�������
		if (look.type == "LBRACE") {
			s1 = block();
			root->children.push_back(s1);
		}
		else if (look.type == "ID" || look.type == "PRINTF" || look.type == "SCANF" || look.type == "BREAK") {
			s1 = stmt();
			root->children.push_back(s1);
		}
		if (look.type == "ELSE") {
			treeNode * n = N(); // N
			match("ELSE", look.line);
			int instr2 = nextinstr; // M2
			treeNode * s2 = new treeNode();
			if (look.type == "LBRACE") {
				s2 = block();
				root->children.push_back(s2);
			}
			else if (look.type == "ID" || look.type == "PRINTF" || look.type == "SCANF") {
				s2 = stmt();
				root->children.push_back(s2);
			}
			// if(B)M1 S1 N else M2 S2
			backpatch(b->truelist, instr1);
			backpatch(b->falselist, instr2);
			root->nextlist = merge(merge(s1->nextlist, s2->nextlist), n->nextlist);
			root->breaklist = merge(s1->breaklist, s2->breaklist);
		}
		else {
			// if(B)M S;
			backpatch(b->truelist, instr1);
			root->nextlist = merge(b->falselist, s1->nextlist);
			root->breaklist = s1->breaklist;
		}
		// 匹配没有花括号只有一条执行语句的 if 或者有花括号的语句块
		initNode(root, "IfStatement", "INNER", look.line);
		return root;
	}
	if (look.type == "FOR") {
		match("FOR", look.line);
		match("LP", look.line);
		treeNode * e1 = assign();// e1
		root->children.push_back(e1);
		int instr1 = nextinstr; // m1
		treeNode * e2 = mybool(); // e2
		root->children.push_back(e2);
		int instr2 = nextinstr; // m2
		match("SEMICOLON", look.line);
		treeNode * e3 = assign(1); // e3
		root->children.push_back(e3);
		match("RP", look.line);
		treeNode *n = N(); // n
		int n_instr = nextinstr; // n_instr
		treeNode *s = new treeNode(); // stmt
		if (look.type == "LBRACE") {
			s = block();
			root->children.push_back(s);
		}
		else if (look.type == "ID" || look.type == "PRINTF" || look.type == "SCANF") {
			s = stmt();
			root->children.push_back(s);
		}
		// for(e1; m1 e2; m2 e3) n stmt
		gen("j", NULL, NULL, instr2);
		backpatch(s->nextlist, instr2);
		backpatch(e2->truelist, n_instr);
		backpatch(n->nextlist, instr1);
		root->nextlist = e2->falselist;
		root->nextlist = merge(root->nextlist, s->breaklist);
		initNode(root, "ForStatement", "INNER", look.line);
		return root;
	}
	if (look.type == "WHILE") {
		match("WHILE", look.line);
		match("LP", look.line);
		int instr1 = nextinstr; // M1
		treeNode * b = mybool(); // B
		root->children.push_back(b);
		match("RP", look.line);
		// 匹配没有花括号只有一条执行语句的while 或者有花括号的语句块
		int instr2 = nextinstr; // M2
		treeNode *s = new treeNode(); // S
		if (look.type == "LBRACE") {
			s = block();
			root->children.push_back(s);
		}
		else if (look.type == "ID" || look.type == "PRINTF" || look.type == "SCANF") {
			s = stmt();
			root->children.push_back(s);
		}
		// 匹配没有花括号只有一条执行语句的while 或者有花括号的语句块
		initNode(root, "WhileStatement", "INNER", look.line);
		// while M1(B) M2 S
		backpatch(b->truelist, instr2);
		backpatch(s->nextlist, instr1);
		root->nextlist = b->falselist;
		root->nextlist = merge(root->nextlist, s->breaklist);
		gen("j", 0, 0, instr1);
		return root;
	}
	if (look.type == "DO") {
		match("DO", look.line);
		root->children.push_back(block());
		match("WHILE", look.line);
		match("LP", look.line);
		root->children.push_back(mybool());
		initNode(root, "DoWhileStatement", "INNER", look.line);
		match("RP", look.line);
		match("SEMICOLON", look.line);
		return root;
	}
	if (look.type == "BREAK") {
		match("BREAK", look.line); match("SEMICOLON", look.line);
		root->children.push_back(NULL);
		initNode(root, "Break", "INNER", look.line);
		root->breaklist = makelist(nextinstr);
		gen("j", 0, 0, 0);
		return root;
	}

	if (look.type == "RETURN") {
		match("RETURN", look.line);
		treeNode* t;
		if (look.type == "SEMICOLON") {
			t = NULL;
			if (current->returnType == VOID_T) {
				gen("return", 0, 0, current->outAddr);
			}
			else {
				// error
				cout << current->name << ": " << "must return a value;" << endl;
			}
			root->children.push_back(NULL);

		}
		else {
			t = mybool();
			root->children.push_back(t);
			// 检查返回值类型是否匹配
			// struct funcInfo* current = cs.top();
			if (current->returnType != t->entry->type) {
				string s1 = typeToString(current->returnType);
				string s2 = typeToString(t->entry->type);
				cout << "mismatch return type error in " << current->name << " : expected " << s1 << " but get " << s2 << endl;
			}

			gen("return", t->entry, 0, current->outAddr);
		}
		match("SEMICOLON", look.line);
		initNode(root, "ReturnStatement", "INNER", look.line);
		return root;
	}

	if (look.type == "LBRACE") {
		return block();
	}
	// 输入输出
	if (look.type == "PRINTF") {
		match("PRINTF", look.line);
		match("LP", look.line);
		root->children.push_back(factor());
		match("RP", look.line);
		match("SEMICOLON", look.line);
		root->children.push_back(NULL);
		initNode(root, "PrintStatement", "INNER", look.line);
		return root;
	}
	if (look.type == "SCANF") {
		match("SCANF", look.line);
		match("LP", look.line);
		root->children.push_back(factor());
		match("RP", look.line);
		match("SEMICOLON", look.line);
		root->children.push_back(NULL);
		initNode(root, "ScanStatement", "INNER", look.line);
		return root;
	}
	//输入输出
	// 函数调用
	struct funcInfo* fi = isFunction(look.word);
	if (fi != nullptr) {
		// root = funCall(fi);
		root = funCall(fi);
		match("SEMICOLON", look.line);
		return root;
	}
	// S->A
	root = assign();
	return root;
}

treeNode* mybool() {
	treeNode* root = new treeNode();
	treeNode *b1 = join(); // B1
	root->children.push_back(b1);
	if (look.type == "OR") {
		move();
		int instr = nextinstr; // M
		treeNode * b2 = mybool(); // B2
		root->children.push_back(b2);
		// b->b1||m b2
		backpatch(b1->falselist, instr);
		root->truelist = merge(b1->truelist, b2->truelist);
		root->falselist = b2->falselist;
	}
	else {
		root->children.push_back(NULL);
		// entry
		root->truelist = b1->truelist;
		root->falselist = b1->falselist;
		root->entry = b1->entry;
	}
	initNode(root, "MyBool", "OR", look.line);
	return root;
}

treeNode* assign(int for_right) {
	lexicalUnit id = look;
	treeNode* root = new treeNode();
	treeNode* left = new treeNode();
	treeNode * right = new treeNode();

	tableEntry * l = get(top, look.word); // ���߸�ֵ��ID
	left->entry = l;
	match("ID", look.line);
	// 如果是数组赋值 a[mybool()] = mybool();
	if (look.type == "LBRACKET") {
		match("LBRACKET", look.line);
		treeNode * index = mybool();
		match("RBRACKET", look.line);
		// 生成树的左节点
		treeNode * ll = new treeNode();
		initNode(ll, "ID", l->word, look.line);
		ll->entry = l;
		treeNode *lr = index;
		initNode(lr, "NUM", "NUM", look.line);
		left->children.push_back(ll);
		left->children.push_back(lr);
		left->entry = gen("[]", ll->entry, lr->entry, NULL);
	}

	if (look.type == "DOT") {
		match("DOT", look.line);
		if (look.type == "PROPERTY") {
			treeNode* property = new treeNode();
			initNode(property, look.type, look.word, look.line);
			propertyNode* p = new propertyNode(findProperty(l->structValue, look.word)->type, look.word);//就是这里structvalue为空值还要在声明处填上给类型声明插入符号表
			left->children.push_back(property);
			l = gen(l, p);
			left->entry = l;
		}
		match("PROPERTY", look.line);
	}
	else {
		left->children.push_back(NULL);
	}
	initNode(left, id.type, id.word, id.line);
	root->children.push_back(left);
	if (look.type == "ASSIGN") {
		match("ASSIGN", look.line);
		right = mybool(); // 右边赋值的表达式
		root->children.push_back(right);
	}
	if (!for_right) {
		match("SEMICOLON", look.line);
	}
	root->entry = gen("=", left->entry, right->entry, NULL);
	initNode(root, "AssignStatement", "=", look.line);
	return root;
}
treeNode* join() {
	treeNode* root = new treeNode();
	treeNode * b1 = equality(); // B1
	root->children.push_back(b1);
	if (look.type == "AND") {
		move();
		int instr = nextinstr; // M
		treeNode * b2 = join(); // B2
		root->children.push_back(b2);
		// b-> b1 && m b2
		backpatch(b1->truelist, instr);
		root->truelist = b2->truelist;
		root->falselist = merge(b1->falselist, b2->falselist);
	}
	else {
		root->children.push_back(NULL);
		// entry
		root->truelist = b1->truelist;
		root->falselist = b1->falselist;
		root->entry = b1->entry;
	}
	initNode(root, "Join", "AND", look.line);
	return root;
}
treeNode* equality() {
	treeNode* root = new treeNode();
	treeNode * e1 = rel(); // E1
	root->children.push_back(e1);
	if (look.type == "EQU") {
		move();
		treeNode * e2 = equality(); // E2
		root->children.push_back(e2);
		initNode(root, "Equality", "==", look.line);
		// B -> E1 relop E2
		root->truelist = makelist(nextinstr);
		root->falselist = makelist(nextinstr + 1);
		gen("j==", e1->entry, e2->entry, 0);
		gen("j", 0, 0, 0);
		return root;
	}
	else if (look.type == "NOTEQU") {
		move();
		treeNode * e2 = equality(); // E2
		root->children.push_back(e2);
		initNode(root, "UnEquality", "!=", look.line);
		// B -> E1 relop E2
		root->truelist = makelist(nextinstr);
		root->falselist = makelist(nextinstr + 1);
		gen("j!=", e1->entry, e2->entry, 0);
		// gen("j!=", get(top, e1->word), get(top, e2->word), 0);
		gen("j", 0, 0, 0);
		return root;
	}
	else {
		root->children.push_back(NULL);
		root->entry = e1->entry;
		root->truelist = e1->truelist;
		root->falselist = e1->falselist;
		initNode(root, "SingleEqual", "INNER", look.line);
		return root;
	}
}
treeNode* rel() {
	treeNode* root = new treeNode();
	treeNode * e1 = expr(NULL); // E1
	root->children.push_back(e1);
	if (look.type == "LESS") {
		match("LESS", look.line);
		treeNode * e2 = expr(NULL); // E2
		root->children.push_back(e2);
		initNode(root, "Rel", "<", look.line);
		// B -> E1 relop E2
		root->truelist = makelist(nextinstr);
		root->falselist = makelist(nextinstr + 1);
		gen("j<", e1->entry, e2->entry, NULL);
		gen("j", 0, 0, 0);
		return root;
	}
	if (look.type == "GREATER") {
		match("GREATER", look.line);
		treeNode * e2 = expr(NULL); // E2
		root->children.push_back(e2);
		initNode(root, "Rel", ">", look.line);
		// B -> E1 relop E2
		root->truelist = makelist(nextinstr);
		root->falselist = makelist(nextinstr + 1);
		gen("j>", e1->entry, e2->entry, NULL);
		gen("j", 0, 0, 0);
		return root;
	}
	if (look.type == "LESSEQU") {
		match("LESSEQU", look.line);
		treeNode * e2 = expr(NULL); // E2
		root->children.push_back(e2);
		initNode(root, "Rel", "<=", look.line);
		// B -> E1 relop E2
		root->truelist = makelist(nextinstr);
		root->falselist = makelist(nextinstr + 1);
		gen("j<=", e1->entry, e2->entry, NULL);
		gen("j", 0, 0, 0);
		return root;
	}
	if (look.type == "GREATEREQU") {
		match("GREATEREQU", look.line);
		treeNode * e2 = expr(NULL); // E2
		root->children.push_back(e2);
		initNode(root, "Rel", ">=", look.line);
		// B -> E1 relop E2
		root->truelist = makelist(nextinstr);
		root->falselist = makelist(nextinstr + 1);
		gen("j>=", e1->entry, e2->entry, NULL);
		gen("j", 0, 0, 0);
		return root;
	}
	root->entry = e1->entry;
	root->truelist = e1->truelist;
	root->falselist = e1->falselist;
	initNode(root, "Rel", "INNER", look.line);
	return root;
}
treeNode* expr(treeNode* subroot = NULL) {
	treeNode* root = new treeNode();
	treeNode * e1 = new treeNode();
	if (subroot == NULL) {
		e1 = term();
		root->children.push_back(e1);
	}
	else {
		e1 = subroot;
		root->children.push_back(subroot);
	}
	if (look.type == "PLUS") {
		move();
		treeNode * e2 = term();
		root->children.push_back(e2);
		root->entry = gen("+", e1->entry, e2->entry, NULL);
		initNode(root, "Addition", "+", look.line);
		return expr(root);
	}
	else if (look.type == "SUB") {
		move();
		treeNode * e2 = term();
		root->children.push_back(e2);
		root->entry = gen("-", e1->entry, e2->entry, NULL);
		initNode(root, "Substraction", "-", look.line);
		return expr(root);
	}
	else {
		if (subroot == NULL) {
			root->entry = e1->entry;
			initNode(root, "SingleExpr", "INNER", look.line);
			return root;
		}
		root->entry = subroot->entry;
		return subroot;
	}
}

treeNode* term() {
	treeNode* root = new treeNode();
	treeNode * l = unary(); // l
	root->children.push_back(l);
	if (look.type == "MUL") {
		move();
		treeNode * r = term(); // r
		root->children.push_back(r);
		root->entry = gen("*", l->entry, r->entry, NULL);
		initNode(root, "Multiplication", "*", look.line);
		return root;
	}
	else if (look.type == "DIVIDE") {
		move();
		treeNode * r = term(); // r
		root->children.push_back(r);
		root->entry = gen("/", l->entry, r->entry, NULL);
		initNode(root, "Divide", "/", look.line);
		return root;
	}
	else {
		root->children.push_back(NULL);
		root->entry = l->entry;
		initNode(root, "SingleTerm", "INNER", look.line);
		return root;
	}
}
treeNode* unary() {
	treeNode* root = new treeNode();
	string temp = look.type;
	if (look.type == "NOT" || look.type == "SUB") {
		move();
		treeNode* left = new treeNode();
		initNode(left, look.type, look.word, look.line);
		left->children.push_back(NULL);
		root->children.push_back(left);
		treeNode *b1 = unary(); // B1
		root->children.push_back(b1);
		// b->!b1
		if (temp == "NOT") {
			root->truelist = b1->falselist;
			root->falselist = b1->truelist;
			root->entry = gen("!", b1->entry, NULL, 0);
		}
		// b->-b1
		else if (temp == "SUB") {
			root->entry = gen("-", b1->entry, NULL, 0);
		}
	}
	else {
		// b->b1
		treeNode *b1 = factor(); // B1
		root->entry = b1->entry;
		root->children.push_back(b1);
	}
	if (temp == "Not") {
		initNode(root, "UnaryNot", "!", look.line);
	}
	else if (temp == "SUB") {
		initNode(root, "UnaryMinus", "-", look.line);
	}
	else {
		initNode(root, "UnaryCommon", "INNER", look.line);
	}
	return root;
}

treeNode* factor() {
	treeNode* root = new treeNode();
	if (look.type == "LP") {
		match("LP", look.line);
		treeNode * b = mybool(); // B1
		root->children.push_back(b);
		root->entry = b->entry;
		match("RP", look.line);
		initNode(root, "Factor", "(mybool)", look.line);
		// b->(B)
		root->truelist = b->truelist;
		root->falselist = b->falselist;
		return root;
	}
	else if (look.type == "ID" || look.type == "STRING") {
		struct funcInfo* fi = isFunction(look.word);
		if (fi != nullptr) {
			cs.push(fi);
			return funCall(fi);
		}
		lexicalUnit temp = look;
		move();
		// 数组
		// 如果是数组赋值 a[mybool()] = mybool();
		if (look.type == "LBRACKET") {
			match("LBRACKET", temp.line);
			treeNode * index = mybool();
			match("RBRACKET", temp.line);
			// 生成树的左节点
			treeNode * ll = new treeNode();
			initNode(ll, "ID", temp.word, temp.line);
			ll->entry = get(top, temp.word);
			treeNode *lr = index;
			initNode(lr, "NUM", "NUM", look.line);
			root->children.push_back(ll);
			root->children.push_back(lr);
			root->entry = gen("[]", ll->entry, lr->entry, NULL);
			initNode(root, "[]", "INNER", temp.line);
		}
		else {
			if (temp.type == "ID" && look.type == "DOT") {
				match("DOT", look.line);
				treeNode* property = new treeNode();
				initNode(property, "PROPERTY", look.word, look.line);
				root->children.push_back(property);
				complexType* structValue = get(top, temp.word)->structValue;
				propertyNode* p = new propertyNode(findProperty(structValue, property->word)->type, property->word);
				root->entry = gen(get(top, temp.word), p);
				move();
			}
			else {
				root->children.push_back(NULL);
				root->entry = get(top, temp.word);
			}
			initNode(root, temp.type, temp.word, temp.line);
		}

		if (root->entry == NULL) {
			cout << "identifier " << root->word << " is undefined." << endl;
		}
		return root;
	}
	else if (look.type == "NUM") {
		initNode(root, look.type, look.word, look.line);
		root->entry = new tableEntry();
		if ((root->word).find(".") != string::npos) {
			root->entry->floatValue = stof(root->word);
			root->entry->type = FLOAT_T;
			root->entry->word = root->word;
			root->entry->offset = memOffset + 32;
			memOffset += 32;
		}
		else {
			root->entry->intValue = stoi(root->word);
			root->entry->word = root->word;
			root->entry->type = INT_T;
			root->entry->offset = memOffset + 32;
			memOffset += 32;
		}
		root->children.push_back(NULL);
		move();
		return root;
	}
}

treeNode* memberlist(complexType* ctype) {
	treeNode* root = new treeNode();
	match("LBRACE", look.line);
	stack<int> left_brace;
	left_brace.push(1);
	while (!left_brace.empty()) {
		if (look.type == "TYPE") {
			root->children.push_back(decls(ctype));
		}
		else {
			match("RBRACE", look.line);
			left_brace.pop();
		}
	}
	initNode(root, "StructMemberList", "INNER", look.line);
	if (ctype != NULL) {
		structList.push_back(ctype);
	}
	return root;
}

treeNode* mystruct() {
	if (look.type == "STRUCT") {
		treeNode* root = new treeNode();
		treeNode* keyword = new treeNode();
		initNode(keyword, "TYPE", "STRUCT", look.line);
		keyword->children.push_back(NULL);
		root->children.push_back(keyword);
		move();
		lexicalUnit temp = look;
		match("COMPLEXTYPE", temp.line);
		treeNode* complextype = new treeNode();
		initNode(complextype, temp.type, temp.word, temp.line);
		complextype->children.push_back(NULL);
		root->children.push_back(complextype);
		complexType* ctype = new complexType();
		ctype->name = temp.word;
		root->children.push_back(memberlist(ctype));
		if (look.type == "ID") {
			treeNode* var = new treeNode();
			initNode(var, look.type, look.word, look.line);
			var->children.push_back(NULL);
			root->children.push_back(var);
			//将变量放入符号表
			match("ID", look.line);
		}
		match("SEMICOLON", look.line);
		initNode(root, "StructDeclaration", "INNER", look.line);
		return root;
	}
}

void display(treeNode* root) {
	if (root == NULL) {
		cout << "ops" << endl;
		return;
	}
	list<treeNode*>::iterator i = root->children.begin();
	while (i != root->children.end() && (*i) != NULL) {
		display(*i);
		i++;
	}
	cout << left << setw(3) << root->id << ":  " << setw(25) << root->type + "," << setw(25) << root->word + ","; cout << setw(12) << "Children: ";
	list<treeNode*>::iterator j = root->children.begin();
	while (j != root->children.end() && (*j) != NULL) {
		cout << (*j)->id << ", ";
		j++;
	}
	cout << endl;
}

void print_addr() {
	for (int i = 1; i < nextinstr; i++) {
		cout << i << "(" << addr[i].op;
		if (addr[i].arg1) {
			cout << "," << addr[i].arg1->word;
		}
		else {
			cout << ",null";
		}
		if (addr[i].arg2) {
			cout << "," << addr[i].arg2->word;
		}
		else {
			cout << ",null";
		}
		if (addr[i].result) {
			cout << "," << addr[i].result->word;
		}
		else if (addr[i].go_to) {
			cout << "," << addr[i].go_to;
		}
		else {
			cout << ",null";
		}
		cout << ")" << endl;
	}
}

int main()
{
	lexicalAnalyzer la;
	la.handle();
	lst = la.lst;

	SymbolTable st = la.st;
	lexicalUnit end;
	end.type = "$";
	lst.push_back(end);

	p = lst.begin();
	look = *p;

	treeNode* root = program();
	error_judge();

	print_addr();
	cout << "end" << endl << endl;
	//toAssembly();
	// display(root);
	system("pause");
}
