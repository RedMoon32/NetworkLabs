#include "stack.h"
#include <stdio.h>
#include <assert.h>

void test1(){
	push(2);
	push(3);
	push(4);
	assert(st->size==3);
	pop();
	pop();
	pop();
	printf("Test 1 succeed \n");
}

void test2(){
	push(2);
	push(3);
	push(4);
	assert(peek()==4);
	pop();
	assert(peek()==3);
	pop();
	assert(peek()==2);
	pop();
	assert(st->size==0);
	assert(empty());
	printf("Test 2 succeed \n");
}

void test3(){
	push(2);
	push(3);
	push(4);
	display();
	stack_size();
}


int main(){
	create();
	test1();	
	test2();
	test3();

}