#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    openlog("writer", LOG_PID, LOG_USER);
    
    if (argc != 3) {
        fprintf(stderr, "Error: Missing parameters. Usage: %s <writefile> <writestr>\n", argv[0]);
        syslog(LOG_ERR, "Invalid number of arguments provided: %d", argc - 1);
        closelog();
        exit(1);
    }
    
    const char *writefile = argv[1];
    const char *writestr = argv[2];
    
    // Open file for writing
    FILE *file = fopen(writefile, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not create file '%s': %s\n", writefile, strerror(errno));
        syslog(LOG_ERR, "Could not create file '%s': %s", writefile, strerror(errno));
        closelog();
        exit(1);
    }
    
    // Write string to file
    if (fprintf(file, "%s", writestr) < 0) {
        fprintf(stderr, "Error: Could not write to file '%s': %s\n", writefile, strerror(errno));
        syslog(LOG_ERR, "Could not write to file '%s': %s", writefile, strerror(errno));
        fclose(file);
        closelog();
        exit(1);
    }
    
    // Close file
    if (fclose(file) != 0) {
        fprintf(stderr, "Error: Could not close file '%s': %s\n", writefile, strerror(errno));
        syslog(LOG_ERR, "Could not close file '%s': %s", writefile, strerror(errno));
        closelog();
        exit(1);
    }
    
    // Log successful write operation
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    
    // Close syslog
    closelog();
    
    return 0;
}
