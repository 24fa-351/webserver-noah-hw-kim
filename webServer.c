#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>

#define DEFAULT_PORT 8080
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024

// input pointer is freed by this function
void handleConnection(int *client_fd_ptr)
{
    int socket_fd = *client_fd_ptr;
    free(client_fd_ptr);
    
    printf("Handling connection on %d\n", socket_fd);

    bool done = false;
    
    while (done == false)
    {
        char buffer[BUFFER_SIZE];
        int bytes_read = read(socket_fd, buffer, sizeof(buffer));
        printf("Received: %s from connection %d\n", buffer, socket_fd);
        // write(socket_fd, buffer, bytes_read);

        int res = strncmp("exit", buffer, 4);
        if (res == 0)
        {
            done = true;
        }

        char path[1000];
        char method[1000];
        char http_version[1000];
        sscanf(buffer, "%s %s %s", method, path, http_version);

        if (strcmp(method, "GET") == 0) {
            if (strncmp(path, "/calc/", 6) == 0) {
                int a, b;
                sscanf(path, "/calc/%d/%d", &a, &b);
                dprintf(socket_fd, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>%d + %d = %d</h1></body></html>", a, b, a + b);
            } else if (strncmp(path, "/static/", 8) == 0) {
                printf("Serving static file\n");
            } else if (strncmp(path, "/stats/", 7) == 0) {
                printf("Serving stats\n");
            } else {
                dprintf(socket_fd, "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><body><h1>404 Not Found</h1></body></html>");
            }
        } else {
            dprintf(socket_fd, "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><body><h1>404 Not Found</h1></body></html>");
        }
    }

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