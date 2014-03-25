/*
   Michael Wu - 711630931

   An implementation of a bidirectional ring leader algorithm using the Hirschberg-Sinclair election algorithm. The algorithm produces a leader within O(logn) rounds with n candidates.

Pseudocode : 

- assume ring is bidirectional
- we have rounds of elections where candidates are eliminated conditionally
- only candidates that win proceed to next round
- p_i is a leader in round r iff it has the largest pid of all nodes within a distance of 2^r or less from p_i
- messages are passed on only if the pid of the received msg is larger than the pid of the node itself, and the node itself 
  leaves the running to become the leader. otherwise, the node consumes the message.
- at the end of the leg for the message, a confirmation msg is returned to the original node to let it know that its message
  has made it all the way to the end in 2^round distance in that direction.

And so at every round we are guauranteed that 1/2 of the candidates are being eliminated.

*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>

void beginElection();

#define LEADERTAG 10
#define WINNERTAG 20
#define FINISHEDTAG 30

int leader_rank;
int PNUM;               // random prime number to shake process ids up
int rank, size, pid;    // mpi rank, total size, and this process's id, computed with our prime number thing
int round_counter;      // the current round
int messages_sent;      // how many this node has sent
int messages_received;  // how many messages this node has received
int total_sent;
int total_received;
int leader=1;           // am i currently in the running for a leader? - initially, i am
int electing=1;         // are we currently still electing a leader? - initially, we are

// election message format : int array
// index 0 - distanceToGo
// index 1 - direction 0 for left (+1) 1 for right (-1)
// index 2 - pid
// index 3 - rank from 

// count totals msg format : int array
// index 0 - messages_sent
// index 1 - messages_received

int main(int argc, char** argv){

    MPI_Init( &argc, &argv ); 

    if (argc < 2){
        printf("Usage : electleader <co-prime number with the number of nodes initialized by mpiexec\n");
        return EXIT_SUCCESS;
    }

    PNUM = atoi(argv[1]);

    MPI_Comm_rank( MPI_COMM_WORLD, &rank ); 
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    pid = ((rank + 1) * PNUM)  % size;

    beginElection();

    MPI_Finalize();
    return EXIT_SUCCESS;
}

int getNextNeighbor(int rank, int offset){
    if(rank + offset < 0){
        return size-1;
    }else return (int) (rank+offset)%size;
}

void sendMessage(int msg[4], int to_rank){
    // some ugly switching to accomodate usage of sending messages in one function altogether
    if(to_rank==-1 && msg){
        // send msg to left and right        
        if(msg[1]==-1){
            int right = getNextNeighbor(rank, 1);
            MPI_Send(msg, 4, MPI_INT, right, LEADERTAG, MPI_COMM_WORLD);
        }else{
            int left = getNextNeighbor(rank, -1);
            MPI_Send(msg, 4, MPI_INT, left, LEADERTAG, MPI_COMM_WORLD);
        }
        messages_sent++;
    }else if (msg){
        MPI_Send(msg, 4, MPI_INT, to_rank, LEADERTAG, MPI_COMM_WORLD);
        messages_sent++;
    }else if (!msg && to_rank==-1){
        int win = 1;
        int i = 0;
        for (i=0;i<size;i++){
            if (i!=rank){
                messages_sent++;
                MPI_Send(&win,1,MPI_INT,i,WINNERTAG, MPI_COMM_WORLD);
            }
        }
    }
}

void beginElection(){
    while(electing && pow(2,round_counter) <= size){
        int distance = pow(2, round_counter);
        
        int left_msg[4] = {distance, -1, pid, rank};
        int right_msg[4] = {distance, 1, pid, rank};
        
        if (leader){ 
            sendMessage(left_msg, -1);
            sendMessage(right_msg, -1);
        }
        // if still in the running, send a election msg out

        int got_left = -1; // did we get the msg from the left back from this round?
        int got_right = -1; // did we get the msg back from the right this round?

        int recvd[4];
        MPI_Status status;
        MPI_Recv(recvd, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        messages_received++;

        if(status.MPI_TAG == WINNERTAG){
            electing = 0;
            leader = 0;
            leader_rank = status.MPI_SOURCE;
        }

        recvd[0]--;

        if(recvd[2] > pid) {
            // if the received msg is greater than your pid, then pass it forward and drop out
            leader = 0;
            if(recvd[0]>0){
                // still more distance to go? keep passing it
                sendMessage(recvd,-1); 
            }else{
                // otherwise, return it to the original pid
                sendMessage(recvd,recvd[3]);
            }
        }
        if(recvd[2] == pid && leader){
            if(recvd[1] == -1) got_left = 1;
            if(recvd[1] == 1) got_right = 1;
            if(got_left && got_right) sendMessage(0,-1);
        }
        round_counter++; 
    }
    if(leader){
        printf("pid %d : is leader\n", pid);
        sendMessage(0,-1);
        int i = 0;
        for (i=0; i<size-1;i++){
            MPI_Status s;
            int t[2];
            MPI_Recv(t, 2, MPI_INT, MPI_ANY_SOURCE, FINISHEDTAG, MPI_COMM_WORLD, &s); 
            total_sent += t[0];
            total_received += t[1];
        }
        printf("rank=%d, id=%d, trcvd=%d, tsent=%d\n", rank, pid, total_sent, total_received); 
    }else{
        printf("rank=%d, id=%d, leader=0, mrcvd=%d, msent=%d\n", rank, pid, messages_received, messages_sent);
        // send totals to leader
        int counts[2] = { messages_sent, messages_received };
        MPI_Send(counts, 2,MPI_INT, leader_rank, FINISHEDTAG, MPI_COMM_WORLD);
    }  
}
