#pragma once
#include<string>
#define MAX 200

class Node {
	std::string identifier;
	std::string scope;
	std::string type;
	Node* next;

public:
	Node();
	Node(std::string key, std::string value, std::string type);
	friend class SymbolTable;
};

class SymbolTable {
	Node* head[MAX];

public:
	SymbolTable();
	int hashf(std::string id); // hash function
	bool insert(std::string id, std::string scope, std::string type);
	Node* find(std::string id);
	bool modify(std::string id, std::string scope, std::string type);
	bool deleteRecord(std::string id);
};