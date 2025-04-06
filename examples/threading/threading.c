#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

/**
 * This function is run by the created thread.
 * It performs the following steps:
 * 1. Waits for a defined time (before trying to obtain the mutex)
 * 2. Locks the mutex
 * 3. Waits again (while holding the mutex)
 * 4. Unlocks the mutex
 * 5. Sets a success flag in the thread_data struct
 */
void* threadfunc(void* thread_param)
{
    // Cast the thread_param to the expected thread_data struct
    struct thread_data* data = (struct thread_data *) thread_param;

    // Sleep before attempting to obtain the mutex
    usleep(data->wait_to_obtain_ms * 1000);

    // Attempt to lock the mutex
    if (pthread_mutex_lock(data->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex");
        data->thread_complete_success = false;
        return thread_param;
    }

    // Sleep again while holding the mutex
    usleep(data->wait_to_release_ms * 1000);

    // Attempt to unlock the mutex
    if (pthread_mutex_unlock(data->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex");
        data->thread_complete_success = false;
        return thread_param;
    }

    // Set success flag
    data->thread_complete_success = true;

    return thread_param;
}

/**
 * This function creates a new thread and provides it with
 * a dynamically allocated thread_data structure.
 * 
 * It performs the following steps:
 * 1. Allocates memory for thread_data
 * 2. Fills in the struct fields with provided arguments
 * 3. Starts the thread with pthread_create()
 * 
 * Returns true if thread creation was successful.
 */
bool start_thread_obtaining_mutex(pthread_t *thread,
                                  pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms,
                                  int wait_to_release_ms)
{
    // Allocate memory for thread_data
    struct thread_data *data = malloc(sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }

    // Fill in the struct fields
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    // Create the thread, passing in the threadfunc and the thread_data
    int rc = pthread_create(thread, NULL, threadfunc, data);
    if (rc != 0) {
        ERROR_LOG("Failed to create thread (error %d)", rc);
        free(data);  // Clean up on failure
        return false;
    }

    return true;
}

