//Name: Kinjal Prajapati
//ID: 110095279
//Section: 3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMANDS 5
#define MAX_ARGS 5
#define MAX_kinInput_LENGTH 100
#define MAX_BACKGROUND_PROCESSES 10
int noOfBgProcess = 0;
pid_t backgroundProcesses[MAX_BACKGROUND_PROCESSES];

void kinjalSplitCommand(char *kinInput, char **commands, int *countCommand);
void kinjalexecuteCommands(char **commands, int countCommand, pid_t *backgroundProcesses, int *noOfBgProcess);
void kinjalbgprocess();

int main()
{
    // Variable declartions to get user kinInput and the commands that they entered
    char kinInput[MAX_kinInput_LENGTH]; // kinInput from the user
    char *commands[MAX_COMMANDS];       // commands that the user entered
    int countCommand;                   // number of commands that the user entered

    pid_t backgroundProcesses[MAX_BACKGROUND_PROCESSES]; // array variablefor bg processes
    int noOfBgProcess = 0;                               // number of bg processes. MAking it 0

    // infinite loop for mshell
    while (1)
    {
        printf("mshell$ ");                          // shell prompt displaying
        fgets(kinInput, MAX_kinInput_LENGTH, stdin); // reading user kinInput
        kinInput[strcspn(kinInput, "\n")] = '\0';    // Remove trailing newline character from kinInput

        if (strcmp(kinInput, "exit") == 0) // condition to exit the shell for the user
        {
            printf("Exiting shell. See you soon...\n"); // simple print statement
            break;                                      // exit the infinite loop
        }

        if (countCommand == MAX_COMMANDS) //checking for rules
        {
            printf("Error: Maximum number of commands reached.\n"); //simple print statement
            break;
        }
        if (strcmp(kinInput, "") == 0) // condition to ignore empty kinInput for bg process
        {
            // Check and update background processes
            kinjalbgprocess();
            continue; // skip to next iteration of the infinite loop
        }

        if (strcmp(kinInput, "jobs") == 0) // checking for jobs to process in the background
        {
            // Print background processes
            printf("Background processes:\n");
            for (int i = 0; i < noOfBgProcess; i++)
            {
                printf("[%d] %d\n", i + 1, backgroundProcesses[i]);
            }
            continue; // skip to next iteration of the infinite loop
        }

        kinjalSplitCommand(kinInput, commands, &countCommand);                              // function call
        kinjalexecuteCommands(commands, countCommand, backgroundProcesses, &noOfBgProcess); // function call
    }

    return 0;
}

void kinjalSplitCommand(char *kinInput, char **commands, int *countCommand)
{
    char *token;   // variable to store the token
    int count = 0; // variable to count the number of commands counted

    token = strtok(kinInput, "|"); // strtok function to split the kinInput into commands using | as the delimiter

    while (token != NULL) // loop to process each token
    {
        // Check for conditional operators
        if (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0)
        {
            // Append the operator to the previous command
            if (count > 0)
            {
                strcat(commands[count - 1], " "); // add space before appending
                strcat(commands[count - 1], token);
            }
        }
        else
        {
            commands[count] = token; // if constion doesnt match storing token as separate command
            count++;
        }

        token = strtok(NULL, "|"); // moving to the next token
    }

    *countCommand = count; // updating the number of commands
}

