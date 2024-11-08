#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
// #include "httpMessage.h"

#define DEFAULT_PORT 8080
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024

static int number_of_requests = 0;
static int bytes_received = 0;
static int bytes_sent = 0;

void handleCalcRequest(int socket_fd, char *path)
{
    int a, b;
    sscanf(path, "/calc/%d/%d", &a, &b);

    char response[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>%d + %d = %d</h1></body></html>";
    bytes_received += sizeof(response) - 1;

    dprintf(socket_fd, response, a, b, a + b);
}

void handleStatsRequest(int socket_fd)
{
    char response[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Number of request: %d</h1><h1>Total Bytes Received: %d</h1><h1>Total Bytes Sent:%d</h1></body></html>";

    bytes_sent += sizeof(response) - 1;

    dprintf(socket_fd, response, number_of_requests, bytes_received, bytes_sent);
}

void handleStaticRequest(int socket_fd, char *path)
{
    FILE *file = fopen(path + 1, "r");
    if (file != NULL)
    {
        dprintf(socket_fd, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
        char c;
        while ((c = fgetc(file)) != EOF)
        {
            write(socket_fd, &c, 1);
            bytes_sent++;
        }
        fclose(file);
    } else
    {
        dprintf(socket_fd, "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><body><h1>404 Not Found</h1></body></html>");
    }
}

void handleNotFound(int socket_fd)
{
    dprintf(socket_fd, "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><body><h1>404 Not Found</h1></body></html>");
}

// input pointer is freed by this function
void handleConnection(int *client_fd_ptr)
{
    int socket_fd = *client_fd_ptr;
    free(client_fd_ptr);
    
    printf("Handling connection on %d\n", socket_fd);

    char buffer[BUFFER_SIZE];
    int bytes_read = read(socket_fd, buffer, sizeof(buffer));

    printf("Received: %s from connection %d\n", buffer, socket_fd);

    bytes_received += bytes_read;
    number_of_requests += 1;

    char method[1000];
    char path[1000];
    char http_version[1000];
    sscanf(buffer, "%s %s %s", method, path, http_version);

    if (strncmp(method, "GET", 3) == 0) {
        if (strncmp(path, "/calc/", 6) == 0) {
            handleCalcRequest(socket_fd, path);
        } else if (strncmp(path, "/stats/", 7) == 0) {
            handleStatsRequest(socket_fd);
        } else if (strncmp(path, "/static/", 8) == 0) {
            handleStaticRequest(socket_fd, path);
        } else {
            handleNotFound(socket_fd);
        }
    } else {
        handleNotFound(socket_fd);
    }

    // http_client_message_t* http_msg;
    // http_read_result_t result;

    // read_http_message(socket_fd, &http_msg, &result);

    // if (result == BAD_REQUEST) {
    //     printf("Bad request\n");
    //     close(socket_fd);
    //     return;
    // } else if (result == CLOSED_CONNECTION) {
    //     printf("Closed connection\n");
    //     close(socket_fd);
    //     return;
    // }

    // respond_to_http_message(socket_fd, http_msg);
    // http_client_message_free(http_msg);


    printf("Done with connection %d\n", socket_fd);
    close(socket_fd);
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;

    if (argc > 2 && strcmp(argv[1], "-p") == 0)
    {
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
    if (err_code < 0)
    {
        perror("Bind failed");
        return 1;
    }

    // listen for connetions
    err_code = listen(server_fd, LISTEN_BACKLOG);
    if (err_code < 0)
    {
        perror("Listen failed");
        return 1;
    }

    // printf("Server ip is %d\n", server_address.sin_addr.s_addr);
    printf("Server listening on port %d\n", port);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while (1)
    {
        pthread_t thread;
        int* client_fd_buf = malloc(sizeof(int));

        *client_fd_buf = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (*client_fd_buf < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Accepted connection on %d\n", *client_fd_buf);

        pthread_create(&thread, NULL, (void* (*) (void*)) handleConnection, (void*) client_fd_buf);
    }
    close(server_fd);

    return 0;
}