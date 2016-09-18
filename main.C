// Felipe de Albuquerque Mello Pereira
// Project 1 of 'Concurrent and Parallel Programming' course


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define SHARED 1
#define INFINITY -1

// Define global vars used to statically allocate vectors. If you want to pass these through the
// command line, they should be allocated dynamically with malloc.
#define NUM_READERS 2
#define NUM_WRITERS 2
#define BUFFER_SIZE 4

// With 'INFINITY' below it will only stop after an int was wrapped around and is about to get to 0.
// It's not really infinity, but for our purposes should be fine. If you want real infinity, change
// 'producer' and 'consumer' functions to have a 'while(true) {...}' instead.
// If you choose a different number, the writer threads will eventually exit, joining the main
// thread, while the reader threads will be left waiting forever.
static int const NUMBER_TOTAL_MESSAGES_PER_THREAD = INFINITY;//100;
static int buffer[BUFFER_SIZE];
static int next_free = 0;
static int oldest_unread = 0;
static bool is_empty = true;
static int number_read[BUFFER_SIZE];

static int next_read[NUM_READERS];
static bool is_up_to_date[NUM_READERS];

static sem_t em;
static sem_t sem_reader;
static sem_t sem_writer;
static int readers_waiting = 0;
static int writers_waiting = 0;

static int active_readers = 0;
static int active_writers = 0;

static pthread_t producer_threads[NUM_WRITERS];
static pthread_t consumer_threads[NUM_READERS];


int inc(int curr_value) {
    // Increments curr_value and wraps around the buffer if needed.

    return (curr_value + 1) % BUFFER_SIZE;
}

void wait_for(unsigned int secs) {
    // Waits for at least 'secs' seconds before returning.
    // Used to make sure stdout catches up with its buffer.

    // Uncomment the lines below if you want to be able to follow how the code is working.
    // unsigned int retTime = time(0) + secs;
    // while (time(0) < retTime);
    return;
}

void read_msg(int buffer_index, int cid) {
    // Prints message number, reader's thread number, buffer index and semaphores' values.

    printf("Reader Thread #%d read int %d from index %d\n",
           cid,
           buffer[buffer_index],
           buffer_index);
    int sem_value;
    sem_getvalue(&em, &sem_value);
    printf("em = %d\n", sem_value);
    sem_getvalue(&sem_reader, &sem_value);
    printf("sem_reader = %d\n", sem_value);
    sem_getvalue(&sem_writer, &sem_value);
    printf("sem_writer = %d\n", sem_value);
    wait_for(1);

    return;
}

void write_msg(int buffer_index, int message_number, int pid) {
    // Saves message in buffer. Also prints message number, writer's thread number, buffer index
    // and semaphores' values.

    buffer[buffer_index] = message_number;
    printf("Writer Thread #%d wrote int %d at index %d\n",
           pid,
           message_number,
           buffer_index);
    int sem_value;
    sem_getvalue(&em, &sem_value);
    printf("em = %d\n", sem_value);
    sem_getvalue(&sem_reader, &sem_value);
    printf("sem_reader = %d\n", sem_value);
    sem_getvalue(&sem_writer, &sem_value);
    printf("sem_writer = %d\n", sem_value);
    wait_for(1);

    return;
}

void signal() {
    // Baton passing
    // Also prints which kind of thread got the baton.

    if ((next_free != oldest_unread || is_empty) && writers_waiting > 0) {
        // Note that next_free == oldest_unread only when the buffer is full and when the buffer is
        // empty. next_free != oldest_unread when the buffer is only half-full.
        printf("PASSED THE BATON TO A WRITER\n\n");
        wait_for(1);
        writers_waiting--;
        sem_post(&sem_writer);
    } else if (!is_empty && readers_waiting > 0) {
        printf("PASSED THE BATON TO A READER\n\n");
        wait_for(1);
        readers_waiting--;
        sem_post(&sem_reader);
    } else {
        printf("RELEASED EM\n\n");
        wait_for(1);
        sem_post(&em);
    }

    return;
}

void deposit(int* buffer, int number_to_write, int pid) {
    // Awaits until there is free space in buffer. Then it writes the message and updates global
    // variables indicating that all reader threads have something to read, that none has read this
    // message and that the buffer it not empty (in case it was before).

        /*
        <await (next_free != oldest_unread || is_empty)
            write_msg(number_to_write);
            number_read[next_free] = 0;
            if (is_empty) {
                is_empty = false;
            }
            next_free = inc(next_free);
            for (int cid = 0; cid < NUM_READERS; cid++) {
                is_up_to_date[cid] = false;
            }
        >
        */

        sem_wait(&em);
        printf("Writer Thread #%d got EM.\n", pid);
        wait_for(1);
        if (next_free == oldest_unread && !is_empty) {
            // Note that next_free == oldest_unread only when the buffer is full and when the buffer
            // is empty.
            printf("Writer Thread #%d got into the writer's waitlist\n\n", pid);
            wait_for(1);
            writers_waiting++;
            sem_post(&em);
            wait_for(1);
            sem_wait(&sem_writer);
        }
        printf("Writer Thread #%d is now active.\n", pid);
        write_msg(next_free, number_to_write, pid);
        number_read[next_free] = 0; // no reader thread has read this new message
        if (is_empty) {
            is_empty = false;
            printf("is_empty = %d\n", is_empty);
        }
        next_free = inc(next_free);
        for (int cid = 0; cid < NUM_READERS; cid++) {
            // No reader threads are up to date with their reading, as of this moment
            is_up_to_date[cid] = false;
        }
        if (next_free == oldest_unread && !is_empty) {
            printf("Buffer is now FULL!\n");
        }
        signal();

        return;
}

