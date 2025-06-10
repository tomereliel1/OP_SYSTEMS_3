#include "segel.h"
#include "request.h"
#include "log.h"
/* todo check this */
//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// Parses command-line arguments
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
}

void get_thread_number(int *thread_num, int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *thread_num = atoi(argv[2]);
}
void get_queue_size(int *queue_size, int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *queue_size = atoi(argv[3]);
}
// TODO: HW3 — Initialize thread pool and request queue
// This server currently handles all requests in the main thread.
// You must implement a thread pool (fixed number of worker threads)
// that process requests from a synchronized queue.


typedef struct {
    int connfd;
    struct timeval arrival_time;
} request_t;

typedef struct {
    request_t **requests;
    int front;
    int rear;
    int size_active;
    int size_pending;
    int max_size;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue_t;

queue_t request_queue;
server_log log_request;

void init_queue(queue_t* q, int size){
    q->requests = (request_t **) malloc(sizeof(request_t *) * size);
    q->front = 0;
    q->rear = 0;
    q->size_active = 0;
    q->size_pending =0;
    q->max_size = size;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void append_to_queue(queue_t *q, request_t* request){
    pthread_mutex_lock(&q->lock);
    while (q->size_active + q->size_pending == q->max_size){
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->requests[q->rear] = request;
    q->rear = (q->rear + 1) % q->max_size;
    q->size_pending += 1;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

request_t* remove_from_queue(queue_t *q){
    pthread_mutex_lock(&q->lock);
    while (q->size_pending == 0){
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    request_t* request = q->requests[q->front];
    q->front = (q->front + 1) % q->max_size;
    q->size_pending -= 1;
    q->size_active += 1;
    pthread_mutex_unlock(&q->lock);
    return request;
}

void update_finish_request(queue_t *q){
    pthread_mutex_lock(&q->lock);
    q->size_active--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

void *worker_thread(void* arg){
    /* todo remove condition */
    int i = 0;
    threads_stats t = malloc(sizeof(struct Threads_stats));
    t->id = pthread_self();             // Thread ID (placeholder)
    t->stat_req = 0;       // Static request count
    t->dynm_req = 0;       // Dynamic request count
    t->total_req = 0;      // Total request count
    struct timeval dispatch;
    while(1){
        request_t* request = remove_from_queue(&request_queue);
        gettimeofday(&dispatch, NULL);
        int connfd = request->connfd;
        struct timeval arrival = request->arrival_time;
        requestHandle(connfd, arrival, dispatch, t, log_request);
        update_finish_request(&request_queue);
        free(request);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // Create the global server log_request
    log_request = create_log();
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    int thread_num;
    int queue_size;
    getargs(&port, argc, argv);
    get_thread_number(&thread_num, argc, argv);
    get_queue_size(&queue_size, argc, argv);
    init_queue(&request_queue, queue_size);
    pthread_t* pool = (pthread_t*) malloc(sizeof(pthread_t) * thread_num);

    int i;
    for (i = 0; i < thread_num ; i++){
        if (pthread_create(&pool[i], NULL, worker_thread, NULL) != 0){
            exit(-1);
        }
    }



    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t * ) & clientlen);
        struct timeval arrival;
        gettimeofday(&arrival, NULL);
        request_t *request = (request_t *)malloc(sizeof(request_t));
        request->connfd = connfd;
        request->arrival_time = arrival;
        append_to_queue(&request_queue, request);
        // TODO: HW3 — Record the request arrival time here
    }
}

/*
        // DEMO PURPOSE ONLY:
        // This is a dummy request handler that immediately processes
        // the request in the main thread without concurrency.
        // Replace this with logic to enqueue the connection and let
        // a worker thread process it from the queue.

        threads_stats t = malloc(sizeof(struct Threads_stats));
        t->id = 0;             // Thread ID (placeholder)
        t->stat_req = 0;       // Static request count
        t->dynm_req = 0;       // Dynamic request count
        t->total_req = 0;      // Total request count

        struct timeval arrival, dispatch;
        arrival.tv_sec = 0; arrival.tv_usec = 0;   // DEMO: dummy timestamps
        dispatch.tv_sec = 0; dispatch.tv_usec = 0; // DEMO: dummy timestamps
        // gettimeofday(&arrival, NULL);

        // Call the request handler (immediate in main thread — DEMO ONLY)
        requestHandle(connfd, arrival, dispatch, t, log);

        free(t); // Cleanup
        Close(connfd); // Close the connection
    }

    // Clean up the server log before exiting
    destroy_log(log);

    // TODO: HW3 — Add cleanup code for thread pool and queue
worker()

 */
