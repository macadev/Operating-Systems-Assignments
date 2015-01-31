#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/*
  Structure used to hold the information necessary
  to restore a command using the history feature.
*/
typedef struct Command {
  int id;
  char *ptr_to_args[MAX_LINE/+1];
  char first_chr;
  int is_command_valid;
} Command;

/**
  setup() reads in the next command line, separating it into distinct tokens
  using whitespace as delimiters. setup() sets the args parameter as a
  null-terminated string.
*/
void setup(char inputBuffer[], char *args[], int *background) {
  int length, /* # of characters in the command line */
      i,      /* loop index for accessing inputBuffer array */
      start,  /* index where beginning of next command parameter is */
      ct;     /* index of where to place the next parameter into args[] */
  ct = 0;
  
  /* read what the user enters on the command line */
  length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
  start = -1;
  if (length == 0)
    exit(0);        /* ^d was entered, end of user command stream */
  if (length < 0){
    perror("error reading the command");
    exit(-1);       /* terminate with error code of -1 */
  }
  
  /* examine every character in the inputBuffer */
  for (i = 0; i < length; i++) {
    switch (inputBuffer[i]){
      case ' ':
      
      case '\t' : /* argument separators */
                  if (start != -1){
                    args[ct] = &inputBuffer[start];       /* set up pointer */
                    ct++;
                  }
                  inputBuffer[i] = '\0';                  /* add a null char; make a C string */
                  start = -1;
                  break;

      case '\n':  /* should be the final char examined */
                  if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                  }
                  inputBuffer[i] = '\0';
                  args[ct] = NULL;                       /* no more arguments to this command */
                  break;

      default :   /* some other character */
                  if (start == -1)
                    start = i;
                  if (inputBuffer[i] == '&'){
                    *background = 1;
                    inputBuffer[i] = '\0';
                  }
    }
  }

  args[ct] = NULL; /* just in case the input line was > 80 */
}

/* 
  Creates a Command object with the information necessary
  to replicate it using the history feature.
*/
Command *createHistoryCommand(int command_counter, char **args, int is_command_valid) {
  Command *new_command = malloc(sizeof(Command));
  new_command->id = command_counter;
  new_command->first_chr = *(args[0]);
  new_command->is_command_valid = is_command_valid;
  
  int i = 0;
  while (args[i] != NULL) {
    new_command->ptr_to_args[i] = malloc(strlen(args[i]));
    if (new_command->ptr_to_args[i] == NULL) {
      printf("memory allocation failed! \n");
      exit(1);
    }
    new_command->ptr_to_args[i] = strdup(args[i]);
    i++;
  }

  return new_command;
}

/*
  Returns the first command to math the character specified by the user.
  Note that it will only check the 10 most recent commands.
*/
Command *findCommandInHistory(char firstCharInCommand, Command** prev_commands, int command_counter) {
  int i = command_counter;
  while (i >= 0) {
    if (prev_commands[i] == NULL) {
      i--;
      continue;
    }
    if (prev_commands[i]->first_chr == firstCharInCommand) {
      return prev_commands[i];
    }
    i--;
  }
  return NULL;
}

/*
  Returns the most recent command executed by the user.
*/
Command *findMostRecentCommand(Command** prev_commands, int command_counter) {
  int i = command_counter;
  while (i >= 0) {
    if (prev_commands[i] == NULL) {
      i--;
      continue;
    } else {
      return prev_commands[i];
    }
  }
  return NULL;
}

/*
  Display the list of all the commands that the user has used.
*/
void executeHistoryCommand(Command** prev_commands, int command_counter) {
  int j = 0;
  for (int i = 0; i <= command_counter - 1; i++) {
    printf("%d ", prev_commands[i]->id);
    while (prev_commands[i]->ptr_to_args[j] != NULL) {
      printf("%s ", prev_commands[i]->ptr_to_args[j]);
      j++;
    }
    printf("\n");
    j = 0;
  }
}

/*
  Check if the command object is valid, i.e not null
*/
char **isCommandValid(Command *cmd) {
  if (cmd == NULL) {
    return NULL;          
  } else {
    return cmd->ptr_to_args;
  }
}

/*
  Creates and stores a Command struct based on the input from the user.
  It also stores an integer used to determine if the execution of the 
  command was successful or not. 
*/
Command** saveCommandToHistory(char **args, int result_of_exec, int *command_counter, int *command_list_size, Command** prev_commands) {
  Command *latest_cmd;
  if (result_of_exec == 1) { //The command couldn't be executed successfully
    latest_cmd = createHistoryCommand(*command_counter, args, 1);
  } else {                   //Command executed successfully
    latest_cmd = createHistoryCommand(*command_counter, args, 0);
  }
  
  if (*command_counter + 1 > *command_list_size) {
    (*command_list_size) *= 2;

    Command** new_command_storage = malloc(*command_list_size * sizeof(Command));
    memcpy(new_command_storage, prev_commands, *command_list_size * sizeof(Command));
    prev_commands = new_command_storage;
  }
  prev_commands[*command_counter] = latest_cmd;
  (*command_counter)++;
  return prev_commands;
}

