#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define PORT "9000"
#define BUFFER_SIZE 1024
#define DATA_FILE_PATH "/var/tmp/aesdsocketdata"

// Global variables for signal handling
volatile sig_atomic_t stop_requested = 0;
int server_fd = -1;
int client_fd = -1;

// Signal handler for SIGINT and SIGTERM
void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        stop_requested = 1;
    }
}

// Get client IP address as string
int get_client_ip(struct sockaddr_storage *client_addr, char *ip_str, size_t ip_str_len) {
    void *addr;
    
    if (client_addr->ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)client_addr;
        addr = &(s->sin_addr);
    } else if (client_addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)client_addr;
        addr = &(s->sin6_addr);
    } else {
        return -1;
    }
    
    if (inet_ntop(client_addr->ss_family, addr, ip_str, ip_str_len) == NULL) {
        return -1;
    }
    
    return 0;
}

// Cleanup resources
void cleanup() {
    if (client_fd != -1) {
        close(client_fd);
    }
    if (server_fd != -1) {
        close(server_fd);
    }
    unlink(DATA_FILE_PATH);
    closelog();
}

// Setup signal handlers for graceful shutdown
int setup_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        return -1;
    }
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        return -1;
    }
    
    return 0;
}

// Create and bind socket
int create_socket() {
    struct addrinfo hints, *res;
    int sockfd;
    int optval = 1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) {
        syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(status));
        return -1;
    }
    
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        syslog(LOG_ERR, "socket creation failed: %s", strerror(errno));
        freeaddrinfo(res);
        return -1;
    }
    
    // Allow address reuse
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        syslog(LOG_ERR, "bind failed: %s", strerror(errno));
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }
    
    freeaddrinfo(res);
    return sockfd;
}

/**
 * Receive data until newline and append to file
 */
int receive_and_append_data(int client_fd) {
    char buffer[BUFFER_SIZE];
    int data_fd;
    ssize_t bytes_received;
    int found_newline = 0;
    
    // Open file for appending
    data_fd = open(DATA_FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (data_fd == -1) {
        syslog(LOG_ERR, "Failed to open data file: %s", strerror(errno));
        return -1;
    }
    
    while (!found_newline && !stop_requested) {
        bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                syslog(LOG_INFO, "Client closed connection");
            } else {
                syslog(LOG_ERR, "recv failed: %s", strerror(errno));
            }
            close(data_fd);
            return -1;
        }
        
        if (write(data_fd, buffer, bytes_received) != bytes_received) {
            syslog(LOG_ERR, "write to file failed: %s", strerror(errno));
            close(data_fd);
            return -1;
        }
        
        if (memchr(buffer, '\n', bytes_received) != NULL) {
            found_newline = 1;
        }
    }
    
    close(data_fd);
    return found_newline ? 0 : -1;
}

// Send file contents to client
int send_file_contents(int client_fd) {
    int data_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_sent;
    
    data_fd = open(DATA_FILE_PATH, O_RDONLY);
    if (data_fd == -1) {
        syslog(LOG_ERR, "Failed to open data file for reading: %s", strerror(errno));
        return -1;
    }
    
    while ((bytes_read = read(data_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_sent = send(client_fd, buffer, bytes_read, 0);
        if (bytes_sent != bytes_read) {
            syslog(LOG_ERR, "send failed: %s", strerror(errno));
            close(data_fd);
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        syslog(LOG_ERR, "read from file failed: %s", strerror(errno));
        close(data_fd);
        return -1;
    }
    
    close(data_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    char client_ip[INET6_ADDRSTRLEN];
    int daemon_mode = 0;
    int opt;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                daemon_mode = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                return -1;
        }
    }
    
    // Initialize syslog
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
    
    // Setup signal handlers
    if (setup_signal_handlers() == -1) {
        cleanup();
        return -1;
    }
    
    // Create and bind socket
    server_fd = create_socket();
    if (server_fd == -1) {
        cleanup();
        return -1;
    }
    
    // If daemon mode requested, fork after successful binding
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "fork failed: %s", strerror(errno));
            cleanup();
            return -1;
        }
        if (pid > 0) {
            cleanup();
            return 0;
        }
        
        if (setsid() < 0) {
            syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
            cleanup();
            return -1;
        }
        
        if (chdir("/") < 0) {
            syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
            cleanup();
            return -1;
        }
        
        int null_fd = open("/dev/null", O_RDWR);
        if (null_fd >= 0) {
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO) {
                close(null_fd);
            }
        }
    }
    
    if (listen(server_fd, 5) == -1) {
        syslog(LOG_ERR, "listen failed: %s", strerror(errno));
        cleanup();
        return -1;
    }
    
    syslog(LOG_INFO, "Server listening on port %s", PORT);
    
    while (!stop_requested) {
        client_addr_len = sizeof(client_addr);
        
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            if (stop_requested) {
                break;
            }
            syslog(LOG_ERR, "accept failed: %s", strerror(errno));
            continue;
        }
        
        if (get_client_ip(&client_addr, client_ip, sizeof(client_ip)) == -1) {
            syslog(LOG_ERR, "Failed to get client IP address");
            close(client_fd);
            client_fd = -1;
            continue;
        }
        
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        if (receive_and_append_data(client_fd) == 0) {
            if (send_file_contents(client_fd) == -1) {
                syslog(LOG_ERR, "Failed to send file contents to client");
            }
        }
        
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_fd);
        client_fd = -1;
    }
    
    cleanup();
    return 0;
}