void consume(int* buffer, int cid) {
    // Awaits until we have a message in the buffer which we still haven't read. After reading,
    // it updates global variables indicating how many reader threads have read this message,
    // what's the oldest message in the buffer which a reader thread still haven't read and that
    // this thread has already read everything that was available to it.

        /*
        <await (!is_empty && !is_up_to_date[cid])
            if (!is_up_to_date[cid]) {
                do {
                    read_msg(next_read[cid], cid);
                    number_read[next_read[cid]]++;
                    if (number_read[next_read[cid]] == NUM_READERS) {
                        oldest_unread = inc(oldest_unread);
                        if (inc(next_read[cid]) == next_free) {
                            is_empty = true;
                        }
                    }
                    next_read[cid] = inc(next_read[cid]);
                } while(next_read[cid] != next_free);
                is_up_to_date[cid] = true;
            }
        >
        */

        sem_wait(&em);
        printf("Reader Thread #%d got the EM.\n", cid);
        wait_for(1);
        if (is_empty || is_up_to_date[cid]) {
            // 'is_up_to_date[cid]' is actually not necessary here, but it helps garantee liveness.
            // If it wasn't here, then when the buffer is full, the first reader thread could read
            // all messages, release the semaphore and still be the one who gets it again. This way,
            // the other reader thread could be left waiting for the semaphore indefinitely. By
            // using '|| is_up_to_date[cid]' above, we are garanteed this does not happen, since
            // this thread would be left in the reader's waitlist and the other thread would finally
            // pick the semaphore.
            printf("Reader Thread #%d got into the reader's waitlist.\n\n", cid);
            wait_for(1);
            readers_waiting++;
            sem_post(&em);
            wait_for(1);
            sem_wait(&sem_reader);
        }
        printf("Reader Thread #%d is now active.\n", cid);
        wait_for(1);
        if (!is_up_to_date[cid]) {
            // This thread still has at least one message to read.
            // Actually the above if is always true since we give preference to writers in signal().
            do {
                read_msg(next_read[cid], cid);
                number_read[next_read[cid]]++;
                if (number_read[next_read[cid]] == NUM_READERS) {
                    // This thread was the last one to read this message
                    oldest_unread = inc(oldest_unread);
                    if (inc(next_read[cid]) == next_free) {
                        // This was the last message available
                        is_empty = true;
                        printf("is_empty = %d\n", is_empty);
                    }
                }
                next_read[cid] = inc(next_read[cid]);
            } while(next_read[cid] != next_free);
            is_up_to_date[cid] = true;
        }
        signal();

        return;
}

void* producer(void* args) {
    int thread_number = *((int*)args);
    int message_number = 1;
    while (message_number != NUMBER_TOTAL_MESSAGES_PER_THREAD + 1) {
        deposit(buffer, message_number, thread_number);
        message_number++;
    }
    printf("Writer thread #%d FINISHED!\n\n", thread_number);
    return NULL;
}

void* consumer(void* args) {
    int thread_number = *((int*)args);
    while (true) {
        consume(buffer, thread_number);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Let's make stdout not use a buffer
    setbuf(stdout, NULL);

    // Init global vars
    for (int cid = 0; cid < NUM_READERS; cid++) {
        next_read[cid] = 0;
        is_up_to_date[cid] = true;
    }

    for (int buffer_index = 0; buffer_index < BUFFER_SIZE; buffer_index++) {
        buffer[buffer_index] = 0;
        number_read[buffer_index] = 0;
    }

    sem_init(&em, SHARED, 1);
    sem_init(&sem_reader, SHARED, 0);
    sem_init(&sem_writer, SHARED, 0);


    // Start threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    for (int pid = 0; pid < NUM_WRITERS; pid++) {
        int* local_pid = (int*) malloc(sizeof(int));
        if (local_pid == NULL) {
            return 1;
        }
        *local_pid = pid;
        pthread_create(&producer_threads[pid], &attr, producer, (void*)local_pid);
    }
    for (int cid = 0; cid < NUM_READERS; cid++) {
        int* local_cid = (int*) malloc(sizeof(int));
        if (local_cid == NULL) {
            return 1;
        }
        *local_cid = cid;
        pthread_create(&consumer_threads[cid], &attr, consumer, (void*)local_cid);
    }
    // Join threads
    for (int pid = 0; pid < NUM_WRITERS; pid++) {
        pthread_join(producer_threads[pid], NULL);
    }
    for (int cid = 0; cid < NUM_READERS; cid++) {
        pthread_join(consumer_threads[cid], NULL);
    }
    // No free for local_pid's and local_cid's since (at least some) joins never finish. Anyway,
    // let's leave that to the OS.
    return 0;
}
