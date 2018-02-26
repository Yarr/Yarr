/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

									     *******************************************************************************/
#include "SpecTxCore.h"
#include "TxCore.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
*/
int trig_mask(char **args, SpecTxCore *spec);
int trig_mode(char **args, SpecTxCore *spec);
int trig_config(char **args, SpecTxCore *spec);
int trig_edge(char **args, SpecTxCore *spec);
int trig_delay(char **args, SpecTxCore *spec);
int trig_deadtime(char **args, SpecTxCore *spec);
int trig_reset(char **args, SpecTxCore *spec);
int lsh_help(char **args, SpecTxCore *spec);
int lsh_exit(char **args, SpecTxCore *spec);

/*
  List of builtin commands, followed by their corresponding functions.
*/
const char *builtin_str[] = {
  "mask",
  "mode",
  "config",
  "logic", // alias for 'config'
  "edge",
  "delay",
  "deadtime",
  "reset",
  "help",
  "exit",
};

int (*builtin_func[]) (char **, SpecTxCore *) = {
  &trig_mask,
  &trig_mode,
  &trig_config,
  &trig_config, // 'logic' is alias for 'config'
  &trig_edge,
  &trig_delay,
  &trig_deadtime,
  &trig_reset,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

int trig_mask(char **args, SpecTxCore *spec) {
  spec->setTriggerLogicMask(std::stoi(args[1], 0, 2));
  return 1;
}

// Difficulty with using the enum, take care of it later
int trig_mode(char **args, SpecTxCore *spec) {
  switch(std::stoi(args[1]))
  {
  case 0 : spec->setTriggerLogicMode(MODE_L1A_COUNT); break;
  case 1 : spec->setTriggerLogicMode(MODE_TIMESTAMP); break;
  case 2 : spec->setTriggerLogicMode(MODE_EUDET_TAG); break;
  }
  
  return 1;
}

int trig_config(char **args, SpecTxCore *spec) {
  spec->setTriggerLogicConfig(std::stoi(args[1]));
  return 1;
}

int trig_edge(char **args, SpecTxCore *spec) {
  spec->setTriggerEdge(std::stoi(args[1], 0, 2));
  return 1;
}

int trig_delay(char **args, SpecTxCore *spec) {
  spec->setTriggerDelay(std::stoi(args[1]), std::stoi(args[2]));
  return 1;
}

int trig_deadtime(char **args, SpecTxCore *spec) {
  spec->setTriggerDeadtime(std::stoi(args[1]));
  return 1;
}

int trig_reset(char **args, SpecTxCore *spec) {
  spec->resetTriggerLogic();
  return 1;
}

int lsh_help(char **args, SpecTxCore *spec)
{
  for (unsigned int i = 1; i < sizeof(builtin_str)/sizeof(char*); ++i)
    std::cout << builtin_str[i] << std::endl;

  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
*/
int lsh_exit(char **args, SpecTxCore *spec)
{
  return 0;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
*/
int lsh_execute(char **args, SpecTxCore *spec)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args, spec);
    }
  }

  std::cout << "Unknown command" << std::endl;
  return 1;
}

#define LSH_RL_BUFSIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
*/
char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = (char*) malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = (char*) realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
*/
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = (char**) malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = (char**) realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
*/
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  SpecTxCore *spec = new SpecTxCore();

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args, spec);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
*/
int main(int argc, char **argv)
{
  // Load config files, if any.

  // TODO: If an arg was passed, run it as one command

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

