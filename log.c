#include <stdlib.h>
#include <string.h>
#include "log.h"

#define INIT_BUFFER_SIZE 1024

// Opaque struct definition
struct Server_Log {
    // TODO: Implement internal log storage (e.g., dynamic buffer, linked list, etc.)
    char* buffer;         // Stores all log entries
    int current_size;             // Actual data length
    int total_capacity;         // Allocated buffer size

    // Reader-writer lock with writer priority
    pthread_mutex_t lock;
    pthread_cond_t read_allowed;
    pthread_cond_t write_allowed;
    int readers_inside;
    int writers_inside;
    int writers_waiting;
};

// Creates a new server log instance (stub)
server_log create_log() {
    server_log log = malloc(sizeof(struct Server_Log));
    if (!log) {
        return NULL;
    }
    log->total_capacity = INIT_BUFFER_SIZE;
    log->current_size = 0;
    log->buffer = malloc(INIT_BUFFER_SIZE);
    if (!log->buffer) {
        free(log);
        return NULL;
    }
    log->buffer[0] = '\0';

    pthread_mutex_init(&log->lock, NULL);
    pthread_cond_init(&log->read_allowed, NULL);
    pthread_cond_init(&log->write_allowed, NULL);

    log->readers_inside = 0;
    log->writers_inside = 0;
    log->writers_waiting = 0;
    return log;
    // TODO: Allocate and initialize internal log structure
    return (server_log)malloc(sizeof(struct Server_Log));
}

// Destroys and frees the log (stub)
void destroy_log(server_log log) {
    if (!log){
        return;
    }
    if (log->buffer){
        free(log->buffer);
    }
    pthread_mutex_destroy(&log->lock);
    pthread_cond_destroy(&log->write_allowed);
    pthread_cond_destroy(&log->read_allowed);
    free(log);
    // TODO: Free all internal resources used by the log
}

// Returns dummy log content as string (stub)
int get_log(server_log log, char** dst) {
    // TODO: Return the full contents of the log as a dynamically allocated string
    // This function should handle concurrent access
    printf("we are doing gettttttttttttttttttttt size is %d\n\n\n\n\n\n\n\n" ,log->current_size);
    pthread_mutex_lock(&log->lock);
    while (log->writers_inside > 0 || log->writers_waiting > 0){
        pthread_cond_wait(&log->read_allowed, &log->lock);
    }
    log->readers_inside++;
    pthread_mutex_unlock(&log->lock);
    /*
    const char* dummy = "Log is not implemented.\n";
    int len = strlen(dummy);
     */
    *dst = (char*)malloc(log->current_size + 1); // Allocate for caller
    if (*dst != NULL) {
        strcpy(*dst, log->buffer);
    }
    pthread_mutex_lock(&log->lock);
    log->readers_inside--;
    if(log->readers_inside == 0){
        pthread_cond_signal(&log->write_allowed);
    }
    pthread_mutex_unlock();
    return log->current_size;


}

// Appends a new entry to the log (no-op stub)
void add_to_log(server_log log, const char* data, int data_len) {
    pthread_mutex_lock(&log->lock);
    log->writers_waiting++;
    while(log->readers_inside > 0 || log->writers_inside > 0){
        pthread_cond_wait(&log->write_allowed, &log->lock);
    }
    log->writers_waiting--;
    log->writers_inside++;
    pthread_mutex_unlock(&log->lock);
    int new_length = log->current_size + data_len + 1;
    if (new_length > log->total_capacity){
        char* new_buffer = realloc(log->buffer, log->total_capacity * 2);
        if (new_buffer){
            log->buffer = new_buffer;
            log->total_capacity = log->total_capacity * 2;
        } else {
            //check this
            pthread_mutex_lock(&log->lock);
            log->writers_inside--;
            if(log->writers_inside == 0){
                pthread_cond_broadcast(&log->read_allowed);
                pthread_cond_signal(&log->write_allowed);
            }
            pthread_mutex_unlock(&log->lock);
            return;
        }
    }
    memcpy(log->buffer + log->current_size, data, data_len);
    log->current_size += data_len;
    log->buffer[log->current_size] = '\0';

    // check this
    pthread_mutex_lock(&log->lock);
    log->writers_inside--;
    if(log->writers_inside == 0){
        pthread_cond_broadcast(&log->read_allowed);
        pthread_cond_signal(&log->write_allowed);
    }
    pthread_mutex_unlock(&log->lock);

    // TODO: Append the provided data to the log
    // This function should handle concurrent access
}
