#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

typedef struct msg
{
    char *method;
    char *path;
    char *http_version;
    char *body;
    char *headers;
    int content_length;
} http_client_message_t;

typedef enum
{
    OK,
    BAD_REQUEST,
    CLOSED_CONNECTION
} http_read_result_t;

#define BUFFER_SIZE 1024

void read_http_message(int socket_fd, http_client_message_t **http_msg, http_read_result_t *result);
void respond_to_http_message(int socket_fd, http_client_message_t *http_msg);
void http_client_message_free(http_client_message_t *http_msg);
bool is_complete_http_message(char *buffer);

#endif
