#include "wsh.h"

// Structure for shell variables (linked list for insertion order)
typedef struct VarNode {
    char *name;
    char *value;
    struct VarNode *next;
} VarNode;

VarNode *var_head = NULL;

// History structure (circular buffer)
typedef struct History {
    char **commands;
    int capacity;
    int count;
    int start;
} History;

History history = {NULL, 0, 0, 0};

// Function declarations for built-in commands
int wsh_cd(char **args);
int wsh_exit_cmd(char **args);
int wsh_export(char **args);
int wsh_local_cmd(char **args);
int wsh_vars(char **args);
int wsh_history_cmd(char **args);
int wsh_ls(char **args);

// List of built-in commands and their corresponding functions
char *builtin_str[] = {
    "cd",
    "exit",
    "export",
    "local",
    "vars",
    "history",
    "ls",
};

int (*builtin_func[]) (char **) = {
    &wsh_cd,
    &wsh_exit_cmd,
    &wsh_export,
    &wsh_local_cmd,
    &wsh_vars,
    &wsh_history_cmd,
    &wsh_ls,
};

int num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/**
 * @brief Initializes the shell environment.
 */
void initialize_shell(void) {
    // Overwrite PATH with /bin
    setenv("PATH", DEFAULT_PATH, 1);

    // Initialize history
    history.capacity = MAX_HISTORY;
    history.count = 0;
    history.start = 0;
    history.commands = malloc(sizeof(char*) * history.capacity);
    if (!history.commands) {
        fprintf(stderr, "wsh: allocation error for history\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < history.capacity; i++) {
        history.commands[i] = NULL;
    }
}

/**
 * @brief Cleans up the shell before exiting.
 */
void cleanup_shell(void) {
    // Free shell variables
    VarNode *current = var_head;
    while (current) {
        VarNode *temp = current;
        current = current->next;
        free(temp->name);
        free(temp->value);
        free(temp);
    }

    // Free history
    for (int i = 0; i < history.capacity; i++) {
        if (history.commands[i]) {
            free(history.commands[i]);
        }
    }
    free(history.commands);
}

/**
 * @brief Adds a command to history.
 * 
 * @param command The command string to add.
 */
void add_history(const char *command) {
    if (history.capacity == 0) return;

    // Do not add built-in commands to history
    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(command, builtin_str[i]) == 0) {
            return;
        }
    }

    // Check if the same as the last command
    if (history.count > 0) {
        int last = (history.start + history.count - 1) % history.capacity;
        if (strcmp(history.commands[last], command) == 0) {
            return;
        }
    }

    // Add to history
    if (history.count < history.capacity) {
        history.commands[(history.start + history.count) % history.capacity] = strdup(command);
        if (!history.commands[(history.start + history.count) % history.capacity]) {
            fprintf(stderr, "wsh: allocation error for history command\n");
            return;
        }
        history.count++;
    } else {
        // Overwrite the oldest command
        free(history.commands[history.start]);
        history.commands[history.start] = strdup(command);
        if (!history.commands[history.start]) {
            fprintf(stderr, "wsh: allocation error for history command\n");
            return;
        }
        history.start = (history.start + 1) % history.capacity;
    }
}

/**
 * @brief Displays the command history.
 */
void show_history(void) {
    for (int i = 0; i < history.count; i++) {
        int idx = (history.start + history.count - 1 - i) % history.capacity;
        printf("%d) %s\n", i + 1, history.commands[idx]);
    }
}

/**
 * @brief Sets the history capacity.
 * 
 * @param capacity The new history capacity.
 */
