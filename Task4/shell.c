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
int handle_redirection(char **tokens);
void handle_signal(int signum);
void add_to_history(char *input);
void display_history();
void set_variable(char *input);
void export_variable(char *input);
void expand_variables(char **tokens);

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
    int index = 0;
    int inside_quotes = 0;

    while (*input != '\0')
    {
        while (*input == ' ' || *input == '\t' || *input == '\n')
            *input++ = '\0';

        if (*input == '\"')
        {
            input++;
            inside_quotes = 1;
        }

        // Handle ${variable} syntax
        if (*input == '$' && *(input + 1) == '{')
        {
            input += 2;
            char *closing_brace = strchr(input, '}');
            if (closing_brace)
            {
                *closing_brace = '\0';
                char *value = getenv(input);
                if (value)
                {
                    tokens[index++] = value;
                }
                else
                {
                    tokens[index++] = "";
                }
                input = closing_brace + 1;
                continue;
            }
        }

        tokens[index++] = input;

        if (inside_quotes)
        {
            char *quote_pos = strchr(input, '\"');
            if (quote_pos)
            {
                inside_quotes = 0;
                *quote_pos = '\0';
                input = quote_pos + 1;
            }
            else
            {
                input += strlen(input);
            }
        }
        else
        {
            while (*input != '\0' && *input != ' ' && *input != '\t' && *input != '\n')
                input++;

            if (*input != '\0')
            {
                *input = '\0';
                input++;
            }
        }
    }
    tokens[index] = NULL;
}

void execute_command(char **tokens)
{
    if (tokens[0] == NULL)
        return;

    expand_variables(tokens);

    if (strcmp(tokens[0], "history") == 0)
    {
        display_history();
    }
    else if (strstr(tokens[0], "=") != NULL)
    {
        set_variable(tokens[0]);
    }
    else if (strcmp(tokens[0], "export") == 0)
    {
        export_variable(tokens[1]);
    }
    else
    {
        int pipe_found = 0;
        for (int i = 0; tokens[i] != NULL; i++)
        {
            if (strcmp(tokens[i], "|") == 0)
            {
                pipe_found = 1;
                break;
            }
        }

        if (pipe_found)
        {
            pipe_handler(tokens);
        }
        else
        {
            int status;
            pid_t pid = fork();

            if (pid == 0)
            {
                if (!handle_redirection(tokens))
                {
                    if (execvp(tokens[0], tokens) < 0)
                    {
                        perror("myshell");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if (pid > 0)
            {
                waitpid(pid, &status, 0);
                if (WIFEXITED(status))
                {
                    printf("Child exited with code %d\n", WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    printf("Child terminated by signal %d\n", WTERMSIG(status));
                }
            }
            else
            {
                perror("myshell");
            }
        }
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
    if (input)
    {
        char *name = strtok(input, "=");
        char *value = strtok(NULL, "=");
        if (name && value)
        {
            setenv(name, value, 1);
        }
        else
        {
            printf("myshell: export: %s: not a valid identifier\n", input);
        }
    }
}

void expand_variables(char **tokens)
{
    for (int i = 0; tokens[i] != NULL; i++)
    {
        if (tokens[i][0] == '$')
        {
            char *var_name = tokens[i] + 1;
            char *value = getenv(var_name);
            if (value != NULL)
            {
                tokens[i] = value;
            }
            else
            {
                tokens[i] = "";
            }
        }
    }
}

void pipe_handler(char **tokens)
{
    int num_pipes = 0;
    for (int i = 0; tokens[i] != NULL; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            num_pipes++;
        }
    }

    int pipe_fds[2 * num_pipes];

    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(pipe_fds + i * 2) < 0)
        {
            perror("myshell");
            exit(EXIT_FAILURE);
        }
    }

    int tokens_index = 0;
    int pipe_index = 0;

    for (int i = 0; i < num_pipes + 1; i++)
    {
        int start = tokens_index;
        while (tokens[tokens_index] != NULL)
        {
            if (strcmp(tokens[tokens_index], "|") == 0)
            {
                tokens[tokens_index] = NULL;
                break;
            }
            tokens_index++;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            if (i != 0)
            {
                dup2(pipe_fds[2 * (pipe_index - 1)], STDIN_FILENO);
            }
            if (i != num_pipes)
            {
                dup2(pipe_fds[2 * pipe_index + 1], STDOUT_FILENO);
            }

            for (int j = 0; j < 2 * num_pipes; j++)
            {
                close(pipe_fds[j]);
            }

            handle_redirection(tokens + start);
            execvp(tokens[start], tokens + start);
            perror("myshell");
            exit(EXIT_FAILURE);
        }

        tokens_index++;
        pipe_index++;
    }

    for (int j = 0; j < 2 * num_pipes; j++)
    {
        close(pipe_fds[j]);
    }

    for (int i = 0; i < num_pipes + 1; i++)
    {
        wait(NULL);
    }
}


int handle_redirection(char **tokens)
{
    int redirection_found = 0;
    char *filtered_tokens[MAX_NUM_TOKENS];
    int filtered_index = 0;

    for (int i = 0; tokens[i] != NULL; i++)
    {
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "2>") == 0)
        {
            redirection_found = 1;
            int fd;
            char *redirection_op = tokens[i]; // Store the redirection operator
            if (strcmp(redirection_op, ">") == 0)
            {
                fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
            }
            else
            {
                fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDERR_FILENO);
            }
            close(fd);

            i++; // Skip the next token (filename)
        }
        else
        {
            filtered_tokens[filtered_index++] = tokens[i];
        }
    }
    filtered_tokens[filtered_index] = NULL;

    // Update the tokens array
    for (int i = 0; i <= filtered_index; i++)
    {
        tokens[i] = filtered_tokens[i];
    }

    return redirection_found;
}
