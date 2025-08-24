# Wisconsin Shell (wsh)

## Overview

This project implements a simple Unix shell called **wsh** (Wisconsin Shell). The shell supports interactive and batch modes, built-in commands, variable substitution, I/O redirection, command history, and execution of external programs.

## Features

- **Interactive Mode**: Prompts user for commands with `wsh> `
- **Batch Mode**: Executes commands from a script file
- **Built-in Commands**: 
  - `cd`: Change directory
  - `exit`: Exit the shell
  - `export`: Set environment variables
  - `local`: Set shell variables
  - `vars`: Display shell variables
  - `history`: Manage command history
  - `ls`: List directory contents (built-in implementation)
- **Variable Substitution**: Supports `$VAR` substitution for both environment and shell variables
- **I/O Redirection**: Supports `<`, `>`, `>>`, `&>`, `&>>`
- **Command History**: Tracks last commands with configurable capacity
- **Path Resolution**: Searches for executables in `$PATH`
- **Comment Support**: Ignores lines starting with `#`
- **Error Handling**: Robust error handling with appropriate error messages

## Project Structure

```
project3/
├── wsh.c              # Main shell implementation
├── wsh.h              # Header file with declarations and constants
├── Makefile           # Build configuration (to be created)
├── tests/             # Test directory
│   ├── run-tests.sh   # Test runner script
│   └── ...            # Test cases
└── README.md          # This file
```

## Building the Project

1. Create a `Makefile` with the required targets:
   - `all`: Builds both optimized and debug binaries
   - `wsh`: Builds optimized binary
   - `wsh-dbg`: Builds debug binary
   - `clean`: Removes binaries
   - `submit`: Submits the project

2. Build the shell:
   ```bash
   make all
   ```

## Usage

### Interactive Mode
```bash
./wsh
wsh> command [args]
```

### Batch Mode
```bash
./wsh script.wsh
```

### Example Script
Create an executable script:
```bash
#!./wsh

# This is a comment
echo hello
local VAR=world
echo $VAR
```

Make it executable and run:
```bash
chmod +x script.wsh
./script.wsh
```

## Testing

The project includes a test framework. To run tests:

```bash
cd tests
./run-tests.sh
```

Test options:
- `-h`: Show help
- `-v`: Verbose output
- `-t n`: Run only test n
- `-c`: Continue after failures
- `-d dir`: Use alternative test directory
- `-s`: Skip pre-test initialization

## Implementation Details

### Key Components

1. **Command Parsing**: Uses `strtok()` to tokenize input with variable substitution
2. **Process Execution**: Uses `fork()` and `execv()` to run external commands
3. **Built-in Commands**: Implemented as function pointers in a dispatch table
4. **Variable Management**: 
   - Shell variables stored in a linked list
   - Environment variables managed with `setenv()/getenv()`
5. **History Management**: Circular buffer implementation for command history
6. **Redirection Handling**: File descriptor manipulation with `dup2()`

### Memory Management

The shell carefully manages memory to avoid leaks:
- All allocated memory is freed during cleanup
- Uses `valgrind` and address sanitizer for leak detection

### Error Handling

- Comprehensive error checking for system calls
- Appropriate error messages printed to stderr
- Shell continues running after most errors

## Development Notes

- Start with basic command execution before adding advanced features
- Test each feature thoroughly before moving to the next
- Use the provided test framework and create additional tests
- Check return codes of all system calls
- Use version control (git) to track changes

## Resources

- GNU Make Manual: https://www.gnu.org/software/make/manual/
- Linux man pages: `fork`, `exec`, `wait`, `dup2`, etc.
- Bash documentation for redirection and variable behavior
