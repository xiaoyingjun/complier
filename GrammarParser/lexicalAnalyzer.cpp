#include<iostream>
#include<iomanip>
#include<fstream>
#include<string>
#include<list>
#include "SymbolTable.h"
#include"lexicalAnalyzer.h"
#include<algorithm>
using namespace std;


string type[] = { "char", "double", "float", "int", "long", "short", "void", "int[]", "float[]" };

string generfunc[] = { "printf", "scanf" };

string keyWord[] =
{ "auto","break","case","char","const","continue",
"default","do","double","else","enum","extern",
"float","for","goto","if","int","long","return",
"short","signed","sizeof","static","struct","switch",
"typedef","unsigned","void","while","catch","class",
"const_cast","delete","friend","inline","new","operator",
"private","protected","public","template","this",
"throw","try","virtual" };

string sepName[SYMBOLNUM] = {
	"plus", "sub", "mul", "divide", "lbrace", "rbrace", "lp", "rp",
	"colon", "comma", "semicolon", "remainder", "power",
	"assign", "less", "greater", "not", "and", "or",
	"equ", "lessequ", "greaterequ", "notequ", "lbracket", "rbracket"
};

string sepSymbol[SYMBOLNUM] = {
	"+", "-", "*", "/", "{", "}", "(", ")",
	":", ",", ";", "%", "^",
	"=", "<", ">", "!", "&&", "||",
	"==", "<=", ">=", "!=", "[", "]"
};

list<string> complexType;
list<string> ::iterator i;

bool lexicalAnalyzer::isType(string str) {
	for (int i = 0; i < sizeof(type) / sizeof(type[0]); i++) {
		if (str == type[i]) {
			return true;
		}
	}
	return false;
}

//print
bool lexicalAnalyzer::isGenerfunc(string func) {
	for (int i = 0; i < sizeof(func) / sizeof(func[0]); i++) {
		if (func == generfunc[i]) {
			return true;
		}
	}
	return false;
}
//print

bool lexicalAnalyzer::isKeyWord(string str) {
	for (int i = 0; i < sizeof(keyWord) / sizeof(keyWord[0]); i++) {
		if (str == keyWord[i]) {
			return true;
		}
	}
	return false;
}


bool lexicalAnalyzer::isBC(char c) {
	if (c == ' ' || c == '\n' || c == '\t') {
		return true;
	}
	else {
		return false;
	}
}

bool lexicalAnalyzer::isChar(char c) {
	if ((c >= 'a'&&c <= 'z') || (c >= 'A'&&c <= 'Z') || c == '_') {
		return true;
	}
	else {
		return false;
	}
}

bool lexicalAnalyzer::isNum(char c) {
	if (c >= '0'&&c <= '9') {
		return true;
	}
	else {
		return false;
	}
}

bool lexicalAnalyzer::isComplexType(string s) {
	//遍历list
	for (i = complexType.begin(); i != complexType.end(); i++) {
		if (*i == s) {
			return true;
		}
	}
	return false;
}

bool lexicalAnalyzer::singleSep(char c) {
	if (c == '+' || c == '-' || c == '*' ||
		c == '{' || c == '}' || c == '(' ||
		c == ')' || c == ':' || c == ',' ||
		c == ';' || c == '%' || c == '^' ||
		c == '[' || c == ']') {
		return true;
	}
	else {
		return false;
	}
}

bool lexicalAnalyzer::doubleSep(char c) {
	if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|') {
		return true;
	}
	else {
		return false;
	}
}

string lexicalAnalyzer::getSepName(string ch)
{
	for (int i = 0; i < SYMBOLNUM; i++)
	{
		if (sepSymbol[i] == ch)
		{
			string s = sepName[i];
			transform(s.begin(), s.end(), s.begin(), ::toupper);
			return s;
		}
	}
}

bool lexicalAnalyzer::isFailed(ifstream& in, lexicalUnit& temp, char& ch, string str) {
	if (isBC(ch) || singleSep(ch) || doubleSep(ch)) {
		temp.type = "NUM";
		temp.word = str;
		temp.property = str;
		return false;
	}
	return true;
}

