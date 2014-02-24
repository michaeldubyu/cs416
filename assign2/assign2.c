#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

void* work(void* p); // some forward declaring

unsigned int OP_COUNT = 0;
int PROCESSES; // number of processes
int NUM_PROCESSES=0; // how many processes we've fired up
int TTR; // time to run

pthread_mutex_t qlock;
pthread_mutex_t countlock;

typedef struct queue_operation {
	int operation; //1 to insert an item,0 to remove
	int value; //value should be ignored if operation = 0 
} operation_t;

typedef struct node {
	int value;
	struct node* next;
} Node;

typedef struct generator_state { 
	int state[4]; //for internal use 
} generator_state_t;

Node* head = 0;
Node* tail = 0;

void enqueue(int n){
    printf("thread before inside q mutex\n");
	pthread_mutex_lock(&qlock);
    printf("thread inside q mutex\n");
	if (head==0){
		Node* temp = (Node*)malloc(sizeof(Node));
		temp->value = n;
		tail = temp;
		head = temp;
	}else{
		Node* temp = (Node*)malloc(sizeof(Node));
		temp->value = n;
		tail->next = temp;
		tail = temp;	
	}
	pthread_mutex_unlock(&qlock);
    printf("thread outside q mutex\n");
}

int dequeue(){
    printf("thread before inside dq mutex\n");
	pthread_mutex_lock(&qlock);
    printf("thread inside dq mutex\n");
	if(head==0){
	    pthread_mutex_unlock(&qlock);
		return 0;
	}else{
		Node* temp = head;
        int tval = temp->value;
		head = head->next;
	    pthread_mutex_unlock(&qlock);
		return tval;
	}
}

int main(int argc, char** args){
	if(argc < 5){
		printf("Usage : process -p <1-100> -t <1-300>.\n");
		return 0;
	}else{
		PROCESSES = atoi(args[2]);
		TTR = atoi(args[4]);

		if(PROCESSES<1 || PROCESSES>100){
			printf("Invalid process count! Exiting!\n");
			return 0;
		}
		if(TTR<1 || TTR>300){	
			printf("Invalid time to run! Exiting!\n");
			return 0;
		}	
	}

	srand(time(0));
	pthread_mutex_init(&qlock,0);
	pthread_mutex_init(&countlock,0);

	while( NUM_PROCESSES < PROCESSES){	// while we still haven't reached the end of time to run
		pthread_t thread;	
		pthread_create(&thread, 0, &work, 0);
		NUM_PROCESSES++;
	}
	unsigned int startTime = (unsigned) time(0);
	while((unsigned) time(0) < startTime + (TTR)){}
	FILE *f = fopen("results.txt", "a");
	fprintf(f, "# of threads=%d time (seconds)=%d total number of operations=%d\n", PROCESSES, TTR, OP_COUNT);
	return 1;
}

int load_generator(operation_t *op, generator_state_t **s) {
	if ( *s == 0 ) {
		*s = (generator_state_t *) malloc(sizeof(generator_state_t)); // initialize state
	}
	// set value for op­>operation and op­>value
	op->operation = rand() % 2;
	op->value = rand();
    printf("about to return success in lg\n");
	return 1; // 1 success ­­ return 0 for failure 
}

void* work(void* p){
	operation_t myops;
	generator_state_t *state = 0;

	while ( load_generator(&myops,&state) ){
        printf("inside lg\n");
		if(myops.operation==0){
            printf("about to dq\n");
			dequeue();
            printf("dq'd\n");
		}else{
            printf("about to q\n");
			enqueue(myops.value);
            printf("q'd\n");
		}
        printf("before count lock\n");
		pthread_mutex_lock(&countlock);
        printf("thread inside count lock\n");
		OP_COUNT++;
		pthread_mutex_unlock(&countlock);
        printf("thread outside count lock\n");
	}
    printf("got outside load generator\n");
}