void set_history_capacity(int capacity) {
    if (capacity <= 0) {
        fprintf(stderr, "wsh: history capacity must be positive\n");
        return;
    }

    char **new_commands = malloc(sizeof(char*) * capacity);
    if (!new_commands) {
        fprintf(stderr, "wsh: allocation error when setting history capacity\n");
        return;
    }
    for (int i = 0; i < capacity; i++) {
        new_commands[i] = NULL;
    }

    // Copy existing commands to new history
    int new_count = history.count < capacity ? history.count : capacity;
    for (int i = 0; i < new_count; i++) {
        int idx = (history.start + history.count - 1 - i) % history.capacity;
        new_commands[i] = strdup(history.commands[idx]);
        if (!new_commands[i]) {
            fprintf(stderr, "wsh: allocation error when copying history commands\n");
            // Free already copied commands
            for (int j = 0; j < i; j++) {
                free(new_commands[j]);
            }
            free(new_commands);
            return;
        }
    }

    // Free old history
    for (int i = 0; i < history.capacity; i++) {
        if (history.commands[i]) {
            free(history.commands[i]);
        }
    }
    free(history.commands);

    // Update history
    history.commands = new_commands;
    history.capacity = capacity;
    history.count = new_count;
    history.start = 0;
}

/**
 * @brief Executes a command from history by its number.
 * 
 * @param number The history number.
 * @return int Status of execution.
 */
int execute_history_command(int number) {
    if (number <= 0 || number > history.count) {
        // Invalid history number
        return 1;
    }

    int idx = (history.start + history.count - number) % history.capacity;
    char *command = history.commands[idx];
    if (!command) {
        return 1;
    }

    // Duplicate the command to avoid modifying the history
    char *command_dup = strdup(command);
    if (!command_dup) {
        fprintf(stderr, "wsh: allocation error for executing history command\n");
        return 1;
    }

    // Parse and execute the command
    char **args = parse_line(command_dup);
    if (args[0] != NULL) {
        execute_command(args);
    }

    free(command_dup);
    // Free each token
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
    return 1;
}

/**
 * @brief Handles variable substitution in tokens.
 * 
 * @param token The token to process.
 * @return char* The substituted token.
 */
char *handle_variable_substitution(char *token) {
    if (token[0] != '$') {
        return strdup(token);
    }

    char *var_name = token + 1; // Skip the '$'

    // Check environment variables first
    char *env_value = getenv(var_name);
    if (env_value) {
        return strdup(env_value);
    }

    // Then check shell variables
    VarNode *current = var_head;
    while (current) {
        if (strcmp(current->name, var_name) == 0) {
            return strdup(current->value);
        }
        current = current->next;
    }

    // Variable not found; substitute with empty string
    return strdup("");
}

/**
 * @brief Parses the input line into tokens, handling variable substitution.
 * 
 * @param line The input line.
 * @return char** Array of tokens.
 */
