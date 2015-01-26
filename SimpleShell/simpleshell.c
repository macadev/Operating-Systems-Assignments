#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define EXEC_FAILED -1


typedef struct Command {
  int id;
  char *ptr_to_args[MAX_LINE/+1];
  char first_chr;
  int is_command_valid;
} Command;

/**
* setup() reads in the next command line, separating it into distinct tokens
* using whitespace as delimiters. setup() sets the args parameter as a
* null-terminated string.
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

Command *findCommandInHistory(char first_char_of_cmd, Command** prev_commands, int command_counter) {
  int i = command_counter;
  while (i >= 0) {
    if (prev_commands[i] == NULL) {
      i--;
      continue;
    }
    if (prev_commands[i]->first_chr == first_char_of_cmd) {
      return prev_commands[i];
    }
    i--;
  }
  return NULL;
}

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

char **isCommandValid(Command *cmd) {
  if (cmd == NULL) {
    return NULL;          
  } else {
    return cmd->ptr_to_args;
  }
}

void saveCommandToHistory(char **args, int result_of_exec, int *command_counter, int *command_list_size, Command** prev_commands) {
  Command *latest_cmd;
  if (result_of_exec == 1) { //The command couldn't be executed successfully
    latest_cmd = createHistoryCommand(*command_counter, args, 1);
  } else {                   //Command executed successfully
    latest_cmd = createHistoryCommand(*command_counter, args, 0);
  }
  
  if (*command_counter + 1 > *command_list_size) {
    (*command_list_size) *= 2;
    realloc(prev_commands, *command_list_size * sizeof(Command));
  }
  prev_commands[*command_counter] = latest_cmd;
  (*command_counter)++;
}

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

void printCommand(char **args) {
  printf("Executing the following command: ");
  int j = 0;
  while (args[j] != NULL) {
    printf("%s ", args[j]);
    j++;
  }
  printf("\n");
}

void executeChangeDirectoryCommand(char **args, char *old_path) {
  // char *temp = "changed boy";

  // if (strcmp(args[1], "..") == 0) {
  //   if (chdir(old_path) == -1) printf(strerror(errno));
  // } else {

  // }

  // memcpy(old_path, temp, strlen(temp));
}

void printWorkingDirectory() {
  char *pwd = (char*) malloc(150*sizeof(char));
  if (pwd == NULL) {
    printf("memory allocation failed! \n");
    exit(1);
  }
  getcwd(pwd, 300);
  printf("%s\n", pwd);
}

int main(void) {

  char *pth = "/";
  char *old_path = (char*) malloc(300);
  memcpy(old_path, pth, strlen(pth));
  chdir(old_path);

  int no_commands_exist = 0;
  int no_commands_with_given_char_exist = 0;
  char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
  int background;             /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/+1];    /* command line (of 80) has max of 40 arguments */
                              /* This is an array of pointers to chars */

  int *command_counter;
  int cmd_counter_temp = 0;
  command_counter = &cmd_counter_temp;
  int *command_list_size;
  int cmd_list_size_temp = 10;
  command_list_size = &cmd_list_size_temp;
  Command *temp_command;
  Command **prev_commands = malloc(*command_list_size * sizeof(Command));

  while (1) {                 /* Program terminates normally inside setup */
    background = 0;
    printf(" COMMAND->\n");
    setup(inputBuffer, args, &background); /* get next command */

    char **temp_arr;
    if (strcmp(args[0], "history") == 0) {
      executeHistoryCommand(prev_commands, *command_counter);
      continue;
    } else if (strcmp(args[0], "cd") == 0) {
      executeChangeDirectoryCommand(args, old_path);
    } else if (strcmp(args[0], "pwd") == 0) {
      printWorkingDirectory();
      saveCommandToHistory(args, 0, command_counter, command_list_size, prev_commands);
      continue;
    } else if (*args[0] == 'r') {

      if (!args[1]) {
        // user pressed r and did not pass any commands
        temp_command = findMostRecentCommand(prev_commands, *command_counter);
        temp_arr = isCommandValid(temp_command);
        no_commands_exist = 1;
      } else {
        // finding previous command that starts with the given character
        temp_command = findCommandInHistory(*args[1], prev_commands, *command_counter);
        temp_arr = isCommandValid(temp_command);
        no_commands_with_given_char_exist = 1;
      }

      //Check if the command is INVALID, if it is don't execute it.
      if (temp_command != NULL && temp_command->is_command_valid == 1){
        printf("The command you are trying to run is erroneous.\n");
        continue;
      }

      // notify user if the command with the character doesn't exist
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

    }

    /* the steps are:
    (1) fork a child process using fork()
    (2) the child process will invoke execvp()
    (3) if background == 1, the parent will wait,
    otherwise returns to the setup() function. */
    
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
      endID = waitpid(childID, &status, WUNTRACED);

      while (endID != childID) {
        endID = waitpid(childID, &status, WUNTRACED);
      }

      if (endID == -1) exit(EXIT_FAILURE);

      if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 1) {
          result_of_exec = 1;
        } 
      }
    }

    saveCommandToHistory(args, result_of_exec, command_counter, command_list_size, prev_commands);

  }
}
