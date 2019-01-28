#include <stdio.h>
struct stack {

	struct Node* head;
	int size;
};

struct Node {

	int value;
	struct Node* next;

};
extern struct stack* st;
int peek();
void push(int data);
void pop();
int empty();
void display();
void create();
void stack_size();