char **parse_line(char *line) {
    int bufsize = MAX_TOKENS, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "wsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, DELIMITERS);
    while (token != NULL) {
        // Handle variable substitution
        char *processed_token = handle_variable_substitution(token);
        tokens[position++] = processed_token;

        if (position >= bufsize) {
            bufsize += MAX_TOKENS;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "wsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}

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
int parse_redirection(char **args, char **input, char **output, int *append, int *redirect_stderr) {
    *input = NULL;
    *output = NULL;
    *append = 0;
    *redirect_stderr = 0;

    int i = 0;
    while (args[i] != NULL) {
        if (strncmp(args[i], ">>", 2) == 0) {
            *append = 1;
            *output = args[i] + 2;
            args[i] = NULL;
            break;
        } else if (strncmp(args[i], ">", 1) == 0) {
            *output = args[i] + 1;
            args[i] = NULL;
            break;
        } else if (strncmp(args[i], "<", 1) == 0) {
            *input = args[i] + 1;
            args[i] = NULL;
            break;
        } else if (strncmp(args[i], "&>", 2) == 0) {
            *output = args[i] + 2;
            *redirect_stderr = 1;
            args[i] = NULL;
            break;
        } else if (strncmp(args[i], "&>>", 3) == 0) {
            *output = args[i] + 3;
            *append = 1;
            *redirect_stderr = 1;
            args[i] = NULL;
            break;
        }
        i++;
    }

    return 0;
}

/**
 * @brief Applies redirections by modifying file descriptors.
 * 
 * @param input Redirected input file (if any).
 * @param output Redirected output file (if any).
 * @param append Whether to append to the output file.
 * @param redirect_stderr Whether to redirect stderr.
 * @return int Status of applying redirections (0 on success, -1 on error).
 */
int apply_redirection(char *input, char *output, int append, int redirect_stderr) {
    // Backup original file descriptors
    int stdin_fd = dup(STDIN_FILENO);
    int stdout_fd = dup(STDOUT_FILENO);
    int stderr_fd = dup(STDERR_FILENO);
    
    if (stdin_fd == -1 || stdout_fd == -1 || stderr_fd == -1) {
        perror("wsh: dup failed");
        return -1;
    }

    // Handle input redirection
    if (input) {
        int fd = open(input, O_RDONLY);
        if (fd == -1) {
            perror("wsh: input redirection failed");
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("wsh: dup2 for stdin failed");
            close(fd);
            return -1;
        }
        close(fd);
    }

    // Handle output redirection
    if (output) {
        int flags = O_WRONLY | O_CREAT;
        if (append) {
            flags |= O_APPEND; // Append to the file
        } else {
            flags |= O_TRUNC; // Truncate the file
        }
        int fd = open(output, flags, 0644);
        if (fd == -1) {
            perror("wsh: output redirection failed");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("wsh: dup2 for stdout failed");
            close(fd);
            return -1;
        }
        close(fd);
    }

    // Handle stderr redirection
    if (redirect_stderr) {
        if (output) {
            // Redirect stderr to stdout if output redirection is active
            if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
                perror("wsh: dup2 for stderr failed");
                return -1;
            }
        } else {
            // If no output file is specified, redirect stderr to /dev/null
            int fd = open("/dev/null", O_WRONLY);
            if (fd == -1) {
                perror("wsh: opening /dev/null failed");
                return -1;
            }
            if (dup2(fd, STDERR_FILENO) == -1) {
                perror("wsh: dup2 for stderr failed");
                close(fd);
                return -1;
            }
            close(fd);
        }
    }

   // Restore original file descriptors (optional, depending on use case)
    //dup2(stdin_fd, STDIN_FILENO);
    //dup2(stdout_fd, STDOUT_FILENO);
    //dup2(stderr_fd, STDERR_FILENO);

    //Close backup descriptors
    close(stdin_fd);
    close(stdout_fd);
    close(stderr_fd);

    return 0;
}
/**
 * @brief Resets file descriptors to their original state.
 * 
 * @param stdin_fd Original stdin file descriptor.
 * @param stdout_fd Original stdout file descriptor.
 * @param stderr_fd Original stderr file descriptor.
 */
void reset_redirection(int stdin_fd, int stdout_fd, int stderr_fd) {
    dup2(stdin_fd, STDIN_FILENO);
    dup2(stdout_fd, STDOUT_FILENO);
    dup2(stderr_fd, STDERR_FILENO);
    close(stdin_fd);
    close(stdout_fd);
    close(stderr_fd);
}

/**
 * @brief Executes the parsed command.
 * 
 * @param args Array of arguments.
 * @return int Status of execution.
 */
int execute_command(char **args) {
    if (args[0] == NULL) {
        // Empty command
        return 1;
    }

    // Check if the command is a history execution
    if (strcmp(args[0], "history") == 0 && args[1] != NULL) {
        int num = atoi(args[1]);
        if (num > 0) {
            return execute_history_command(num);
        }
    }

    // Check for built-in commands
    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    // Not a built-in command; launch external program
    // Add to history
    // Reconstruct the command string
    int len = 0;
    for (int i = 0; args[i] != NULL; i++) {
        len += strlen(args[i]) + 1; // +1 for space or null terminator
    }
    char *command_str = malloc(len);
    if (!command_str) {
        fprintf(stderr, "wsh: allocation error when adding to history\n");
        return 1;
    }
    command_str[0] = '\0';
    for (int i = 0; args[i] != NULL; i++) {
        strcat(command_str, args[i]);
        if (args[i+1] != NULL) {
            strcat(command_str, " ");
        }
    }
    add_history(command_str);
    free(command_str);

    return launch_process(args);
}

/**
 * @brief Launches a program and waits for it to terminate.
 * 
 * @param args Array of arguments.
 * @return int Status of execution.
 */
int launch_process(char **args) {
    pid_t pid, wpid;
    int status;

    // Parse redirections
    char *input = NULL;
    char *output = NULL;
    int append = 0;
    int redirect_stderr = 0;
    parse_redirection(args, &input, &output, &append, &redirect_stderr);

    pid = fork();
    if (pid == 0) {
        // Child process

        // Apply redirections
        if (apply_redirection(input, output, append, redirect_stderr) == -1) {
            exit(EXIT_FAILURE);
        }

        // Handle variable substitution already done in parse_line()

        // If the command contains a slash, execute it directly
        if (strchr(args[0], '/')) {
            execv(args[0], args);
            // If execv returns, there was an error
            perror("wsh");
            exit(EXIT_FAILURE);
        } else {
            // Search in PATH
            char *path_env = getenv("PATH");
            if (!path_env) {
                fprintf(stderr, "wsh: PATH not set\n");
                exit(EXIT_FAILURE);
            }

            // Duplicate PATH to tokenize
            char *path_dup = strdup(path_env);
            if (!path_dup) {
                fprintf(stderr, "wsh: allocation error\n");
                exit(EXIT_FAILURE);
            }

            char *dir = strtok(path_dup, ":");
            char executable_path[1024];
            int found = 0;

            while (dir != NULL) {
                snprintf(executable_path, sizeof(executable_path), "%s/%s", dir, args[0]);
                if (access(executable_path, X_OK) == 0) {
                    found = 1;
                    break;
                }
                dir = strtok(NULL, ":");
            }

            if (!found) {
                fprintf(stderr, "wsh: command not found: %s\n", args[0]);
                free(path_dup);
                exit(EXIT_FAILURE);
            }

            execv(executable_path, args);
            // If execv returns, there was an error
            perror("wsh");
            free(path_dup);
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // Error forking
        perror("wsh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1) {
                perror("wsh");
                break;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/**
 * @brief Built-in command: change directory.
 */
int wsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "wsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("wsh");
        }
    }
    return 1;
}



/**
 * @brief Built-in command: exit the shell.
 */
int wsh_exit_cmd(char **args) {
    if (args[1] != NULL) {
        fprintf(stderr, "wsh: exit takes no arguments\n");
        return 1;
    }
    cleanup_shell();
    exit(EXIT_SUCCESS);
}

/**
 * @brief Built-in command: export environment variable.
 */
int wsh_export(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "wsh: export requires an argument\n");
        return 1;
    }

    char *arg = args[1];
    char *equal_sign = strchr(arg, '=');
    if (!equal_sign) {
        fprintf(stderr, "wsh: export requires VAR=VALUE format\n");
        return 1;
    }

    *equal_sign = '\0';
    char *var = arg;
    char *value = equal_sign + 1;

    if (setenv(var, value, 1) != 0) {
        perror("wsh");
    }

    return 1;
}

/**
 * @brief Built-in command: local shell variable.
 */
int wsh_local_cmd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "wsh: local requires an argument\n");
        return 1;
    }

    char *arg = args[1];
    char *equal_sign = strchr(arg, '=');
    if (!equal_sign) {
        fprintf(stderr, "wsh: local requires VAR=VALUE format\n");
        return 1;
    }

    *equal_sign = '\0';
    char *var = arg;
    char *value = equal_sign + 1;

    // Handle variable substitution in value
    char *processed_value = handle_variable_substitution(value);
    if (!processed_value) {
        processed_value = strdup("");
    }

    // Check if variable already exists
    VarNode *current = var_head;
    while (current) {
        if (strcmp(current->name, var) == 0) {
            // Update existing variable
            free(current->value);
            current->value = processed_value;
            return 1;
        }
        current = current->next;
    }

    // Add new variable
    VarNode *new_var = malloc(sizeof(VarNode));
    if (!new_var) {
        fprintf(stderr, "wsh: allocation error for shell variable\n");
        free(processed_value);
        return 1;
    }
    new_var->name = strdup(var);
    new_var->value = processed_value;
    new_var->next = NULL;

    // Append to the end of the list to maintain insertion order
    if (var_head == NULL) {
        var_head = new_var;
    } else {
        VarNode *tail = var_head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = new_var;
    }

    return 1;
}

