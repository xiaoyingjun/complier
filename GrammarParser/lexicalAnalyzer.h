#pragma once
#include<string>
#include<list>
#include "SymbolTable.h"
#define IDNUM 1000
#define SYMBOLNUM 25
using namespace std;

struct lexicalUnit {
	string type;
	string word;
	string property;
	int line;
};

class lexicalAnalyzer {
public:
	SymbolTable st;
	list<lexicalUnit> lst;
	bool isType(string str);
	bool isGenerfunc(string func);
	bool isKeyWord(string str);
	bool isBC(char c);
	bool isChar(char c);
	bool isNum(char c);
	bool isComplexType(string s);
	bool singleSep(char c);
	bool doubleSep(char c);
	string getSepName(string ch);
	bool isFailed(ifstream& in, lexicalUnit& temp, char& ch, string str);
	void handleFail(ifstream& in, lexicalUnit& temp, char& ch, string str);
	void handle();
};