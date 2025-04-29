# README Major Assignment 2 CSCE 3600

## Group Members
- Lucero Torres
- Marcus Lara Sanchez

## Project Organization
- Marcus Lara Sanchez: Implemented the `cd`, `exit`, and `path` built-in commands, batch mode, interactive mode, input/output redirection, pipelining, and signal handling.
- Lucero Torres: Implemented the `myhistory` built-in command and wrote the README documentation; assisted with debugging and defensive coding.

All group members collaborated on coding the main shell engine, testing, and GitLab commits.

## Design Overview
Our shell program supports both interactive and batch modes. 

The interactive mode shows custom prompts and waits for user input. Batch mode reads commands from a file and executes them sequentially without prompting.

Commands are parsed by splitting on semicolons for multiple sequential commands, then further split into tokens for arguments. Built-in commands (`cd`, `exit`, `path`, `myhistory`) are handled without creating a new process. External commands are executed by forking a child process and using `execv()`.

The shell also supports input/output redirection, basic pipelines, and proper signal handling so that Ctrl+C and Ctrl+Z affect only child processes.

We implemented a custom `myhistory` built-in command to store the last 20 user-entered commands and allow replay or clearing of history.

## Specifications
- If a line contains multiple semicolons, the shell ignores empty commands and continues.
- Extra whitespace between tokens is ignored when parsing commands.
- Redirection is supported for input and output separately but not simultaneously.
- Pipelining is supported up to 2 pipes.
- Built-in commands are not executed through pipelines or with redirection.
- Invalid commands result in an error message but do not crash the shell.
- The history buffer stores the most recent 20 commands, overwriting the oldest when full.
- Batch file errors are detected and cause a graceful exit.
- Very long command lines trigger a warning but do not crash the shell.

## Known Bugs
- Our shell does not currently support simultaneous use of both input and output redirection in a single command.
- Re-executing a historical command involving complex piping or redirection may behave unexpectedly.
- The PATH environment variable is modified during the shell's runtime, but not restored upon exit.
