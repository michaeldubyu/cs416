#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

int world_rank, world_size;
int leader_id = -1;
int is_participant = 0;
const int ELECTION_TAG = 20;
const int LEADER_TAG = 30;

int main(int argc, char ** argv){
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	

	int token;
	
	if(leader_id < 0){
		send_election_message(world_rank);
		is_participant = 1;
	}

	while(1){
		MPI_Status status;
		int from = calculate_from();
		MPI_Recv(&token, 1, MPI_INT, from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("Process %d recieved a tag of : %d with a token of %d\n", world_rank, status.MPI_TAG, token);
		if(status.MPI_TAG == ELECTION_TAG){
			if(token > world_rank){
				//If the UID in the election message is larger, the process unconditionally forwards the 
				//election message in a clockwise direction.
				send_election_message(token);
				is_participant = 1;
			}else if(token < world_rank && !is_participant){
				//If the UID in the election message is smaller, and the process is not yet a participant, 
				//the process replaces the UID in the message with its own UID, sends the updated election 
				//message in a clockwise direction.
				send_election_message(world_rank);
				is_participant = 1;

			} else if(token < world_rank && is_participant){
				// If the UID in the election message is smaller, and the process is
				//already a participant (i.e., the process has already sent out an 
				//election message with a UID at least as large as its own UID), the 
				//process discards the election message.

				//do nothing
				continue;
			}else if(token == world_rank){
				// If the UID in the incoming election message is the same as the 
				//UID of the process, that process starts acting as the leader.

				//we have won
				
				send_leader_message(world_rank);
				is_participant = 0;
			}else {
				exit(1);
			}
		}else if(status.MPI_TAG == LEADER_TAG){
			if(token == world_rank){
				printf("RETURNED TO LEADER, ALGORITHM COMPLETED SUCCESSFULLY \n");
				break;
			}
			printf("Leader Set in Process %d as: %d\n", world_rank, token);
			is_participant = 0;
			leader_id = token;
			send_leader_message(token);
			break;
		}

	}
	MPI_Finalize();
	return 0;
}

int max(int a, int b){
	if ( a > b ){
		return a;
	}
	return b;
}

int send_leader_message(int leader){
	MPI_Send(&leader, 1, MPI_INT, (world_rank + 1) % world_size, LEADER_TAG, MPI_COMM_WORLD);
	return 1;
}


int send_election_message(int token){
	printf("Sending token %d from process %d to process %d\n", token, world_rank, (world_rank+1) % world_size);
	MPI_Send(&token, 1, MPI_INT, (world_rank + 1) % world_size, ELECTION_TAG, MPI_COMM_WORLD);
	return 1;
}
int calculate_from(){
	int from = world_rank -1;
	if(from < 0){
		from = world_size -1;
	}
	return from;
}
