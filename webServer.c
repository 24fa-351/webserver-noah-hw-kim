#include "webServer.h"

#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
// #include "httpMessage.h"

/*
ref:
https://stackoverflow.com/questions/20508788/do-i-need-content-type-application-octet-stream-for-file-download

*/

static int number_of_requests = 0;
static int bytes_received = 0;
static int bytes_sent = 0;

void printNotFound(int socket_fd) {
    dprintf(socket_fd,
            "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><body><h1>404 Not "
            "Found</h1></body></html>");
}

void printBadRequest(int socket_fd) {
    dprintf(socket_fd,
            "HTTP/1.1 400 Bad Request\nContent-Type: text/html\n\n<html><body><h1>400 Bad "
            "Request</h1></body></html>");
}

void parseHttpMessage(char *buffer, char *method, char *path, char *http_version) {
    sscanf(buffer, "%s %s %s", method, path, http_version);
}

void handleHttpMessage(int socket_fd, char *buffer) {
    char method[MAX_METHOD_LENGTH];
    char path[MAX_PATH_LENGTH];
    char http_version[MAX_HTTP_VERSION_LENGTH];

    parseHttpMessage(buffer, method, path, http_version);

    if (strncmp(method, "GET", 3) != 0 || strlen(method) > 4) {
        printBadRequest(socket_fd);
        return;
    }

    if (strlen(path) > 256) {
        printBadRequest(socket_fd);
        return;
    }

    if (strncmp(http_version, "HTTP/1.1", 8) != 0) {
        printBadRequest(socket_fd);
        return;
    }

    if (strncmp(path, "/calc/", 6) == 0) {
        handleCalcRequest(socket_fd, path);
    } else if (strncmp(path, "/stats/", 7) == 0) {
        handleStatsRequest(socket_fd);
    } else if (strncmp(path, "/static/", 8) == 0) {
        handleStaticRequest(socket_fd, path);
    } else {
        printNotFound(socket_fd);
    }
}

void handleCalcRequest(int socket_fd, char *path) {
    int a, b;
    sscanf(path, "/calc/%d/%d", &a, &b);

    char response[] =
        "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>%d + %d = "
        "%d</h1></body></html>";
    bytes_received += sizeof(response) - 1;

    dprintf(socket_fd, response, a, b, a + b);
}

void handleStatsRequest(int socket_fd) {
    char response[] =
        "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Number of request: "
        "%d</h1><h1>Total Bytes Received: %d</h1><h1>Total Bytes Sent:%d</h1></body></html>";

    bytes_sent += sizeof(response) - 1;

    dprintf(socket_fd, response, number_of_requests, bytes_received, bytes_sent);
}

void handleStaticRequest(int socket_fd, char *path) {
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), ".%s", path);
    path = full_path;

    struct stat st;
    if (stat(path, &st) < 0) {
        perror("stat");
        dprintf(socket_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }
    int file_size = st.st_size;
    bytes_sent += file_size;

    int file_fd = open(path, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        dprintf(socket_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    dprintf(socket_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/png\r\n"
            "Content-Disposition: inline; filename=\"%s\"\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            path + 1, file_size);

    char buffer[BUFFER_SIZE];

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        write(socket_fd, buffer, bytes_read);
    }

    close(file_fd);
}

// input pointer is freed by this function
void handleConnection(int *client_fd_ptr) {
    int socket_fd = *client_fd_ptr;
    free(client_fd_ptr);

    printf("Handling connection on %d\n", socket_fd);

    while (1) {
        char buffer[BUFFER_SIZE];
        int bytes_read = read(socket_fd, buffer, sizeof(buffer));

        if (bytes_read < 0) {
            perror("Read failed");
            break;
        }

        if (bytes_read == 0) {
            printf("Connection closed\n");
            break;
        }

        if (strncmp(buffer, "exit", 4) == 0) {
            printf("Exiting\n");
            break;
        }

        if (strncmp(buffer, "/r/n", 4) == 0) {
            printf("Invalid Request\n");
            continue;
        }

        printf("Received: %s from connection %d\n", buffer, socket_fd);

        number_of_requests += 1;
        bytes_received += bytes_read;

        handleHttpMessage(socket_fd, buffer);
    }

    printf("Done with connection %d\n", socket_fd);
    close(socket_fd);
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;

    if (argc > 2 && strcmp(argv[1], "-p") == 0) {
        port = atoi(argv[2]);
    }

    // create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // set up server address
    struct sockaddr_in server_address;
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    int err_code;
    // bind socket to server address
    err_code = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (err_code < 0) {
        perror("Bind failed");
        return 1;
    }

    // listen for connetions
    err_code = listen(server_fd, LISTEN_BACKLOG);
    if (err_code < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port %d\n", port);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while (1) {
        pthread_t thread;
        int *client_fd_buf = malloc(sizeof(int));

        *client_fd_buf = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (*client_fd_buf < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Accepted connection on %d\n", *client_fd_buf);

        pthread_create(&thread, NULL, (void *(*)(void *))handleConnection, (void *)client_fd_buf);
    }
    close(server_fd);

    return 0;
}