void kinjalexecuteCommands(char **commands, int countCommand, pid_t *backgroundProcesses, int *noOfBgProcess)
{
    int i;     // var for loping
    int fd[2]; // var for file descriptors
    int kinFdPrev = 0;
    pid_t pid;     // VAR for pid
    int status;    // VAR for status
    int cflag = 0; // flag for conditional operator

    // loop through each command
    for (i = 0; i < countCommand; i++)
    {
        char *args[MAX_ARGS]; // array to store the command arguments
        int kinArgCount = 0;  // number of command arguments variable

        char *token = strtok(commands[i], " "); // separating the command using the space delimited string
        while (token != NULL)
        {
            args[kinArgCount] = token; // storing the token in the array
            kinArgCount++;
            token = strtok(NULL, " ");
        }
        args[kinArgCount] = NULL;

        // Check for redirection operators
        int kinRedirectIn = 0;
        int kinRedirectOut = 0;
        int kinRedirectAppend = 0;
        char *kinRedirectFile = NULL;

        // loop through each argument to check for redirection operators
        for (int j = 0; j < kinArgCount; j++)
        {
            // checking for redirection operators <
            if (strcmp(args[j], "<") == 0)
            {

                kinRedirectIn = 1;             // setting the flag to 1
                kinRedirectFile = args[j + 1]; // storing the next argument as the file name
                args[j] = NULL;                // setting the argument to NULL
            }
            // checking for redirection operators >
            else if (strcmp(args[j], ">") == 0)
            {
                kinRedirectOut = 1;            // setting the flag to 1
                kinRedirectFile = args[j + 1]; // storing the next argument as the file name
                args[j] = NULL;                // setting the argument to NULL
            }
            // checking for redirection operators >>
            else if (strcmp(args[j], ">>") == 0)
            {
                kinRedirectAppend = 1;         // setting the flag to 1
                kinRedirectFile = args[j + 1]; // storing the next argument as the file name
                args[j] = NULL;                // setting the argument to NULL
            }
        }

        // create pipie if its not last command
        if (i < countCommand - 1)
        {
            if (pipe(fd) == -1) // checking if the pipe is closed
            {
                perror("Pipe creation failed"); // simple print statement
                exit(EXIT_FAILURE);             // exit with failure
            }
        }

        // forking
        pid = fork();
        // checking if fork fails or not
        if (pid == -1)
        {
            perror("Fork failed"); // simple print statement
            exit(EXIT_FAILURE);    // exit with failure
        }
        else if (pid == 0)
        {
            // Child process
            // if its not last command handling redirections and pipe
            if (i < countCommand - 1)
            {
                close(fd[0]);                  // closing the read end of the pipe
                dup2(kinFdPrev, STDIN_FILENO); // redirecting STDIN to the previous command output
                dup2(fd[1], STDOUT_FILENO);    // redirecting STDOUT to the write end of pipe
            }
            else
            {
                dup2(kinFdPrev, STDIN_FILENO); // redirecting STDIN to the previous command output
            }

            // checking for redirectional operators
            if (kinRedirectIn)
            {
                int kinFdIn = open(kinRedirectFile, O_RDONLY); // opening the file for reading
                if (kinFdIn == -1)                             // checking if the file opened is failed
                {
                    perror("kinInput redirection failed"); // simple print statement
                    exit(EXIT_FAILURE);
                }
                dup2(kinFdIn, STDIN_FILENO); // redirecting STDIN to the kinInput file descriptor
                close(kinFdIn);              // closing the kinInput file descriptor
            }

            // checking for redirectional operators out
            if (kinRedirectOut)
            {
                // opening file for writing, creating if it doesnt exist and truncating if it does
                int kinFdOut = open(kinRedirectFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (kinFdOut == -1) // checking if the file opened is failed
                {
                    perror("Output redirection failed"); // simple print statement
                    exit(EXIT_FAILURE);
                }
                dup2(kinFdOut, STDOUT_FILENO); // redirecting STDOUT to the output file descriptor
                close(kinFdOut);               // closing the output file descriptor
            }

            if (kinRedirectAppend)
            {
                // opening file for appending, creating if it doesnt exist and appending if it does
                int kinFdAppend = open(kinRedirectFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (kinFdAppend == -1) // checking if the file opened is failed
                {
                    perror("Append redirection failed"); // simple print statement
                    exit(EXIT_FAILURE);
                }
                dup2(kinFdAppend, STDOUT_FILENO); // redirecting STDOUT to the output file descriptor
                close(kinFdAppend);               // closing the output file descriptor
            }

            if (execvp(args[0], args) == -1) // checking if the execvp function is failed
            {
                perror("Execution failed"); // simple print statement
                exit(EXIT_FAILURE);         // exit with failure
            }
        }
        else
        {
            // Parent process
            // if its not last command in pipe then execute
            if (i < countCommand - 1)
            {
                close(fd[1]);      // closing the write end of the pipe
                kinFdPrev = fd[0]; // storing the read end of the pipe as the previous command
            }

            // Check if the command is running in the background
            if (strcmp(args[kinArgCount - 1], "&") == 0)
            {
                // Adding the background process to the array
                backgroundProcesses[*noOfBgProcess] = pid;
                (*noOfBgProcess)++;                             // increasing the number of background processes
                printf("Background process %d started\n", pid); // simple print statement
            }
            else
            {
                waitpid(pid, &status, 0); // waiting for the child process to finish

                // Check the exit status for conditional execution
                if (i < countCommand - 1)
                {
                    // checking for conditional operators and status
                    if (strcmp(args[kinArgCount - 1], "&&") == 0 && status != 0)
                    {
                        cflag++; // increasing the conditional flag
                    }
                    else if (strcmp(args[kinArgCount - 1], "||") == 0 && status == 0)
                    {
                        cflag--; // decreasing the conditional flag
                    }
                }
            }
        }
    }
}

void kinjalbgprocess()
{

    pid_t pid;  // variable to store the process id
    int status; // status of background process

    for (int i = 0; i < noOfBgProcess; i++) // looping through the background processes array
    {
        pid = backgroundProcesses[i]; // getting the process id of current background process

        // checking if process has completed using WNOHANG
        // if process has completed waitpid returns pid
        if (waitpid(pid, &status, WNOHANG) > 0)
        {
            printf("Background process %d completed\n", pid); // simple print statement
            // Remove the completed process from the array
            // shifting the remaining processes to the left
            for (int j = i; j < noOfBgProcess - 1; j++)
            {
                backgroundProcesses[j] = backgroundProcesses[j + 1];
            }

            // as 1 bg process is completed, decrement the number of bg processes
            noOfBgProcess--;
            i--; // Adjust the loop counter
        }
    }
}

