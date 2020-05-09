/*
	Joseph Om, Spring 2020
	Homework 2 - Process Creation and Process Management in C
	Score: 100/100 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>

// Function to check if all child processes have terminated
void checkChildren(int* childProcessCount){
		// Process every child 
		int i;
		for (i = 0; i < *childProcessCount; i++){
			int status;

			int curr_pid = waitpid(-1, &status, WNOHANG);

			if (curr_pid == 0){
				continue;
			}

			if (WIFSIGNALED(status)) {
				printf("[process %d terminated abnormally]\n", curr_pid);
				*childProcessCount -= 1;
			}

			else {
				printf("[process %d terminated with exit status %d]\n", curr_pid, WEXITSTATUS(status));
				*childProcessCount -= 1;
			}
		}
}

int main(int argc, char** argv) {

	// Disable buffered output
	setvbuf( stdout, NULL, _IONBF, 0 );

	// Obtain current working directory
	long size = pathconf(".", _PC_PATH_MAX);
	char* currPath = calloc(size, sizeof(char));	
	currPath = getcwd(currPath, (size_t) size);
	
	// 2d array to hold all possible paths 
	char** pathContainer = calloc(50, sizeof(char*)); 

	// Handle $MYPATH on where to search for commands
	char* MYPATH = calloc(900, sizeof(char));
	if (getenv("MYPATH") != NULL){
		strcpy(MYPATH, getenv("MYPATH"));
	}
	else {
		strcpy(MYPATH, "/bin:.");
	}

	char* path = strtok(MYPATH, ":");
	int pathCount = 0;

	while (path != NULL){
		pathContainer[pathCount] = calloc(100, sizeof(char)); 
		strcpy(pathContainer[pathCount], path);
		pathContainer[pathCount][strlen(pathContainer[pathCount])] = '\0';
		pathCount += 1;
		path = strtok(NULL, ":");
	}

	int childProcessCount = 0; 

	while (1){	

		// Allocate space for user input 
		char* userInput = calloc(1024, sizeof(char));

		// Allocate space for the tokens  
		char** tokenContainer = calloc(16, sizeof(char*));
		
		// Check present child processes 
		checkChildren(&childProcessCount);

		// Output current working directory
		printf("%s$ ", currPath);

		// Recieve command from user
		fgets(userInput, 1024, stdin);

		userInput[strlen(userInput) - 1] = '\0';

		// If the user has inputted "exit", end the loop
		if (!strcmp(userInput, "exit")) {
			// Deallocation of memory allocated for the command tokens
			int i;
			for (i = 0; i < 16; i++){
				if (tokenContainer[i] != NULL){
					free(tokenContainer[i]);
				}
			}
			free(tokenContainer);
		    free(userInput);
			break;
		}

		// Parse the command tokens 
		int tokenCount = 0;
		char* token = strtok(userInput, " ");

		while (token != NULL){
			char* currToken = calloc(65, sizeof(char));
			strcpy(currToken, token);
			tokenContainer[tokenCount] = currToken;
			tokenCount += 1;
			token = strtok(NULL, " ");
		}

		// Checks if the current command contains a pipe
		int containsPipe  = 0;	
		int pipeIndex = 0;

		int l;
		for (l = 0; l < tokenCount; l++){
			if (!strcmp(tokenContainer[l],"|")){
				containsPipe = 1;
				pipeIndex = l;
			}
		}

		// Handling a "cd" command 
		if (!strcmp(tokenContainer[0], "cd")){

			int success;

			// cd
			if (tokenCount == 1){
				success = chdir(getenv("HOME"));

				if (success == 0){
					free(currPath);
					currPath = calloc(strlen(getenv("HOME") + 1), sizeof(char));
					strcpy(currPath, getenv("HOME"));
				}
			}

			// cd /
			else if (!strcmp(tokenContainer[1], "/")){
				success = chdir("/");

				if (success == 0){
					free(currPath);
					currPath = calloc(pathconf(".", _PC_PATH_MAX) + 1, sizeof(char));
					getcwd(currPath, pathconf(".", _PC_PATH_MAX));
				}
			}

			// cd {arg}
			else {
				free(currPath);
				currPath = calloc(pathconf(".", _PC_PATH_MAX) + 1, sizeof(char));	
				getcwd(currPath, pathconf(".", _PC_PATH_MAX));

				char* temp = calloc(900, sizeof(char));
				strcpy(temp, currPath);

				temp[strlen(temp)] = '/';
				
				int i;
				for (i = 0; i < strlen(tokenContainer[1]); i++) {
					temp[strlen(temp)] = tokenContainer[1][i];
				}

				temp[strlen(temp)] = '\0';

				success = chdir(tokenContainer[1]);

				if (success == 0){
					free(currPath);
					currPath = calloc(pathconf(".", _PC_PATH_MAX) + 1, sizeof(char));	
					getcwd(currPath, pathconf(".", _PC_PATH_MAX));
				}
				else{
					fprintf(stderr, "chdir() failed: Not a directory\n");
				}
				free(temp);
			}

			// Deallocation of memory allocated for the command tokens
			int i;
			for (i = 0; i < tokenCount; i++){
				if (tokenContainer[i] != NULL){
					free(tokenContainer[i]);
				}
			}
			free(tokenContainer);
		    free(userInput);
			continue; 
		}

		int foundExec = 0;
		int foundExecOne = 0;
		int foundExecTwo = 0;
		int pathIndex[2];

		// (PIPE) Locating the command executable for both commands 
		if (containsPipe) {
			int j;
			for (j = 0; j < 2; j++){
				int i;
				int commandIndex = 0;

				if (j == 1){
					commandIndex = pipeIndex + 1;
				}

				for (i = 0; i < pathCount; i++){
					if (pathContainer[i] == NULL){
						break;
					}
					pathIndex[j] = i;

					char* tempPath = calloc(200, sizeof(char));
					strcpy(tempPath, pathContainer[i]);

					tempPath[strlen(tempPath)] = '/';

					int k;
					for (k = 0; k < strlen(tokenContainer[commandIndex]); k++){
						tempPath[strlen(tempPath)] = tokenContainer[commandIndex][k];
					}

					struct stat buf;
					int rc= lstat(tempPath, &buf);
					free(tempPath);

					if (rc == 0){
						if (buf.st_mode & S_IXUSR){
							if (j == 0){
								foundExecOne = 1;
							}
							if (j == 1){
								foundExecTwo = 1;
							}
						}
						break;
					}
				}
			}
		}

		// (NO PIPE) Locating the command executable, check every path + / + token[0]
		else {

			int i;
			for (i = 0; i < pathCount; i++){	
				if (pathContainer[i] == NULL){
					break;
				}

				pathIndex[0] = i;

				char* tempPath = calloc(200, sizeof(char));
				strcpy(tempPath, pathContainer[i]);

				tempPath[strlen(tempPath)] =  '/';

				// Add the argument to the path
				int j; 
				for (j = 0; j < strlen(tokenContainer[0]); j++){
					tempPath[strlen(tempPath)] = tokenContainer[0][j];
				}

				struct stat buf;
				int rc = lstat(tempPath, &buf);

				free(tempPath);

				// Check if the file exists 
				if (rc == 0){

					// Check if the file can be executed
					if (buf.st_mode & S_IXUSR){
						foundExec = 1;
					} 
					break;
				}
			}
		}

		// (PIPE) In the case that the command was not found 
		if (containsPipe && (foundExecOne == 0 || foundExecTwo == 0)){
			fprintf(stderr, "ERROR: one or both of the commands not found");
		}

		// (NO PIPE) In the case that the command was not found
		else if (!containsPipe && foundExec == 0){
			fprintf(stderr, "ERROR: command \"%s\" not found\n", tokenContainer[0]);
		}

		// (NO PIPE) Execution of the command
		else if (foundExec != 0 && !containsPipe) {
			// Create executable path string
			char* exec = calloc(strlen(tokenContainer[0]) + strlen(pathContainer[pathIndex[0]]) + 2, sizeof(char));
			strcpy(exec, pathContainer[pathIndex[0]]);
			exec[strlen(exec)] = '/';

			int i;
			for (i = 0; i < strlen(tokenContainer[0]); i++){
				exec[strlen(exec)] = tokenContainer[0][i];
			}


			exec[strlen(exec)] = '\0';

			// Process background processes
			int isBackground = 0; 
		
			if (strcmp(tokenContainer[tokenCount-1], "&") == 0){
				isBackground = 1;
				free(tokenContainer[tokenCount - 1]);
				tokenContainer[tokenCount - 1] = '\0';
			}		 

			// Begin execution of command
			pid_t pid;
			pid = fork();

			if (pid == -1){
				perror("Failed to fork...");
				return EXIT_FAILURE;
			}

			// Child process
			if (pid == 0){
				execv(exec, tokenContainer);
				perror("EXEC FAILED\n");
				return EXIT_FAILURE;
			}

			// Parent process
			else if (pid > 0){
				if (isBackground == 0){
					waitpid(pid, NULL, 0);
				}
				else{
					printf("[running background process \"%s\"]\n", tokenContainer[0]);
					childProcessCount += 1;
					int child_pid = waitpid(-1, NULL, WNOHANG);
					
					if (child_pid == -1){
						perror("waitpid() error\n" );
						return EXIT_FAILURE;
					}
				}
 			}
			free(exec);
		}

		// (PIPE) Execution of the command
		else if (foundExecOne != 0 && foundExecTwo != 0 && containsPipe){
			
			// Process background processes 
			int isBackground = 0; 
		
			if (strcmp(tokenContainer[tokenCount-1], "&") == 0){
				isBackground = 1;
				free(tokenContainer[tokenCount - 1]);
				tokenContainer[tokenCount - 1] = '\0';
				tokenCount -= 1;
			}		 

			int i;

			// Create executable path string one 
			char* execOne = calloc(strlen(tokenContainer[0]) + strlen(pathContainer[pathIndex[0]]) + 2, sizeof(char));
			strcpy(execOne, pathContainer[pathIndex[0]]);
			execOne[strlen(execOne)] = '/';

			for (i = 0; i < strlen(tokenContainer[0]); i++){
				execOne[strlen(execOne)] = tokenContainer[0][i];
			}
			execOne[strlen(execOne)] = '\0';

			// Create arguments array for command one 
			char** argsOne = calloc(pipeIndex + 1, sizeof(char*));
			int argsCountOne = 0;

			for (i = 0; i < pipeIndex; i++){
				argsOne[i] = calloc(strlen(tokenContainer[i]) + 1, sizeof(char));
				strcpy(argsOne[i], tokenContainer[i]);
				argsCountOne += 1;
			}

			// Create executable path command two
			char* execTwo = calloc(strlen(tokenContainer[pipeIndex + 1]) + strlen(pathContainer[pathIndex[1]]) + 2, sizeof(char));
			strcpy(execTwo, pathContainer[pathIndex[1]]);
			execTwo[strlen(execTwo)] = '/';

			for (i = 0; i < strlen(tokenContainer[pipeIndex + 1]); i++){
				execTwo[strlen(execTwo)] = tokenContainer[pipeIndex + 1][i];
			}
			execTwo[strlen(execTwo)] = '\0';

			// Create arguments array for command two
			char** argsTwo = calloc(tokenCount - pipeIndex, sizeof(char*));
			int argsCountTwo = 0;

			for (i = pipeIndex + 1; i < tokenCount; i++){
				argsTwo[argsCountTwo] = calloc(strlen(tokenContainer[i]) + 1, sizeof(char));
				strcpy(argsTwo[argsCountTwo], tokenContainer[i]);
				argsCountTwo += 1;
			}


			// Create pipe
			int pipefd[2]; 
			int rc = pipe(pipefd);
			if (rc == -1){
				perror("Pipe creation failed\n");
				return EXIT_FAILURE;
			}

			// Begin execution of the commands
			pid_t pid1;
			pid1 = fork();
			if (pid1 == -1){
				perror("Failed to fork...\n");
				return EXIT_FAILURE;
			}

			// Child process commmand 1
			if (pid1 == 0){
				// Close READ
				close(pipefd[0]);
				close(1);
				dup2(pipefd[1], 1);
				close(pipefd[1]);
				execv(execOne, argsOne);
				perror("EXEC ONE FAILED\n");
				return EXIT_FAILURE;
			}

			pid_t pid2;
			pid2 = fork();
			if (pid2 == -1){
				perror("Failed to fork...\n");
				return EXIT_FAILURE;
			}
			// Child process command 2
			if (pid2 == 0 && pid1 > 0){
				// Close WRITE
				close(pipefd[1]);
				close(0);
				dup2(pipefd[0], 0);	
				close(pipefd[0]);
				execv(execTwo, argsTwo);
				perror("EXEC TWO FAILED\n");
				return EXIT_FAILURE;
			}

			if (pid2 > 0 && pid1 > 0){
				// Parent process
				close(pipefd[0]);
				close(pipefd[1]);

				if (!isBackground) {
					waitpid(pid2, NULL, WUNTRACED | WCONTINUED);
					waitpid(pid1, NULL, WUNTRACED | WCONTINUED);
				}

				else {
					printf("[running background process \"%s\"]\n", tokenContainer[0]);
					printf("[running background process \"%s\"]\n", tokenContainer[tokenCount - pipeIndex]);
					
					childProcessCount += 2;
					int child_1 = waitpid(-1, NULL, WNOHANG);
					int child_2 = waitpid(-1, NULL, WNOHANG);

					if (child_1 == -1){
						perror("waitpid() error\n");
						return EXIT_FAILURE;
					}

					if (child_2 == -1){
						perror("waitpid() error\n");
						return EXIT_FAILURE;
					}
				}

				// Free allocated memory 
				for (i = 0; i < argsCountOne + 1; i++){
					free(argsOne[i]);
				}
				free(argsOne);
				for (i = 0; i < argsCountTwo + 1; i++){
					free(argsTwo[i]);
				}
				free(execOne);
				free(execTwo);
				free(argsTwo);

			}
			// If not parent process, exit
			if (!(pid1 > 0 && pid2 > 0)){
				exit(1);
			}
		}

		// Deallocation of memory allocated for the command tokens
		int i;
		for (i = 0; i < 16; i++){
			if (tokenContainer[i] != NULL){
				free(tokenContainer[i]);
			}
		}
		free(tokenContainer);
	    free(userInput);
	}

	// Output bye
	printf("bye\n");

	// Deallocation of memory allocated for the possible paths 
	int i;
	for (i = 0; i < 50; i++){
		if (pathContainer[i] != NULL){
			free(pathContainer[i]);
		}
	}

	free(pathContainer);
	free(currPath);
	free(MYPATH);

	return EXIT_SUCCESS;
}