/*
  Notifies the user that either there are no previous commands to show,
  or that no command exists with the specified character.
*/
void notifyUserHistoryCommandFailed(int no_commands_exist, int no_commands_with_given_char_exist) {
  if (no_commands_exist) {
    printf("No previous commands have been recorded! \n");  
    no_commands_exist = 0;
  } 

  if (no_commands_with_given_char_exist) {
    printf("Previous command with that character doesn't exist! \n");  
    no_commands_with_given_char_exist = 0;
  }
}

/*
  Prints the arguments of the command. Used to notify
  the user of the command being ran when using the history feature.
*/
void printCommand(char **args) {
  printf("Executing the following command: ");
  int j = 0;
  while (args[j] != NULL) {
    printf("%s ", args[j]);
    j++;
  }
  printf("\n");
}

/*
  Function to change directories.
  You can go up the directory tree by using cd ..
  The command prompt displays the current directory.
*/
void executeChangeDirectoryCommand(char **args) {
  char *space = " "; 

  /*
    Path name has spaces, note that the input should not have
    backslashes, to access a directory that has a space in its name
    write out its name without escaping the spaces. This is due to
    the behavior of the setup function.
  */
  if (args[2] != NULL) {
    char *concatenatedPath = (char*) malloc(300);
    
    int j = 1;
    //Concatenate each word contained in the Path
    while (args[j] != NULL) {
      strcat(concatenatedPath, args[j]);
      if (args[j+1] != NULL) {
        strcat(concatenatedPath, space);
      }
      j++;
    }

    // Navifate to the path
    if (chdir(concatenatedPath) == -1) {
      printf("%s\n", strerror(errno));
    }
    free(concatenatedPath);

  } else {

    // Path doesn't have any spaces in its name
    if (chdir(args[1]) == -1) {
      printf("%s\n", strerror(errno));
    }
  }
}

/*
  Prints the current working directory. Upon starting the shell
  the default directory is the root.
*/
void printWorkingDirectory() {
  char *pwd = (char*) malloc(150*sizeof(char));
  if (pwd == NULL) {
    printf("memory allocation failed! \n");
    exit(1);
  }
  getcwd(pwd, 300);
  printf("%s\n", pwd);
  free(pwd);
}

/*
  Returns the path to the current working directory
*/
char *getWorkingDirectory() {
  char *pwd = (char*) malloc(150*sizeof(char));
  if (pwd == NULL) {
    printf("memory allocation failed! \n");
    exit(1);
  }
  getcwd(pwd, 300);
  return pwd;
}

/*
  Stores the pid of a background process so that it can be displayed when
  the 'jobs' command is activated
*/
void saveProcessID(int pid, int *activeJobs, int *jobIndex) {
  activeJobs[*jobIndex] = pid;
  (*jobIndex)++;
  printf("%d\n", *jobIndex);
}

/*
  Print the jobs that have been launched with '&' at the end.
*/
void printJobs(int *activeJobs, int *jobIndex) {
  if (*jobIndex == 0) {
    printf("There are no background processes running.\n");
    return;
  }

  int status;
  printf("SELECTOR | PID | STATUS \n");
  for (int i = 0; i <= *jobIndex; i++) {
    if (waitpid(activeJobs[i], &status, WNOHANG) == 0) {
      // Process is still alive  
      printf("[%d] %d RUNNING\n", i, activeJobs[i]);
    } else {
      // Process is dead
      printf("[%d] %d TERMINATED\n", i, activeJobs[i]);
    }
  }
}

/*
  Used to bring a process to the foreground. Activated by using
  the 'fg' command and specifying a process selector from the 
  jobs table.
*/
void bringProcessToForeground(int *activeJobs, int *jobIndex, int processSelector) {
  int status;
  if (processSelector > *jobIndex) {
    printf("No process with the specified selector exists.\n");
    return;
  }
  waitpid(activeJobs[processSelector], &status, WUNTRACED);
}

