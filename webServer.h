#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#define DEFAULT_PORT 8080
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define MAX_METHOD_LENGTH 16
#define MAX_PATH_LENGTH 256
#define MAX_HTTP_VERSION_LENGTH 16

void printNotFound(int socket_fd);
void printBadRequest(int socket_fd);
void parseHttpMessage(char *buffer, char *method, char *path, char *http_version);
void handleHttpMessage(int socket_fd, char *buffer);
void handleCalcRequest(int socket_fd, char *path);
void handleStatsRequest(int socket_fd);
void handleStaticRequest(int socket_fd, char *path);
void handleConnection(int *client_fd_ptr);

#endif