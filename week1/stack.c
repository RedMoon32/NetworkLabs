#include "stack.h"
#include <stdlib.h>
#include <stdio.h>

struct stack* st = NULL;

int peek(){
	if (st!=NULL && st->size>0){
		return st->head->value;	
	}
	else{
		printf("!!!Stack is not created\n");
	}
	return NULL;
}

void pop(){
	if (st!=NULL && st->size>0){
		st->size-=1;
		struct Node* old = st->head;
		int v = st->head->value;
		free(st->head);
		st->head = st->head->next;
	}
	else{
		printf("!!!Stack is not created\n");
	}
}

void push(int data){
	if (st!=NULL){
		struct Node* new = malloc(sizeof(struct Node));
		new->value = data;
		if (st->size == 0){
			st->head = new;
		}
		else{
			struct Node* old = st->head;
			st->head = new;
			new->next = old;
		}
		st->size+=1;
	}
	else{
		printf("!!!Stack is not created\n");
	}
}

void create(){
	if (st != NULL){
		free(st);
	}
	st = (struct stack *) malloc(sizeof(struct stack));
	
}

int empty(){
	if (st!=NULL){
		return st->size==0;
	}
	else{
		printf("!!!Stack is not created\n");
	}
	return 1;
}

void display(){
	if (st!=NULL){
		struct Node* prev = st->head;
		while (prev!=NULL){
			printf("%d ",prev->value);
			prev = prev->next;
		}
	}
	else{
		printf("!!!Stack is not created\n");
	}
	printf("\n");
}

void stack_size(){
	printf("Stack size :%d\n",st->size);
}