/*
  Executes the main loop of the shell.
*/
int main(void) {

  // Variables related to the current directory path
  char *pth = "/";
  chdir(pth);

  // Variables related to jobs processing
  int *activeJobs = (int*) malloc(300 * sizeof(int));
  if (activeJobs == NULL) {
    printf("memory allocation failed! \n");
    exit(1);
  }
  int temp_index = 0;
  int *jobIndex = &temp_index;

  // Variables related to input processing
  char inputBuffer[MAX_LINE];       /* buffer to hold the command entered */
  int background;                   /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/+1];          /* command line (of 80) has max of 40 arguments */
                                    /* This is an array of pointers to chars */

  // Variables used to control the history command
  int no_commands_exist = 0;
  int no_commands_with_given_char_exist = 0;
  int *command_counter;
  int cmd_counter_temp = 0;
  command_counter = &cmd_counter_temp;
  int *command_list_size;
  int cmd_list_size_temp = 10;
  command_list_size = &cmd_list_size_temp;
  Command *temp_command;
  Command **prev_commands = malloc(*command_list_size * sizeof(Command));
  if (prev_commands == NULL) {
    printf("memory allocation failed! \n");
    exit(1);
  }

  while (1) {                 /* Program terminates normally inside setup */
    background = 0;

    // Print promp and current path
    printf("COMMAND:%s->\n", getWorkingDirectory());
    setup(inputBuffer, args, &background); /* get next command */
    // If there is no input, display prompt again.
    if (args[0] == NULL) continue;

    char **temp_arr;
    if (strcmp(args[0], "exit") == 0) {
      // free the allocated memory before exiting
      free(activeJobs);
      for (int i = 0; i < *command_list_size; i++) {
        free(prev_commands[i]);
      }
      free(prev_commands);
      exit(0);
    } else if (strcmp(args[0], "history") == 0) {
      
      // display the command history
      executeHistoryCommand(prev_commands, *command_counter);
      continue;

    } else if (strcmp(args[0], "cd") == 0) {
      
      // change directory to the specified directory
      prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
      executeChangeDirectoryCommand(args);
      continue;

    } else if (strcmp(args[0], "pwd") == 0) {
      
      // Print the current working directory
      printWorkingDirectory();
      prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
      continue;

    } else if (strcmp(args[0], "jobs") == 0) {

      // Display the active and terminated jobs
      printJobs(activeJobs, jobIndex);
      prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
      continue;

    } else if (strcmp(args[0], "fg") == 0) {
      
      // Bring a job to the foregroung. Note that a selector id must be specified. E.g 'fg 2'
      // The selectors are shown in the jobs table
      if (args[1] == NULL) {
        printf("You need to specify a process selector to bring it to the foreground.\n");
        prev_commands = saveCommandToHistory(args, 1, command_counter, command_list_size, prev_commands);
        continue;
      }
      bringProcessToForeground(activeJobs, jobIndex, atoi(args[1]));
      prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
      continue;

    } else if (*args[0] == 'r') {

      // Execute a command from the 10 most recent commands.
      if (!args[1]) {
        // user pressed r and did not pass any commands
        // load the most recent command from the history
        temp_command = findMostRecentCommand(prev_commands, *command_counter);
        temp_arr = isCommandValid(temp_command);
        no_commands_exist = 1;
      } else {
        // finding previous command that starts with the given character
        temp_command = findCommandInHistory(*args[1], prev_commands, *command_counter);
        temp_arr = isCommandValid(temp_command);
        no_commands_with_given_char_exist = 1;
      }

      //Check if the command is INVALID, in which case it is not executed.
      if (temp_command != NULL && temp_command->is_command_valid == 1){
        printf("The command you are trying to run is erroneous.\n");
        continue;
      }

      // Notify user if the command with the character doesn't exist
      // or if 'r' was pressed with no char and no commands have been
      // entered before
      if (temp_arr == NULL) {
        notifyUserHistoryCommandFailed(no_commands_exist, no_commands_with_given_char_exist);
        no_commands_exist = 0;
        no_commands_with_given_char_exist = 0;
        continue;
      }

      memcpy(args, temp_arr, MAX_LINE);
      printCommand(args);

      // Handle case where the user invokes 'r' on the change directory command
      if (strcmp(args[0], "cd") == 0) {
        prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
        executeChangeDirectoryCommand(args);
        continue;
      }

      // Display the active and terminated jobs
      if (strcmp(args[0], "jobs") == 0) {
        printJobs(activeJobs, jobIndex);
        prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
        continue;
      }

      // Run the pwd command
      if (strcmp(args[0], "pwd") == 0) {
        printWorkingDirectory();
        prev_commands = saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
        continue;
      }
    }
    
    
    /* Section dedicated to process creation and execution */
    int status;
    int result_of_exec = 0;
    pid_t childID, endID;
    childID = fork();

    if (childID == 0) {
      result_of_exec = execvp(args[0], args);
      if (result_of_exec == -1) {
        exit(EXIT_FAILURE);
      } else {
        exit(EXIT_SUCCESS);
      }

    } else if (background != 1) {
      // If background flag is not equal to 1, then
      // the parent process will wait for the child to terminate.
      endID = waitpid(childID, &status, WUNTRACED);

      if (endID == -1) exit(EXIT_FAILURE);

      if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 1) {
          result_of_exec = 1;
        } 
      }
    } else {
      // If parent doesn't wait, then the child's process ID
      // stored so that it can be displayed on the jobs table.
      saveProcessID(childID, activeJobs, jobIndex);
    }

    //The command given by the user is saved to the history.
    prev_commands = saveCommandToHistory(args, result_of_exec, command_counter, command_list_size, prev_commands);
  }
}
