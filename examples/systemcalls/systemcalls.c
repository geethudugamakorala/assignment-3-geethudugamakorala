#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int result = system(cmd);
    
    // system() returns -1 on error, or the exit status of the command
    if (result == -1) {
        return false;  // system() call failed
    }
    
    // Check if command executed successfully (exit status 0)
    return (result == 0);
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (execv(command[0], command) == -1) {
            perror("execv failed");
            va_end(args);
            exit(1);
        }
    } else if (pid < 0) {
        perror("fork failed");
        va_end(args);
        return false;
    } else {
        int status;
        pid_t pid1 = wait(&status);
        if (pid1 == -1) {
            perror("wait failed");
            va_end(args);
            return false;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                // Command failed
                va_end(args);
                return false;
            }
        } else {
            // Command did not terminate normally
            va_end(args);
            return false;
        }
    }
    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        
        // Open the output file for writing
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            va_end(args);
            exit(1);
        }
        
        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 failed");
            close(fd);
            va_end(args);
            exit(1);
        }
        
        close(fd);
        
        if (execv(command[0], command) == -1) {
            // If execv fails
            perror("execv failed");
            va_end(args);
            exit(1);
        }
    } else if (pid < 0) {
        // Fork failed
        perror("fork failed");
        va_end(args);
        return false;
    } else {
        // Parent process
        int status;
        pid_t pid1 = wait(&status);
        if (pid1 == -1) {
            perror("wait failed");
            va_end(args);
            return false;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                // Command failed
                va_end(args);
                return false;
            }
        } else {
            // Command did not terminate normally
            va_end(args);
            return false;
        }
    }
    va_end(args);

    return true;
}
