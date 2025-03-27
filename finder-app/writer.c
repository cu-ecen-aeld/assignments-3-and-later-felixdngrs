#include <syslog.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MIN_ARGS (2)

int main(int argc, char **argv)
{
	const mode_t file_mode = 0666;
	const char * const program_name = argv[0];

	if((argc - 1) != MIN_ARGS)
	{
		syslog(LOG_ERR, "Error: Two arguments are required");
		syslog(LOG_ERR, "Usage: %s <file path> <text to write>", program_name);
	}
	
	const char * writefile = argv[1]; 
	const char * writestr = argv[2];
	openlog(program_name , LOG_PID | LOG_CONS | LOG_PERROR, LOG_USER);
	
	int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, file_mode);

	if(fd < 0)
	{
		syslog(LOG_ERR, "Error: couldn't open file '%s'", writefile);
		return 1;
	}

	syslog(LOG_DEBUG, "Writing '%s' to %s", writestr, writefile);
	
	ssize_t bytes_written = write(fd, writestr, strlen(writestr));
	if (bytes_written < 0) {
		syslog(LOG_ERR, "Error: couldn't write to file %s", writefile);
        close(fd);
        return 1;
    }

	close(fd);

	closelog();
	return 0;
}
