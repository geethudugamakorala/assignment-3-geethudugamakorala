#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    // Initialize the completion status to false
    thread_func_args->thread_complete_success = false;
    
    // Wait before attempting to obtain the mutex
    if (thread_func_args->wait_to_obtain_ms > 0) {
        DEBUG_LOG("Waiting %d ms before attempting to obtain mutex", thread_func_args->wait_to_obtain_ms);
        usleep(thread_func_args->wait_to_obtain_ms * 1000); // Convert ms to microseconds
    }
    
    // Obtain the mutex
    DEBUG_LOG("Attempting to obtain mutex");
    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0) {
        ERROR_LOG("Failed to obtain mutex, error code %d", rc);
        return thread_param;
    }
    DEBUG_LOG("Successfully obtained mutex");
    
    // Wait while holding the mutex
    if (thread_func_args->wait_to_release_ms > 0) {
        DEBUG_LOG("Waiting %d ms before releasing mutex", thread_func_args->wait_to_release_ms);
        usleep(thread_func_args->wait_to_release_ms * 1000); // Convert ms to microseconds
    }
    
    // Release the mutex
    DEBUG_LOG("Releasing mutex");
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0) {
        ERROR_LOG("Failed to release mutex, error code %d", rc);
        return thread_param;
    }
    DEBUG_LOG("Successfully released mutex");
    
    // Mark completion as successful
    thread_func_args->thread_complete_success = true;
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    
    // Allocate memory for thread_data structure
    struct thread_data *thread_data_ptr = malloc(sizeof(struct thread_data));
    if (thread_data_ptr == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }
    
    // Setup the thread data with mutex and wait arguments
    thread_data_ptr->mutex = mutex;
    thread_data_ptr->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data_ptr->wait_to_release_ms = wait_to_release_ms;
    thread_data_ptr->thread_complete_success = false;
    
    // Create the thread using threadfunc as entry point
    int rc = pthread_create(thread, NULL, threadfunc, thread_data_ptr);
    if (rc != 0) {
        ERROR_LOG("Failed to create thread, error code %d", rc);
        free(thread_data_ptr); // Clean up allocated memory on failure
        return false;
    }
    
    DEBUG_LOG("Successfully created thread with wait_to_obtain_ms=%d, wait_to_release_ms=%d", 
              wait_to_obtain_ms, wait_to_release_ms);
    
    return true;
}

