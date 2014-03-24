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

int PNUM;               // random prime number to shake process ids up
int rank, size, pid;    // mpi rank, total size, and this process's id, computed with our prime number thing
int round_counter;      // the current round
int messages_sent;      // how many this node has sent
int messages_received;  // how many messages this node has received
int leader=1;           // am i currently in the running for a leader? - initially, i am
int electing=1;         // are we currently still electing a leader? - initially, we are

typedef struct _electionMessage{
    int distanceToGo;
    int direction; // 0 for left (+1) 1 for right (-1)
    int pid;
    int rank_from;
} em;

int main(int argc, char** argv){

    MPI_Init( &argc, &argv ); 

    if (argc < 2){
        printf("Usage : electleader <co-prime number with the number of nodes initialized by mpiexec\n");
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

void sendMessage(em* msg, int to_rank){
    const int nitems=4;
    int          blocklengths[4] = {1,1,1,1};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_em;
    MPI_Aint     offsets[4];

    offsets[0] = offsetof(em, distanceToGo);
    offsets[1] = offsetof(em, direction );
    offsets[2] = offsetof(em, pid);
    offsets[3] = offsetof(em, rank_from);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_em);
    MPI_Type_commit(&mpi_em); 

    // some ugly switching to accomodate usage of sending messages in one function altogether
    if(to_rank==-1 && msg){
        // send msg to left and right        
        if(msg->direction==-1){
            int right = getNextNeighbor(rank, 1);
            MPI_Send(msg, 1, mpi_em, right, LEADERTAG, MPI_COMM_WORLD);
        }else{
            int left = getNextNeighbor(rank, -1);
            MPI_Send(msg, 1, mpi_em, left, LEADERTAG, MPI_COMM_WORLD);
        }
        messages_sent++;
    }else if (msg){
        MPI_Send(msg, 1, mpi_em, to_rank, LEADERTAG, MPI_COMM_WORLD);
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

em receiveMsg(){
    const int nitems=4;
    int          blocklengths[4] = {1,1,1,1};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_em;
    MPI_Aint     offsets[4];

    offsets[0] = offsetof(em, distanceToGo);
    offsets[1] = offsetof(em, direction );
    offsets[2] = offsetof(em, pid);
    offsets[3] = offsetof(em, rank_from);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_em);
    MPI_Type_commit(&mpi_em); 

    em recvd;
    MPI_Status status;
    MPI_Recv(&recvd, 1, mpi_em, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    messages_received++; 
    if(status.MPI_TAG == WINNERTAG){
        electing = 0;
        leader = 0;
    } 
    return recvd;
}

void beginElection(){
    while(electing && pow(2,round_counter) <= size){
        int distance = pow(2, round_counter);
        
        em left_msg;
        left_msg.distanceToGo = distance;
        left_msg.direction = -1; 
        left_msg.pid = pid;
        left_msg.rank_from = rank;

        em right_msg = left_msg;
        right_msg.direction = 1;
        
        if (leader){ 
            sendMessage(&left_msg, -1);
            sendMessage(&right_msg, -1);
        }
        // if still in the running, send a election msg out

        em recvd = receiveMsg();
        
        int got_left = -1; // did we get the msg from the left back from this round?
        int got_right = -1; // did we get the msg back from the right this round?
 
        recvd.distanceToGo--;

        if(recvd.pid > pid) {
            // if the received msg is greater than your pid, then pass it forward and drop out
            leader = 0;
            if(recvd.distanceToGo>0){
                // still more distance to go? keep passing it
                sendMessage(&recvd,-1); 
            }else{
                // otherwise, return it to the original pid
                sendMessage(&recvd,recvd.rank_from);
            }
        }
        if(recvd.pid == pid && leader){
            if(recvd.direction == -1) got_left = 1;
            if(recvd.direction == 1) got_right = 1;
            if(got_left && got_right) sendMessage(0,-1);
        }
        round_counter++; 
    }
    if(leader){
        sendMessage(0,-1);
        printf("rank=%d, id=%d, trcvd=%d, tsent=%d\n", rank, pid, messages_received, messages_sent);
    }else{
        printf("rank=%d, id=%d, leader=0, mrcvd=%d, msent=%d\n", rank, pid, messages_received, messages_sent);
    }   
}
