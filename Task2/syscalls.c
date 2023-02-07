#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

// Function to recursively create directories
int recursiveDirectoryCreation(const char *path) {
    char buffer[128];
    char *pointer = NULL;
    size_t length;

    snprintf(buffer, sizeof(buffer), "%s", path);
    length = strlen(buffer);
    if (buffer[length - 1] == '/')
        buffer[length - 1] = 0;
    for (pointer = buffer + 1; *pointer; pointer++)
        if (*pointer == '/') {
            *pointer = 0;
            mkdir(buffer, 0777);
            *pointer = '/';
        }
    return mkdir(buffer, 0777);
}

int main(int argc, char *argv[]){
    // Check the number of arguments present
    if (argc < 2)
    {
        fprintf(stderr, "missing argument\n");
        return 1;
    }

    // Syscall for help
    if( strcmp(argv[1], "help") == 0 )
    {
        printf("Usage: syscalls <command> [arguments]\n");
        return 0;
    }

    //Syscall for read
    else if ( strcmp(argv[1], "read") == 0 )
    {
        if (argc < 3)
        {
            fprintf(stderr, "missing argument\n");
            return 1;
        }
        int fileDesc = open(argv[2], O_RDONLY);
        if (fileDesc == -1)
        {
            fprintf(stderr, "Failed to open %s\n", argv[2]);
            return 1;
        }
        char buffer[128];
        ssize_t n;
        while ((n = read(fileDesc, buffer, sizeof(buffer))) > 0)
        {
            write(1, buffer, n);
        }
        if (n == -1)
        {
            fprintf(stderr, "Failed to read %s\n", argv[2]);
            close(fileDesc);
            return 1;
        }
        close(fileDesc);
        return 0;
    }

    // Syscall for write
    else if( strcmp(argv[1], "write") == 0 )
    {
        int fileDesc = open(argv[2], O_WRONLY | O_CREAT, 0777);
        if (fileDesc == -1)
        {
            fprintf(stderr, "Failed to open %s\n", argv[2]);
            return 1;
        }

        ssize_t totalWrittenBytes = 0;
        for (int i = 3; i < argc; i++)
        {
            ssize_t n = write(fileDesc, argv[i], strlen(argv[i]));
            write(fileDesc, "\n", 1);
            if (n == -1)
            {
                fprintf(stderr, "Failed to write to %s\n", argv[2]);
                close(fileDesc);
                return 1;
            }
            totalWrittenBytes += n;
        }

        close(fileDesc);
        printf("Wrote %ld B\n", totalWrittenBytes);
        return 0;
    }

    // Syscall for mkdir
    else if (strcmp(argv[1], "mkdir") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "missing argument\n");
            return 1;
        }

        char *path = argv[2];
        struct stat st;
        if (stat(path, &st) == 0)
        {
            fprintf(stderr, "%s already exists", path);
            return 1;
        }
        int result = recursiveDirectoryCreation(path);
        if ( result != 0 )
        {
            fprintf(stderr, "Failed to create %s", path);
            return 0;
        }
    }

    // If no matching commands are found
    else
    {
        fprintf(stderr, "invalid command\n");
        return 1;
    }
}