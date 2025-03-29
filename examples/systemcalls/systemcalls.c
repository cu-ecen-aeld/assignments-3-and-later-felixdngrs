#include "systemcalls.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>

#define IS_CHILD(pid)	(0 == pid)
#define IS_PARENT(pid)	(pid > 0)
#define IS_ERR(pid)		(pid < 0)

bool check_cmd(const char *cmd)
{
	bool is_abs_path = NULL != strchr(cmd, '/');
	return is_abs_path;
}

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
	int sysstat = system(cmd);
	int ret = false;

    if (WIFEXITED(sysstat)) {
        int exit_status = WEXITSTATUS(sysstat);
        // Exit normal
		ret = (exit_status == 0);
    }
    return ret;
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
/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    va_end(args);

	command[count] = command[count];
    
    bool is_abs_path = check_cmd(command[0]);
    if(!(is_abs_path))
    {
    	return false;
    }
    pid_t pid = fork();
    
    if(IS_PARENT(pid))
    {
    	int wstatus = 0;
    	pid_t wpid = waitpid(pid, &wstatus, 0);
    	if(wpid < 0)
    	{
    		return false;
    	}
    	if (WIFEXITED(wstatus)) {
        	return 0 == WEXITSTATUS(wstatus);
        }
    	
    } else if(IS_CHILD(pid))
    {
    	execv(command[0], command);
    	return false;
    } 
    

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
    char *command[count + 1];
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    if (!check_cmd(command[0])) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (IS_CHILD(pid)) {
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            close(fd);
            exit(1);
        }
        close(fd);

        execv(command[0], command);
        exit(1);
    }

    int status;
    pid_t wpid = waitpid(pid, &status, 0);
    if (wpid < 0) {
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

