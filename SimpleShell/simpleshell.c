#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */


typedef struct Command {
  int id;
  char *ptr_to_args[MAX_LINE/+1];
  char first_chr;
} Command;

/**
* setup() reads in the next command line, separating it into distinct tokens
* using whitespace as delimiters. setup() sets the args parameter as a
* null-terminated string.
*/
void setup(char inputBuffer[], char *args[], int *background)
{
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

Command *createHistoryCommand(int command_counter, char **args) {
  Command *new_command = malloc(sizeof(Command));
  new_command->id = command_counter;
  new_command->first_chr = *(args[0]);
  
  int i = 0;
  while (args[i] != NULL) {
    new_command->ptr_to_args[i] = malloc(strlen(args[i]));
    if (new_command->ptr_to_args[i] == NULL) {
      printf("memory allocaiton failed! \n");
      exit(1);
    }
    new_command->ptr_to_args[i] = strdup(args[i]);
    i++;
  }

  return new_command;
}

char **findCommandInHistory(char first_char_of_cmd, Command** prev_commands, int command_counter) {
  int i = command_counter;
  while (i >= 0) {
    if (prev_commands[i] == NULL) {
      i--;
      continue;
    }
    if (prev_commands[i]->first_chr == first_char_of_cmd) {
      return prev_commands[i]->ptr_to_args;
    }
    i--;
  }
  return NULL;
}

char **findMostRecentCommand(Command** prev_commands, int command_counter) {
  int i = command_counter;
  while (i >= 0) {
    if (prev_commands[i] == NULL) {
      i--;
      continue;
    } else {
      return prev_commands[i]->ptr_to_args;
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

int main(void)
{
  int no_commands_exist = 0;
  int no_commands_with_given_char_exist = 0;
  char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
  int background;             /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/+1];    /* command line (of 80) has max of 40 arguments */
                              /* This is an array of pointers to chars */

  int command_counter = 0;
  int command_list_size = 10;
  Command **prev_commands = malloc(command_list_size * sizeof(Command));
  int j = 0;
  int i = 0;

  while (1) {                 /* Program terminates normally inside setup */
    background = 0;
    printf(" COMMAND->\n");
    setup(inputBuffer, args, &background); /* get next command */

    char **temp_arr;
    if (strcmp(args[0], "history") == 0) {
      executeHistoryCommand(prev_commands, command_counter);
      continue;
    } else if (*args[0] == 'r') {

      if (!args[1]) {
        // user pressed r and did not pass any commands
        temp_arr = findMostRecentCommand(prev_commands, command_counter);
        no_commands_exist = 1;
      } else {
        // finding previous command that starts with the given character
        temp_arr = findCommandInHistory(*args[1], prev_commands, command_counter);
        no_commands_with_given_char_exist = 1;
      }

      // notify user if the command with the character doesn't exist
      // or if 'r' was pressed with no char and no commands have been
      // entered before
      if (temp_arr == NULL) {
        if (no_commands_exist) {
          printf("No previous commands have been recorded! \n");  
          no_commands_exist = 0;
        } 

        if (no_commands_with_given_char_exist) {
          printf("Previous command with that character doesn't exist! \n");  
          no_commands_with_given_char_exist = 0;
        }
        continue;
      }

      memcpy(args, temp_arr, MAX_LINE);

      printf("Executing the following command: ");
      j = 0;
      while (args[j] != NULL) {
        printf("%s ", args[j]);
        j++;
      }
      printf("\n");

    }
      
    Command *latest_cmd = createHistoryCommand(command_counter, args);
    
    if (command_counter + 1 > command_list_size) {
      command_list_size *= 2;
      realloc(prev_commands, command_list_size * sizeof(Command));
    }
    prev_commands[command_counter] = latest_cmd;

    printf("%d\n", command_counter);
    command_counter++;

    
    int status;
    pid_t childID, endID;
    childID = fork();

    /* the steps are:
    (1) fork a child process using fork()
    (2) the child process will invoke execvp()
    (3) if background == 1, the parent will wait,
    otherwise returns to the setup() function. */
    if (childID == 0) {
      execvp(args[0], args);
      exit(EXIT_SUCCESS);
    } else if (background != 1) {
      endID = waitpid(childID, &status, WNOHANG|WUNTRACED);
      while (endID != childID) {
        //printf("waiting for child");
        endID = waitpid(childID, &status, WNOHANG|WUNTRACED);
      }
    }
  }
}