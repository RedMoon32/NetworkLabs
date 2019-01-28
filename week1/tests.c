#include "stack.h"
#include <stdio.h>
#include <assert.h>

struct stack* st = NULL;

void test1(){
	push(st,2);
	push(st,3);
	push(st,4);
	assert(st->size==3);
	pop(st);
	pop(st);
	pop(st);
	printf("Test 1 succeed \n");
}

void test2(){
	push(st,2);
	push(st,3);
	push(st,4);
	assert(peek(st)==4);
	pop(st);
	assert(peek(st)==3);
	pop(st);
	assert(peek(st)==2);
	pop(st);
	assert(st->size==0);
	assert(empty(st));
	printf("Test 2 succeed \n");
}

void test3(){
	push(st,2);
	push(st,3);
	push(st,4);
	display(st);
	stack_size(st);
}


int main(){
	st = create();
	test1();	
	test2();
	test3();

}