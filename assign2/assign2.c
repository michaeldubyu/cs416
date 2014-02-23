#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

int PROCESSES; // number of processes
int TTR; // time to run
time_t rawtime;
struct tm *timeinfo;

pthread_mutex_t enqlock;
pthread_mutex_t deqlock;

typedef struct queue_operation {
	int operation; //1 to insert an item,0 to remove
	int value; //value should be ignored if operation = 0 
} operation_t;

typedef struct node {
	operation_t* op;
	node* next;
} Node;

typedef struct generator_state { 
	int state[4]; //for internal use 
} generator_state_t;

int load_generator(operation_t *op, generator_state_t **s) {
	if ( *s == 0 ) {
		*s = (generator_state_t *) malloc(sizeof(generator_state_t)); // initialize state
	}
	// set value for op­>operation and op­>value
	srand(time(0));
	op->operation = rand() % 2;
	op->value = rand();
	return 1; // 1 success ­­ return 0 for failure 
}

Node* head = 0;
Node* tail = 0;

void enqueue(operation_t* n){
	pthread_mutex_lock(&enqlock);
	if(head==0){
		tail = (Node*) malloc(sizeof(Node));
		tail->next = 0;
		tail->op = n;
		head = tail;
	}else{
		Node* temp = (Node*) malloc(sizeof(Node));
		temp->op = n;
		temp->next = tail->next;
		tail->next = temp;
		tail = temp;
	}		
	pthread_mutex_unlock(&enqlock);
}

operation_t* dequeue(){
	pthread_mutex_lock(&deqlock);
	if(head==0){
		return 0;
	}else{
		operation_t* n = head->op;
		head = head->next;
		return n;
	}
	pthread_mutex_unlock(&deqlock);
}

int main(int argc, char** args){
	if(argc < 3){
		printf("Need arguments to proceed! Exiting!\n");
		return 0;
	}else{
		PROCESSES = atoi(args[1]);
		TTR = atoi(args[2]);

		if(PROCESSES<1 || PROCESSES>100){
			printf("Invalid process count! Exiting!\n");
			return 0;
		}
		if(TTR<1 || TTR>100){	
			printf("Invalid time to run! Exiting!\n");
			return 0;
		}	
	}

	pthread_mutex_init(&enqlock,0);
	pthread_mutex_init(&deqlock,0);

}

void* work(){
	operation_t myops;
	generator_state_t *state = 0;

	while ( load_generator(&myops,&state) ){ }
}
