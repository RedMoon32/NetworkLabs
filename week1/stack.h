#include <stdio.h>
struct stack {

	struct Node* head;
	int size;
};

struct Node {

	int value;
	struct Node* next;

};

int peek(struct stack* st);
void push(struct stack* st,int data);
void pop(struct stack* st);
int empty(struct stack* st);
void display(struct stack* st);
struct stack*  create();
void stack_size(struct stack* st);