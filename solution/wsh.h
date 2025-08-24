#ifndef WSH_H
#define WSH_H

// Include necessary standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// Define constants
#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100
#define DELIMITERS " \t\r\n\a"
#define DEFAULT_PATH "/bin"
#define MAX_HISTORY 5

// Function declarations

/**
 * @brief Displays the shell prompt.
 */
void display_prompt(void);

/**
 * @brief Reads a line of input from the user or batch file.
 * 
 * @param input_stream The input stream to read from.
 * @return char* The input line.
 */
char *read_line(FILE *input_stream);

/**
 * @brief Parses the input line into tokens, handling variable substitution.
 * 
 * @param line The input line.
 * @return char** Array of tokens.
 */
char **parse_line(char *line);

/**
 * @brief Executes the parsed command.
 * 
 * @param args Array of arguments.
 * @return int Status of execution.
 */
int execute_command(char **args);

/**
 * @brief Launches a program and waits for it to terminate.
 * 
 * @param args Array of arguments.
 * @return int Status of execution.
 */
int launch_process(char **args);

/**
 * @brief Initializes the shell environment.
 */
void initialize_shell(void);

/**
 * @brief Cleans up the shell before exiting.
 */
void cleanup_shell(void);

/**
 * @brief Handles variable substitution in tokens.
 * 
 * @param token The token to process.
 * @return char* The substituted token.
 */
char *handle_variable_substitution(char *token);

/**
 * @brief Adds a command to history.
 * 
 * @param command The command string to add.
 */
void add_history(const char *command);

/**
 * @brief Displays the command history.
 */
void show_history(void);

/**
 * @brief Sets the history capacity.
 * 
 * @param capacity The new history capacity.
 */
void set_history_capacity(int capacity);

/**
 * @brief Executes a command from history by its number.
 * 
 * @param number The history number.
 * @return int Status of execution.
 */
int execute_history_command(int number);

/**
 * @brief Parses redirection tokens and sets up file descriptors.
 * 
 * @param args Array of arguments.
 * @param input Redirected input file (if any).
 * @param output Redirected output file (if any).
 * @param append Whether to append to the output file.
 * @param redirect_stderr Whether to redirect stderr.
 * @return int Status of parsing (0 on success, -1 on error).
 */
int parse_redirection(char **args, char **input, char **output, int *append, int *redirect_stderr);

/**
 * @brief Applies redirections by modifying file descriptors.
 * 
 * @param input Redirected input file (if any).
 * @param output Redirected output file (if any).
 * @param append Whether to append to the output file.
 * @param redirect_stderr Whether to redirect stderr.
 * @return int Status of applying redirections (0 on success, -1 on error).
 */
int apply_redirection(char *input, char *output, int append, int redirect_stderr);

/**
 * @brief Resets file descriptors to their original state.
 * 
 * @param stdin_fd Original stdin file descriptor.
 * @param stdout_fd Original stdout file descriptor.
 * @param stderr_fd Original stderr file descriptor.
 */
void reset_redirection(int stdin_fd, int stdout_fd, int stderr_fd);

/**
 * @brief Built-in command: change directory.
 */
int wsh_cd(char **args);

/**
 * @brief Built-in command: exit the shell.
 */
int wsh_exit_cmd(char **args);

/**
 * @brief Built-in command: export environment variable.
 */
int wsh_export(char **args);

/**
 * @brief Built-in command: local shell variable.
 */
int wsh_local_cmd(char **args);

/**
 * @brief Built-in command: display shell variables.
 */
int wsh_vars(char **args);

/**
 * @brief Built-in command: history management.
 */
int wsh_history_cmd(char **args);

/**
 * @brief Built-in command: ls.
 */
int wsh_ls(char **args);

#endif // WSH_H
