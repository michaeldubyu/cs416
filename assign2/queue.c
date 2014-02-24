#include <stdlib.h>
#include <stdio.h>

// a queue in c

typedef struct node{
	struct node* next;
	int data;
} Node;

Node* head = 0;
Node* tail = 0; 

void enqueue(int data){
	if (head==0){
		Node* temp = (Node*)malloc(sizeof(Node));
		temp->data = data;
		tail = temp;
		head = temp;
	}else{
		Node* temp = (Node*)malloc(sizeof(Node));
		temp->data = data;
		tail->next = temp;
		tail = temp;	
	}
}

int dequeue(){
	if(head==0){
		return -1;
	}else{
		Node *temp = head;
		head = head->next;
		return temp->data;
	}
}

int main(){
	enqueue(0);
	enqueue(1);
	enqueue(2);
	enqueue(3);

	for (int i=0;i<4;i++){
		int d = dequeue();
		printf("value is %d\n", d);
	}
}
