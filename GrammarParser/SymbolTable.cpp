#include "SymbolTable.h"
#include<string>
#include<iostream>
using namespace std;

Node::Node() {
	next = NULL;
}

Node::Node(string id, string scope, string type) {
	this->identifier = id;
	this->scope = scope;
	this->type = type;
	this->next = NULL;
}

SymbolTable::SymbolTable() {
	for (int i = 0; i < MAX; i++) {
		head[i] = NULL;
	}
}

int SymbolTable::hashf(string id) {
	int asciiSum = 0;

	for (int i = 0; i < id.length(); i++) {
		asciiSum += id[i];
	}

	return (asciiSum % MAX);
}

bool SymbolTable::insert(string id, string scope, string type) {
	int index = hashf(id);
	Node* p = new Node(id, scope, type);
	if (head[index] == NULL) {
		head[index] = p;
		return true;
	}
	else {
		Node* start = head[index];
		while (start->next != NULL) {
			start = start->next;
		}
		start->next = p;
		return true;
	}
	return false;
}

Node* SymbolTable::find(string id) {
	int index = hashf(id);
	Node* p = head[index];
	while ((p != NULL) && (p->identifier != id)) {
		p = p->next;
	}
	return p;
}

bool SymbolTable::modify(string id, string scope, string type) {
	int index = hashf(id);
	Node* p = head[index];
	while (p != NULL) {
		if (p->identifier == id) {
			p->scope = scope;
			p->type = type;
			return true;
		}
		p = p->next;
	}
	return false;
}

bool SymbolTable::deleteRecord(string id) {
	int index = hashf(id);
	Node* temp = head[index];
	Node* par = temp;
	while (temp != NULL) {
		if (temp->identifier == id) {
			break;
		}
		par = temp;
		temp = temp->next;
	}
	if (temp == NULL) { // id is NULL or id doesn't exist in the table
		return false;
	}
	else if (par == temp) {
		head[index] = temp->next;
		delete temp;
		return true;
	}
	else {
		par->next = temp->next;
		delete temp;
		return true;
	}
	return false;
}


