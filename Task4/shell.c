#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define HISTORY_SIZE 100

char *history[HISTORY_SIZE];
int history_count = 0;

void tokenize(char *input, char **tokens, const char *delimiter);
void execute_command(char **tokens);
void pipe_handler(char **tokens);
void handle_redirection(char **tokens);
void handle_signal(int signum);
void add_to_history(char *input);
void display_history();
void set_variable(char *input);
void export_variable(char *input);

int main()
{
    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    printf("Welcome to myshell!\n");

    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_NUM_TOKENS];

    while (1)
    {
        printf("myshell> ");
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strlen(input) - 1] = '\0';

        add_to_history(input);

        tokenize(input, tokens, " ");
        execute_command(tokens);
    }

    return 0;
}

void tokenize(char *input, char **tokens, const char *delimiter)
{
    char *token;
    int index = 0;
    token = strtok(input, delimiter);
    while (token != NULL)
    {
        tokens[index++] = token;
        token = strtok(NULL, delimiter);
    }
    tokens[index] = NULL;
}

void execute_command(char **tokens)
{
    if (tokens[0] == NULL)
        return;

    if (strcmp(tokens[0], "history") == 0)
    {
        display_history();
    }
    else if (strstr(tokens[0], "=") != NULL)
    {
        set_variable(tokens[0]);
    }
    else if (strncmp(tokens[0], "export", 6) == 0)
    {
        export_variable(tokens[0]);
    }
    else
    {
        pipe_handler(tokens);
    }
}

void handle_signal(int signum)
{
    if (signum == SIGINT)
    {
        printf("\nChild process terminated by SIGINT\n");
    }
    else if (signum == SIGTSTP)
    {
        printf("\nChild process suspended by SIGTSTP\n");
    }
}

void add_to_history(char *input)
{
    if (history_count < HISTORY_SIZE)
    {
        history[history_count++] = strdup(input);
    }
    else
    {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
        {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(input);
    }
}

void display_history()
{
    for (int i = 0; i < history_count; i++)
    {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

void set_variable(char *input)
{
    char *name = strtok(input, "=");
    char *value = strtok(NULL, "=");
    setenv(name, value, 0);
}

void export_variable(char *input)
{
    char *name = strtok(input, "=");
    char *value = strtok(NULL, "=");
    setenv(name, value, 1);
}

void pipe_handler(char **tokens)
{
    char *cmds[MAX_NUM_TOKENS][MAX_NUM_TOKENS];
    int cmd_index = 0;
    int i = 0;
    while (tokens[i] != NULL)
    {
        int token_index = 0;
        while (tokens[i] != NULL && strcmp(tokens[i], "|") != 0)
        {
            cmds[cmd_index][token_index++] = tokens[i++];
        }
        cmds[cmd_index][token_index] = NULL;
        cmd_index++;
        if (tokens[i] != NULL)
            i++;
    }
    cmds[cmd_index][0] = NULL;

    int pipe_fds[2 * (cmd_index - 1)];

    for (int j = 0; j < cmd_index - 1; j++)
    {
        if (pipe(pipe_fds + j * 2) < 0)
        {
            perror("myshell");
            exit(EXIT_FAILURE);
        }
    }

    for (int j = 0; j < cmd_index; j++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            if (j != 0)
            {
                dup2(pipe_fds[(j - 1) * 2], STDIN_FILENO);
            }

            if (j != cmd_index - 1)
            {
                dup2(pipe_fds[j * 2 + 1], STDOUT_FILENO);
            }

            for (int k = 0; k < 2 * (cmd_index - 1); k++)
            {
                close(pipe_fds[k]);
            }

            handle_redirection(cmds[j]);
            if (execvp(cmds[j][0], cmds[j]) < 0)
            {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int j = 0; j < 2 * (cmd_index - 1); j++)
    {
        close(pipe_fds[j]);
    }

    for (int j = 0; j < cmd_index; j++)
    {
        wait(NULL);
    }
}

void handle_redirection(char **tokens)
{
    for (int i = 0; tokens[i] != NULL; i++)
    {
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "2>") == 0)
        {
            int fd;
            int saved_fd;
            if (strcmp(tokens[i], ">") == 0)
            {
                fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                saved_fd = dup(STDOUT_FILENO);
                dup2(fd, STDOUT_FILENO);
            }
            else
            {
                fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                saved_fd = dup(STDERR_FILENO);
                dup2(fd, STDERR_FILENO);
            }
            close(fd);

            tokens[i] = NULL;

            if (execvp(tokens[0], tokens) < 0)
            {
                perror("myshell");
                exit(EXIT_FAILURE);
            }

            dup2(saved_fd, (strcmp(tokens[i], ">") == 0) ? STDOUT_FILENO : STDERR_FILENO);
            close(saved_fd);
        }
    }
}