/**
 * @brief Built-in command: display shell variables.
 */
int wsh_vars(char **args) {
    (void)args; // Mark as unused to prevent compiler warnings
    VarNode *current = var_head;
    while (current) {
        printf("%s=%s\n", current->name, current->value);
        current = current->next;
    }
    return 1;
}

/**
 * @brief Built-in command: history management.
 */
int wsh_history_cmd(char **args) {
    if (args[1] == NULL) {
        // Display history
        show_history();
    } else if (strcmp(args[1], "set") == 0) {
        if (args[2] == NULL) {
            fprintf(stderr, "wsh: history set requires a number\n");
            return 1;
        }
        int new_capacity = atoi(args[2]);
        if (new_capacity <= 0) {
            fprintf(stderr, "wsh: history set requires a positive integer\n");
            return 1;
        }
        set_history_capacity(new_capacity);
    } else {
        // Attempt to execute a history command
        int num = atoi(args[1]);
        if (num > 0) {
            return execute_history_command(num);
        }
    }
    return 1;
}

/**
 * @brief Built-in command: ls.
 */
int wsh_ls(char **args) {
    (void)args; // Mark as unused to prevent compiler warnings

    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        // Set LANG=C and execute ls -1 --color=never
        setenv("LANG", "C", 1);
        char *ls_args[] = {"ls", "-1", "--color=never", NULL};
        execv("/bin/ls", ls_args);
        // If execv returns, there was an error
        perror("wsh");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("wsh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1) {
                perror("wsh");
                break;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/**
 * @brief Displays the shell prompt.
 */
void display_prompt(void) {
    printf("wsh> ");
    fflush(stdout);
}

/**
 * @brief Reads a line of input from the user or batch file.
 * 
 * @param input_stream The input stream to read from.
 * @return char* The input line.
 */
char *read_line(FILE *input_stream) {
    char *line = NULL;
    size_t bufsize = 0; // getline allocates buffer
    ssize_t nread;

    nread = getline(&line, &bufsize, input_stream);
    if (nread == -1) {
        free(line);
        return NULL;
    }

    return line;
}

/**
 * @brief Main function: Entry point of the shell.
 */
int main(int argc, char **argv) {
    char *line;
    char **args;
    int status = 1;
    FILE *input_stream = stdin;

    initialize_shell();

    // Batch mode if a file is provided
    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (!input_stream) {
            perror("wsh");
            cleanup_shell();
            exit(EXIT_FAILURE);
        }
    } else if (argc > 2) {
        fprintf(stderr, "wsh: too many arguments\n");
        cleanup_shell();
        exit(EXIT_FAILURE);
    }

    // Main loop
    while (status) {
        if (input_stream == stdin) {
            display_prompt();
        }

        line = read_line(input_stream);
        if (!line) {
            // EOF reached
            break;
        }

        // Ignore comments and empty lines
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '#' || *trimmed == '\0' || *trimmed == '\n') {
            free(line);
            continue;
        }

        // Add to history before parsing to handle history execution properly
        // (excluding built-in commands)
        char *line_copy = strdup(trimmed);
        if (line_copy) {
            // Remove trailing newline
            size_t len = strlen(line_copy);
            if (len > 0 && line_copy[len-1] == '\n') {
                line_copy[len-1] = '\0';
            }
            add_history(line_copy);
            free(line_copy);
        }

        args = parse_line(trimmed);
        status = execute_command(args);

        free(line);
        // Free each token
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
        free(args);
    }

    if (input_stream != stdin) {
        fclose(input_stream);
    }

    cleanup_shell();

    return EXIT_SUCCESS;
}
