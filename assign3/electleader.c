/*
   Michael Wu - 711630931

   An implementation of a bidirectional ring leader algorithm using the Hirschberg-Sinclair election algorithm. The algorithm produces a leader within O(logn) rounds with n candidates.

Pseudocode : 

- assume ring is bidirectional
- we have rounds of elections where candidates are eliminated conditionally
- only candidates that win proceed to next round
- p_i is a leader in round r iff it has the largest pid of all nodes within a distance of 2^r or less from p_i

And so at every round we are guauranteed that 1/2 of the candidates are being eliminated.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void beginElection();

int PNUM; // random prime number to shake process ids up
int rank, size, pid; // mpi rank, total size, and this process's id, computed with our prime number thing
int round_counter; // the current round
int messages_sent; // how many this node has sent
int messages_received; // how many messages this node has received
int leader=1; // am i currently in the running for a leader? - initially, i am

#define LEADERTAG 10

int main(int argc, char** argv){

    MPI_Init( &argc, &argv ); 

    if (argc < 2){
        printf("Usage : electleader <num processes initialized with mpiexec.\n");
        return EXIT_SUCCESS;
    }

    PNUM = atoi(argv[1]);

    MPI_Comm_rank( MPI_COMM_WORLD, &rank ); 
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    pid = (rank + 1) * PNUM  % size;

    beginElection();

    MPI_Finalize();
    return EXIT_SUCCESS;
}

int getNextNeighbor(int rank, int offset){
    if(rank + offset < 0){
        return size-1;
    }else return (int) (rank+offset)%size;
}

void sendMessage(int left, int right){
        printf("Rank %d pid %d : Sending message to %d and %d using offset %d..\n", 
                rank, pid, right, left, (int) pow(2,round_counter));

        if (right!=rank && left!=right) MPI_Send(&pid, 1, MPI_INT, right, LEADERTAG, MPI_COMM_WORLD); 
        if (left!=rank) MPI_Send(&pid, 1, MPI_INT, left, LEADERTAG, MPI_COMM_WORLD);

        messages_sent += 2;
}

void beginElection(){
    while(pow(2,round_counter)<=size && leader){
        int right = getNextNeighbor(rank, pow(2,round_counter));
        int left = getNextNeighbor(rank, -pow(2,round_counter));

        int right_pid = (right+1) * PNUM % size;
        int left_pid = (left+1) * PNUM % size;

        printf("Rank %d pid %d : right rank %d, pid %d left rank  %d pid %d.\n", rank, pid, right, right_pid, left ,left_pid);

        if(pid >= right_pid && pid >= left_pid){
            // we know this guy is definitely a leader for this round, send the leader msg    
            sendMessage(left,right);
        }else{
            // you are not a leader, so you should recv the msg, and then leave the running
            int in;
            MPI_Request recv_s1;
            printf("Rank %d pid %d : Waiting for message..\n", rank, pid); 
            MPI_Irecv(&in, 1, MPI_INT, MPI_ANY_SOURCE, LEADERTAG, MPI_COMM_WORLD, &recv_s1);
            
            messages_received += 1;

            printf("Rank %d pid %d : Dropped out of the running.\n", rank, pid);
            leader = 0;
            break;
        }
        round_counter++;
        printf("Rank %d pid %d : The round is now %d, leader status %d, size is %d.\n", 
            rank, pid, round_counter, leader, size);
    }
    if (leader) {
        printf("Rank %d pid %d : I'm the leader!\n", rank, pid);
    }
}
