#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "httpMessage.h"


void read_http_message(int socket_fd, http_client_message_t **http_msg, http_read_result_t *result) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(socket_fd, buffer, sizeof(buffer));

    if (bytes_read == 0) {
        *result = CLOSED_CONNECTION;
        return;
    }

    if (bytes_read < 0) {
        perror("Read failed");
        *result = BAD_REQUEST;
        return;
    }

    if (!is_complete_http_message(buffer)) {
        *result = BAD_REQUEST;
        return;
    }

    *result = OK;

    sscanf(buffer, "%s %s %s", (*http_msg)->method, (*http_msg)->path, (*http_msg)->http_version);

    // sscanf(buffer, "%s %s %s %s %s %d", (*http_msg)->method, (*http_msg)->path, (*http_msg)->http_version, (*http_msg)->body, (*http_msg)->headers, &(*http_msg)->content_length);

    printf("Method: %s\n", (*http_msg)->method);
    printf("Path: %s\n", (*http_msg)->path);
    printf("HTTP Version: %s\n", (*http_msg)->http_version);
    printf("Body: %s\n", (*http_msg)->body);
    printf("Headers: %s\n", (*http_msg)->headers);
    printf("Content Length: %d\n", (*http_msg)->content_length);

};

void respond_to_http_message(int socket_fd, http_client_message_t *http_msg) {

};

void http_client_message_free(http_client_message_t *http_msg) {
    free(http_msg->method);
    free(http_msg->path);
    free(http_msg->http_version);
    free(http_msg->body);
    free(http_msg->headers);
    free(http_msg);
};

bool is_complete_http_message(char* buffer) {
    if (strlen(buffer) < 10) {
        return false;
    } else if (strncmp(buffer, "GET", 3) != 0) {
        return false;
    } 
    
    // else if (strncmp(buffer + strlen(buffer) - 2, "\n\n", 2) != 0) {
    //     return false;
    // }
    return true;
}