void lexicalAnalyzer::handleFail(ifstream& in, lexicalUnit& temp, char& ch, string str) {
	while (!(isBC(ch) || singleSep(ch) || doubleSep(ch))) {
		str += ch;
		in.get(ch);
	}
	temp.type = "UNKNOWN";
	temp.word = str;
	temp.property = str;
}

int CountLines(const char *filename)
{
	ifstream ReadFile;
	int n = 0;
	char line[512];
	string temp;
	ReadFile.open(filename, ios::in);//ios::in 表示以只读的方式读取文件
	if (ReadFile.fail())//文件打开失败:返回0
	{
		return 0;
	}
	else//文件存在
	{
		while (getline(ReadFile, temp))
		{
			n++;
		}
		return n;
	}
	ReadFile.close();
}

void lexicalAnalyzer::handle()
{
	ifstream infile("D://input.txt");
	//cout << "本程序的行数为: " << CountLines("d://input.txt") << endl;
	char ch;
	char s[150];
	int count = 1;
	bool openNext = false;
	string lasttype;
	string str;
	infile.get(ch);
	while (!infile.eof())
	{
		str = "";

		lexicalUnit temp;
		if (ch == '\n') {
			count++;
		}
		if (!isBC(ch)) {
			if (isChar(ch)) {
				str += ch;
				infile.get(ch);
				while (isChar(ch) || isNum(ch)) {
					str += ch;
					infile.get(ch);
				}
				if (openNext == true) {
					complexType.push_back(str);
					temp.type = "COMPLEXTYPE";
					temp.word = str;
					temp.property = "";
					temp.line = count;
					openNext = false;
				}
				else if (isKeyWord(str)) {
					temp.type = str;
					transform(temp.type.begin(), temp.type.end(), temp.type.begin(), ::toupper);
					if (str == "struct") {
						openNext = true;
						temp.type = "STRUCT";
					}
					if (isType(str)) {
						temp.type = "TYPE";
					}
					temp.word = str;
					temp.property = "";
					temp.line = count;
				}
				else if (isComplexType(str)) {
					temp.type = "COMPLEXTYPE";
					temp.word = str;
					temp.property = "";
					temp.line = count;
				}
				else if (isGenerfunc(str)) {
					temp.type = str;
					transform(temp.type.begin(), temp.type.end(), temp.type.begin(), ::toupper);
					temp.word = str;
					temp.property = "";
					temp.line = count;
				}
				else {
					temp.type = "ID";
					temp.word = str;
					temp.property = str;
					temp.line = count;
				}
			}
			else if (isNum(ch)) {
				str += ch;
				infile.get(ch);
				while (isNum(ch)) {
					str += ch;
					infile.get(ch);
				}
				if (ch == '.') { // float
					str += ch;
					infile.get(ch);
					if (isNum(ch)) {
						str += ch;
						infile.get(ch);
						while (isNum(ch)) {
							str += ch;
							infile.get(ch);
						}
						if (ch == 'E') {
							str += ch;
							infile.get(ch);
							if (ch == '+' || ch == '-') {
								str += ch;
								infile.get(ch);
								if (isNum(ch)) {
									while (isNum(ch)) {
										str += ch;
										infile.get(ch);
									}
									if (isFailed(infile, temp, ch, str)) {
										handleFail(infile, temp, ch, str);
									}
								}
								else {
									handleFail(infile, temp, ch, str);
								}
							}
							else if (isNum(ch)) {
								str += ch;
								infile.get(ch);
								while (isNum(ch)) {
									str += ch;
									infile.get(ch);
								}
								if (isFailed(infile, temp, ch, str)) {
									handleFail(infile, temp, ch, str);
								}
							}
							else {
								handleFail(infile, temp, ch, str);
							}

						}
						else {
							if (isFailed(infile, temp, ch, str)) {
								handleFail(infile, temp, ch, str);
							}
						}
					}
					else {
						handleFail(infile, temp, ch, str);
					}
				}
				else if (ch == 'E') { // scientific notation
					str += ch;
					infile.get(ch);
					if (ch == '+' || ch == '-') {
						str += ch;
						infile.get(ch);
						if (isNum(ch)) {
							while (isNum(ch)) {
								str += ch;
								infile.get(ch);
							}
							if (isFailed(infile, temp, ch, str)) {
								handleFail(infile, temp, ch, str);
							}
						}
						else {
							handleFail(infile, temp, ch, str);
						}
					}
					else if (isNum(ch)) {
						str += ch;
						infile.get(ch);
						while (isNum(ch)) {
							str += ch;
							infile.get(ch);
						}
						if (isFailed(infile, temp, ch, str)) {
							handleFail(infile, temp, ch, str);
						}
					}
					else {
						handleFail(infile, temp, ch, str);
					}
				}
				else // integer
				{
					if (isFailed(infile, temp, ch, str)) {
						;
						handleFail(infile, temp, ch, str);
					}
				}
			}
			else if (singleSep(ch)) {
				str += ch;
				temp.type = getSepName(str);
				temp.word = ch;
				temp.property = "";
				temp.line = count;
				infile.get(ch);
			}
			else if (doubleSep(ch)) {
				str += ch;
				char fch = ch;
				infile.get(ch);
				if (fch == '&') {
					if (ch == '&') {
						str += ch;
						temp.type = getSepName(str);
						temp.word = str;
						temp.property = "";
						infile.get(ch);
					}
					else
					{
						temp.type = "UNKNOWN";
						temp.word = str;
						temp.property = str;
						temp.line = count;
					}
				}
				else if (fch == '|') {
					if (ch == '|') {
						str += ch;
						temp.type = getSepName(str);
						temp.word = str;
						temp.property = "";
						temp.line = count;
						infile.get(ch);
					}
					else
					{
						temp.type = "UNKNOWN";
						temp.word = str;
						temp.property = str;
						temp.line = count;
					}
				}

				else if (ch == '=') {
					str += ch;
					temp.type = getSepName(str);
					temp.word = str;
					temp.property = "";
					temp.line = count;
					infile.get(ch);
				}

				else {
					temp.type = getSepName(str);
					temp.word = str;
					temp.property = "";
					temp.line = count;
				}
			}
			else if (ch == '/') {
				infile.get(ch);
				str += ch;//
				if (ch == '*') {
					char former;
					infile.get(former);
					infile.get(ch);
					while (!(former == '*' && ch == '/')) {
						former = ch;
						infile.get(ch);
					}
					infile.get(ch);
					continue;
				}
				else {
					temp.type = getSepName(str);
					temp.word = '/';
					temp.property = "";
					temp.line = count;
				}
			}
			else if (ch == '"') {
				infile.get(ch);
				if (ch == '%') {
					infile.get(ch);
					str += ch;
					temp.type = "FORMATSTRING";
					temp.word = str;
					temp.property = "";
					temp.line = count;
				}
				else {
					while (ch != '"')
					{
						str += ch;
						infile.get(ch);
					}
					temp.type = "STRING";
					temp.word = str;
					temp.property = "";
					temp.line = count;
					infile.get(ch);
				}
			}
			else if (ch == '.') {
				if (lasttype == "ID") {
					str += ch;
					temp.type = "DOT";
					temp.word = str;
					temp.line = count;
					temp.property = "";
					lasttype = temp.type;
					lst.push_back(temp);
					st.insert(temp.word, "local", temp.type);
					str = "";
					infile.get(ch);
					if (isChar(ch)) {
						while (isChar(ch) || isNum(ch)) {
							str += ch;
							infile.get(ch);
						}
						temp.type = "PROPERTY";
						temp.word = str;
						temp.property = "";
						temp.line = count;
					}
					else {
						handleFail(infile, temp, ch, str);
					}
				}
				else {
					handleFail(infile, temp, ch, str);
				}
			}
			else {
				handleFail(infile, temp, ch, str);
			}
			lasttype = temp.type;
			lst.push_back(temp);
			st.insert(temp.word, "local", temp.type);
		}
		else {
			infile.get(ch);
		}

	}
	/*list<lexicalUnit>::iterator p = lst.begin();
	cout << left << setw(15) << "单词"
	<< setw(15) << "词素"
	<< setw(15)<<"行数"
	<< setw(15) << "属性" << endl;
	for (; p != lst.end(); p++)
	{
	cout << left << setw(15) << (*p).type
	<< setw(15) << (*p).word
	<< setw(15)
	<<(*p).line<<setw(15)
	;
	if ((*p).type == "ID") {
	Node* t = st.find((*p).word);
	cout << t << endl;
	}
	else {

	cout << (*p).property << endl;
	}

	}*/
	infile.close();
}
