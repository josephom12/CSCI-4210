/*
	Joseph Om, Spring 2020
	Homework 3 - Multi-threading in C using Pthreads
	Score: 100/100 
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// Global Variables
int max_squares;
int*** dead_end_boards;
int dead_end_boards_length; 
int dead_end_boards_count;
int x; 
pthread_t mainThread;

// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to handle adding a board to the main dead_end_boards
void addDeadEndBoard(int m, int n, int** board, int seen){

	// If this is not a valid dead_end_board, free it 
	if (seen < x){
		int i;
		for (i = 0; i < m; i++){
			free(board[i]);
		}
		free(board);
		return; 
	}

	pthread_mutex_lock(&mutex);

	// If the dead_end_boards capacity is filled
	if (dead_end_boards_count + 1 >= dead_end_boards_length){
		dead_end_boards = realloc(dead_end_boards, (dead_end_boards_length + 10) * sizeof(int***));
		dead_end_boards_length += 10;
	}
	// Add the board to the dead_end_boards
	dead_end_boards[dead_end_boards_count] = board; 
	dead_end_boards_count += 1;

	pthread_mutex_unlock(&mutex);
	return; 
}

// Function to free up dead end boards
void freeDeadEndBoards(int m){
	int i,j;
	for (i = 0; i < dead_end_boards_count; i++){
		for (j = 0; j < m; j++){
			free(dead_end_boards[i][j]);
		}
		free(dead_end_boards[i]);
	}
	free(dead_end_boards);
	return;
}

// Function to output all dead_end_boards
void printDeadEndBoards(int m, int n){

	printf("THREAD %ld: Dead end boards:\n", pthread_self());
	
	int i, j, k;
	for (i = 0; i < dead_end_boards_count; i++){

		printf("THREAD %ld: > ", pthread_self());
		for (j = 0; j < m; j++){
			if (j != 0){
				printf("THREAD %ld:   ", pthread_self());
			}
			for (k = 0; k < n; k++){
				if (dead_end_boards[i][j][k] == 0){
					printf(".");
				}
				else {
					printf("S");
				}
			}
		printf("\n");
		}
	}
	return;
}

// Function to find potential moves
void searchPotentialMoves(int** board, int* currPos, int m, int n, int* numMoves, int** nextMoves) {

		// (r-1), (c-2)
		if (((currPos[0] - 1 >= 0 && currPos[0] - 1 < m)  && (currPos[1] - 2 >= 0 && currPos[1] - 2 < n)) && board[currPos[0] - 1][currPos[1] - 2] == 0){
			(*numMoves)++; 
			nextMoves[0][0] = currPos[0] - 1; 
			nextMoves[0][1] = currPos[1] - 2;		}
		else {
			nextMoves[0][0] = 0;
			nextMoves[0][1] = 0;
		}
		// (r-2), (c-1)
		if ((((currPos[0] - 2) >= 0 && (currPos[0] - 2) < m)  && ((currPos[1] - 1) >= 0 && (currPos[1] - 1) < n)) && board[currPos[0] - 2][currPos[1] - 1] == 0){
			(*numMoves)++; 
			nextMoves[1][0] = currPos[0] - 2;
			nextMoves[1][1] = currPos[1] - 1;
		}
		else {
			nextMoves[1][0] = 0;
			nextMoves[1][1] = 0;
		}
		// (r-2), (c+1)
		if ((((currPos[0] - 2) >= 0 && (currPos[0] - 2) < m)  && ((currPos[1] + 1) >= 0 && (currPos[1] + 1) < n)) && board[currPos[0] - 2][currPos[1] + 1] == 0){
			(*numMoves)++; 
			nextMoves[2][0] = currPos[0] - 2;
			nextMoves[2][1] =  currPos[1] + 1;
		}
		else{
			nextMoves[2][0] = 0;
			nextMoves[2][1] = 0; 
		}
		// (r-1), (c+2)
		if ((((currPos[0] - 1) >= 0 && (currPos[0] - 1) < m)  && ((currPos[1] + 2) >= 0 && (currPos[1] + 2) < n)) && board[currPos[0] - 1][currPos[1] + 2] == 0){
			(*numMoves)++; 
			nextMoves[3][0] = currPos[0] - 1;
			nextMoves[3][1] =  currPos[1] + 2;
		}
		else{
			nextMoves[3][0] = 0;
			nextMoves[3][1] = 0;
		}
		// (r+1), (c+2)
		if ((((currPos[0] + 1) >= 0 && (currPos[0] + 1) < m)  && ((currPos[1] + 2) >= 0 && (currPos[1]  + 2) < n)) && board[currPos[0] + 1][currPos[1] + 2] == 0){
			(*numMoves)++; 
			nextMoves[4][0] = currPos[0] + 1;
			nextMoves[4][1] = currPos[1] + 2;
		}
		else{
			nextMoves[4][0] = 0;
			nextMoves[4][1] = 0;
		}
		// (r+2), (c+1)
		if ((((currPos[0] + 2) >= 0 && (currPos[0] + 2) < m)  && ((currPos[1] + 1) >= 0 && (currPos[1] + 1) < n)) && board[currPos[0] + 2][currPos[1] + 1] == 0){
			(*numMoves)++; 
			nextMoves[5][0] = currPos[0] + 2;
			nextMoves[5][1] =  currPos[1] + 1;
		}
		else{
			nextMoves[5][0] = 0;
			nextMoves[5][1] = 0;
		}
		// (r+2), (c-1)
		if ((((currPos[0] + 2) >= 0 && (currPos[0] + 2) < m)  && ((currPos[1] - 1) >= 0 && (currPos[1] - 1) < n)) && board[currPos[0] + 2][currPos[1] - 1] == 0){
			(*numMoves)++; 
			nextMoves[6][0] = currPos[0] + 2;
			nextMoves[6][1] =  currPos[1] - 1;
		}
		else {
			nextMoves[6][0] = 0;
			nextMoves[6][1] = 0;
		}
		// (r+1), (c-2)
		if ((((currPos[0] + 1) >= 0 && (currPos[0] + 1) < m)  && ((currPos[1] - 2) >= 0 && (currPos[1] - 2) < n)) && board[currPos[0] + 1][currPos[1] - 2] == 0){
			(*numMoves)++; 
			nextMoves[7][0] = currPos[0] + 1;
			nextMoves[7][1] = currPos[1] - 2;
		}
		else{
			nextMoves[7][0] = 0;
			nextMoves[7][1] = 0;
		}
		return;
}

struct processNextMoveArgs {
	int** board;
	int m;
	int n;
	int currMove;
	int currPos[2];
	int seen;
};

void* processNextMove(void* args){

	struct processNextMoveArgs* args1 = args;

	int** board;
	int m,n; 
	int currMove; 
	int currPos[2];
	int seen;
	int highest;

	board = args1->board;
	m = args1->m;
	n = args1->n;
	currMove = args1->currMove;
	currPos[0] = args1->currPos[0];
	currPos[1] = args1->currPos[1];
	seen = args1->seen;	

	highest = currMove;

	int* returnSize = malloc(1*sizeof(int));
	*returnSize = 0;

	if (currMove == 1){
		printf("THREAD %ld: Solving Sonny's knight's tour problem for a %dx%d board\n", pthread_self(), m, n);
	}

	int numMoves = 0; 

	int** nextMoves = calloc(8, sizeof(int*));

	int i;
	for (i = 0; i < 8; i++){
		nextMoves[i] = calloc(2, sizeof(int));
	}

	searchPotentialMoves(board, currPos, m, n, &numMoves, nextMoves); 

	// In the case that there is only one possible next move 
	if (numMoves == 1){

		free(args1);
		free(returnSize);

		// Find the next move, input it into the board and processNextMove 
		int nextPos[2]; 
		int i;
		for (i = 0; i < 8; i++){
			if (!(nextMoves[i][0] == 0 && nextMoves[i][1] == 0)){
				nextPos[0] = nextMoves[i][0];
				nextPos[1] = nextMoves[i][1];
				break;
			}
		}

		board[nextPos[0]][nextPos[1]] = currMove + 1;

		for (i = 0; i < 8; i++){
			free(nextMoves[i]);
		}
		free(nextMoves);

		struct processNextMoveArgs* args = malloc(sizeof(struct processNextMoveArgs));
		args->board = board;
		args->m = m;
		args->n = n;
		args->currMove = currMove + 1;
		args->currPos[0] = nextPos[0];
		args->currPos[1] = nextPos[1];
		args->seen = seen + 1;

		processNextMove(args);
	}

	// In the case that there are more than one possible move
	else if (numMoves > 1){

		free(args1);
		
		printf("THREAD %ld: %d moves possible after move #%d; creating threads...\n", pthread_self(), numMoves, currMove);

		pthread_t tid[8];

		int i;
		for (i = 0; i < 8; i++){
			if (!(nextMoves[i][0] == 0 && nextMoves[i][1] == 0)){

				// Create a new board for this specific move 
				int** newBoard = calloc(m, sizeof(int*));
				int j,k;
				for (j = 0; j < m; j++){
					newBoard[j] = calloc(n, sizeof(int));
					for (k = 0; k < n; k++){
						newBoard[j][k] = board[j][k];
					}
				}

				newBoard[nextMoves[i][0]][nextMoves[i][1]] = currMove + 1;

				int currPos[2];
				currPos[0] = nextMoves[i][0];
				currPos[1] = nextMoves[i][1];

				struct processNextMoveArgs* args = malloc(sizeof(struct processNextMoveArgs));
				args->board = newBoard;
				args->m = m;
				args->n = n;
				args->currMove = currMove + 1;
				args->currPos[0] = currPos[0];
				args->currPos[1] = currPos[1];
				args->seen = seen + 1;

				pthread_create(&tid[i], NULL, &processNextMove, args);

				#ifdef NO_PARALLEL 
				void* returnValue = malloc(0);
				free(returnValue);

				pthread_join(tid[i], (void**) &returnValue);

				if (*(int*) returnValue > highest){
					highest = *(int*) returnValue;
				}	

				printf("THREAD %ld: Thread [%ld] joined (returned %d)\n", pthread_self(), tid[i], *(int*) returnValue);
				free(returnValue);
				#endif

			}
		}

		#ifndef NO_PARALLEL
			// Wait for all children to be processed
			for (i = 0; i < 8; i++){
				if (nextMoves[i][0] != 0 && nextMoves[i][1] != 0){
					
					void* returnValue = malloc(0); 
					free(returnValue);

					pthread_join(tid[i], (void**) &returnValue);		
					
					if (*(int*) returnValue > highest){
						highest = *(int*) returnValue;
					}			

					printf("THREAD %ld: Thread [%ld] joined (returned %d)\n", pthread_self(), tid[i], highest);
					free(returnValue);
				}
			}
	
		#endif

		for (i = 0; i < 8; i++){
			free(nextMoves[i]);
		}
		free(nextMoves);
	}

	// In the case that there are no more possible moves 
	else if (numMoves == 0){

		free(args1);

		// In the case that we have visited all squares
		if (seen == m * n){
			printf("THREAD %ld: Sonny found a full knight's tour!\n", pthread_self());
			// Free the board 

			for (i = 0; i < m; i++){
				free(board[i]);
			}
			free(board);
		}
		// In the case that we have reached a dead end
		else if (seen < m * n){
			printf("THREAD %ld: Dead end after move #%d\n", pthread_self(), currMove);
			addDeadEndBoard(m, n, board, seen);
		}
		
		// Update max_squares if seen is greater
		if (currMove > max_squares){
			max_squares = currMove;
		}

		for (i = 0; i < 8; i++){
			free(nextMoves[i]);
		}
		free(nextMoves);

		*returnSize = currMove;
		pthread_exit(returnSize);
	}

	if (pthread_self() != mainThread){

		*returnSize = currMove;
		if (highest > *returnSize){
			*returnSize = highest;
		}

		for (i = 0; i < m; i++){
			free(board[i]);
		}
		free(board);

		pthread_exit(returnSize);
	}

	free(returnSize);
	return returnSize;
}

int main(int argc, char** argv){

	if (argc < 3){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
		return EXIT_FAILURE;	
	}

	// Disable buffered output for stdout 
	setvbuf( stdout, NULL, _IONBF, 0 );

	int m = atoi(argv[1]);
	int n = atoi(argv[2]);

	if (argc == 4){
		x = atoi(argv[3]);
	}
	else {
		x = 0;
	}

	// Check if m and n are greater than 2
	if (!(m > 2 && n > 2)){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
		return EXIT_FAILURE;	
	}

	// If x is present, validate that it is not greater than m x n 
	if (argc == 4) {
		int sum = m * n;
		if (x > sum){
			fprintf(stderr, "ERROR: Invalid argument(s)\n");
			fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
			return EXIT_FAILURE;	
		}
	}

	// Global Variables
	max_squares = 0;
	dead_end_boards = calloc(10, sizeof(int**));
	dead_end_boards_length = 10; 
	dead_end_boards_count = 0;

	// Initialize the starting board 
	int** board = calloc(m, sizeof(int*));
	int i, j;
	for (i = 0; i < m; i++){
		board[i] = calloc(n, sizeof(int));
		for (j = 0; j < n; j++){
			board[i][j] = 0;
		}
	}
	board[0][0] = 1; 

	// Main Thread 
	int currPos[2]; 
	currPos[0] = 0;
	currPos[1] = 0;

	struct processNextMoveArgs* args = malloc(sizeof(struct processNextMoveArgs));

	args->board = board;
	args->m = m;
	args->n = n;
	args->currMove = 1;
	args->currPos[0] = currPos[0];
	args->currPos[1] = currPos[1];
	args->seen = 1;

	mainThread = pthread_self();
	processNextMove(args);

	printf("THREAD %ld: Best solution(s) found visit %d squares (out of %d)\n", pthread_self(), max_squares, n * m);

	printDeadEndBoards(m, n);
	freeDeadEndBoards(m); 

	// End Mutex
	pthread_mutex_destroy(&mutex);

	for (i = 0; i < m; i++){
		free(board[i]);
	}
	free(board);

	return EXIT_SUCCESS;
}