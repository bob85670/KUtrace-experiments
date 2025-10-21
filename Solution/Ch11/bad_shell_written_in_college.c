/*
gcc bad_shell_written_in_college.c -o badshell
./badshell

Features: 
- Executes commands by creating child processes: Yes. Uses fork and execvp.
- Handles absolute, relative, and PATH environment variable paths: Yes. Via execvp and environ.
- Built-in command: exit: Yes. Terminates shell with exit(0).
- Built-in command: timeX: No
- Operator: & (background execution): Yes. Background processes run without waiting.
- Operator: `|` (piping): Yes
- Robust to SIGINT (Ctrl-C): Yes. Handles SIGINT to terminate child processes.
- Handles SIGUSR1 for child process control: No. No SIGUSR1 handling implemented.
- Handles SIGCHLD for background process termination: Yes. Reaps terminated children with waitpid.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

#define LIMIT 256 // max number of tokens for a command
#define MAXLINE 1024 // max number of characters from user input

#define TRUE 1
#define FALSE !TRUE

// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;

static char* currentDirectory;
extern char** environ;

struct sigaction act_child;
struct sigaction act_int;

int no_reprint_prmpt;

pid_t pid;

//signal handler for SIGCHLD
void signalHandler_child(int p) {
	// wait for all dead processes.
	while (waitpid(-1, NULL, WNOHANG) > 0) { }
	printf("\n");
}

//signal handler for SIGINT
void signalHandler_int(int p) {
	//  send signal to the child process
	if (kill(pid, SIGTERM) == 0) {
		printf("\nProcess %d received a SIGINT signal\n", pid);
		no_reprint_prmpt = 1;			
	} else {
		printf("\n");
	}
}

//change directory
int changeDirectory(char* args[]) {
	// cd -> home directory
	if (args[1] == NULL) {
		chdir(getenv("HOME")); 
		return 1;
	}
	// change directory accordingly
	else { 
		if (chdir(args[1]) == -1) {
			printf(" %s: no such directory\n", args[1]);
			return -1;
		}
	}
	return 0;
}

void init() {
	// See if we are running interactively
	GBSH_PID = getpid();
	printf("Shell PID: %d\n", GBSH_PID);
	// The shell is interactive if STDIN is the terminal  
	GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);  

	if (GBSH_IS_INTERACTIVE) {
		// Loop until we are in the foreground
		while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp())) {
			kill(GBSH_PID, SIGTTIN);             
		}
			  
		// Set the signal handlers for SIGCHILD and SIGINT
		act_child.sa_handler = signalHandler_child;
		act_int.sa_handler = signalHandler_int;			
		
		sigaction(SIGCHLD, &act_child, 0);
		sigaction(SIGINT, &act_int, 0);
		
		// Put ourselves in our own process group
		setpgid(GBSH_PID, GBSH_PID); //new process group leader
		GBSH_PGID = getpgrp();
		if (GBSH_PID != GBSH_PGID) {
			printf("Error, the shell is not process group leader");
			exit(EXIT_FAILURE);
		}

		// Grab control of the terminal
		tcsetpgrp(STDIN_FILENO, GBSH_PGID);  
		
		// Save default terminal attributes for shell
		tcgetattr(STDIN_FILENO, &GBSH_TMODES);

		// Get the current directory
		currentDirectory = (char*)calloc(1024, sizeof(char));
	} else {
		printf("Could not make the shell interactive.\n");
		exit(EXIT_FAILURE);
	}
}

//execute a program, including background program
void launchProg(char** args, int background) {	 
	int err = -1;
	 
	if ((pid = fork()) == -1) {
		printf("Child process could not be created\n");
		return;
	}
	//child
	if (pid == 0) {
		//ignore SIGINT signals, let the parent process to handle them with signalHandler_int	
		signal(SIGINT, SIG_IGN);
		
		// We set parent=<pathname>/simple-c-shell as an environment variable
		// for the child
		setenv("parent", getcwd(currentDirectory, 1024), 1);	
		
		// non-existing commands -> error
		if (execvp(args[0], args) == err) {
			printf("Command not found");
			kill(getpid(), SIGTERM);
		}
	}
	 
	//parent
	 
	// If the process is not requested to be in background, wait for child
	if (background == 0){
		waitpid(pid, NULL, 0);
	} else {
		// create a background process, don't wait
		printf("Process created with PID: %d\n", pid);
	}	 
}
 
//piplining
void pipeHandler(char * args[]) {
	// File descriptors
	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
	int filedes2[2];
	
	int num_cmds = 0;
	
	char* command[256];
	
	pid_t pid;
	
	int err = -1;
	int end = 0;
	
	// Variables used for the different loops
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
	// calculate the number of commands
	while (args[l] != NULL){
		if (strcmp(args[l], "|") == 0) {
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	
	// For each command between '|', pipes will be configured 
	while (args[j] != NULL && end != 1) {
		k = 0;

		while (strcmp(args[j], "|") != 0) {
			command[k] = args[j];
			j++;	
			if (args[j] == NULL) {
				// end: check whether there are any more arguments
				end = 1;
				k++;
				break;
			}
			k++;
		}
		// Last position of the command will be NULL
		command[k] = NULL;
		j++;		
		
		// set different descriptors for the pipes inputs and output
		if (i % 2 != 0) {
			pipe(filedes); // for odd i
		} else {
			pipe(filedes2); // for even i
		}
		
		pid = fork();
		
		if (pid == -1) {			
			if (i != num_cmds - 1) {
				if (i % 2 != 0) {
					close(filedes[1]); // for odd i
				} else {
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if (pid == 0) {
			// If we are in the first command
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}

			// last command
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0], STDIN_FILENO);
				} else { // for even number of commands
					dup2(filedes2[0], STDIN_FILENO);
				}

			// middle command
			} else { // for odd i
				if (i % 2 != 0) {
					dup2(filedes2[0], STDIN_FILENO); 
					dup2(filedes[1], STDOUT_FILENO);
				} else { // for even i
					dup2(filedes[0], STDIN_FILENO); 
					dup2(filedes2[1], STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0], command) == err) {
				kill(getpid(), SIGTERM);
			}		
		}
				
		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0) {
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1) {
			if (num_cmds % 2 != 0) {					
				close(filedes[0]);
			} else {					
				close(filedes2[0]);
			}
		} else {
			if (i % 2 != 0) {					
				close(filedes2[0]);
				close(filedes[1]);
			} else {					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
		waitpid(pid, NULL, 0);
		i++;	
	}
}
			
//handle the commands
int commandHandler(char* args[]) {
	int i = 0;
	int j = 0;
	
	int fileDescriptor;
	int standardOut;
	
	int aux;
	int background = 0;
	
	char* args_aux[256];
	
	// look for the special characters and separate the command in an array
	while (args[j] != NULL) {
		if ((strcmp(args[j], ">") == 0) || (strcmp(args[j], "<") == 0) || (strcmp(args[j], "&") == 0)) {
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
	
	// 'exit' command 
	if (strcmp(args[0], "exit") == 0) {
		exit(0);
	}
	// 'pwd' command 
	else if (strcmp(args[0], "pwd") == 0) {
		if (args[j] != NULL) {
			// If we want file output
			if ((strcmp(args[j], ">") == 0) && (args[j+1] != NULL)) {
				fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				printf("%s\n", getcwd(currentDirectory, 1024));
				dup2(standardOut, STDOUT_FILENO);
			}
		} else {
			printf("%s\n", getcwd(currentDirectory, 1024));
		}
	} 
	// 'clear' command 
	else if (strcmp(args[0],"clear") == 0) {
		system("clear");
	}
	// 'cd' command 
	else if (strcmp(args[0],"cd") == 0) {
		changeDirectory(args);
	}
	else {
		// detect whether piped execution or background execution 
		while (args[i] != NULL && background == 0) {
			// background execution -> exit loop
			if (strcmp(args[i], "&") == 0) {
				background = 1;
			// piplining with '|'
			} else if (strcmp(args[i], "|") == 0) {
				pipeHandler(args);
				return 1;
			}
			i++;
		}
		// launch the program 
		args_aux[i] = NULL;
		launchProg(args_aux, background);
	}
	return 1;
}

int main(int argc, char* argv[], char** envp) {
	char line[MAXLINE]; // buffer for the user input
	char* tokens[LIMIT]; // array for the different tokens in the command
	int numTokens;
	pid = -10; // initialize pid to an pid that is not possible
	
	init();
	
	// set our extern char** environ to the environment
	environ = envp;
	
	// set shell=<pathname>/simple-c-shell as an environment variable for child
	setenv("shell", getcwd(currentDirectory, 1024), 1);
	
	// Main loop
	while (TRUE) {
		// print the shell prompt
		printf("$$ 3230shell ## ");
		
		// empty the line buffer
		memset(line, '\0', MAXLINE);

		// wait for user input
		fgets(line, MAXLINE, stdin);
	
		// no input, continue loop
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		
		// all the tokens of the input and pass it to commandHandler 
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
		
		commandHandler(tokens);
	}          
	exit(0);
}
