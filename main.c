#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in below.
 *
 * === User information ===
 * User 1: arnara13 
 * SSN: 060889-3339
 * User 2: hafthort12
 * SSN: 220492-2819
 * === End User Information ===
 ********************************************************/

static void *barber_work(void *arg);

typedef struct 
{
    struct customer **buf;       // Buffer array
    int n;          // Maximum number of slots
    int front;      // buf[(front+1)%n] is first item
    int rear;       // buf[rear%n] is last item
    sem_t mutex;    // Protects accesses to buf
    sem_t slots;    // Counts available slots for insertion
    sem_t items;    // Counts available items for use
} sbuf_t;

struct chairs
{
    int max;    
    sem_t mutex;
    sem_t chair; 
    sem_t barber;
    sbuf_t barberShop; // Circular buffer - contains customer info if not empty
};

struct barber
{
    int room;
    struct simulator *simulator;
};

struct simulator
{
    struct chairs chairs;
    
    pthread_t *barberThread;
    struct barber **barber;
};

// Sbuf helper functions
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, struct customer *);
struct customer* sbuf_remove(sbuf_t *sp);

//Semaphores helper functions
void P(sem_t *sem) { sem_wait(sem); }
void V(sem_t *sem) { sem_post(sem); }

/**
 * Initialize data structures and create waiting barber threads.
 */
static void setup(struct simulator *simulator)
{
    struct chairs *chairs = &simulator->chairs;

    /* Setup semaphores*/
    chairs->max = thrlab_get_num_chairs();
    
    //Upphafsstilla mutex, chair og barber
    sem_init(&chairs->mutex, 0, 1);
    sem_init(&chairs->chair, 0, thrlab_get_num_chairs());
    sem_init(&chairs->barber, 0, 0);

    // Sbuf circular buffer for waiting room chairs
    sbuf_init(&chairs->barberShop, thrlab_get_num_chairs());
    	
    /* Create barber thread data */
    simulator->barberThread = malloc(sizeof(pthread_t) * thrlab_get_num_barbers());
    simulator->barber = malloc(sizeof(struct barber*) * thrlab_get_num_barbers());

    /* Start barber threads */
    struct barber *barber;
    for (unsigned int i = 0; i < thrlab_get_num_barbers(); i++) 
    {
    	barber = calloc(sizeof(struct barber), 1);
    	barber->room = i;
    	barber->simulator = simulator;
    	simulator->barber[i] = barber;
    	pthread_create(&simulator->barberThread[i], 0, barber_work, barber);
    	pthread_detach(simulator->barberThread[i]);
    }
}

/**
 * Free all used resources and end the barber threads.
 */
static void cleanup(struct simulator *simulator)
{
    /* Free chairs */
    sbuf_deinit(&simulator->chairs.barberShop);

    /* Free barber thread data */
    free(simulator->barber);
    free(simulator->barberThread);
}

/**
 * Called in a new thread each time a customer has arrived.
 */
static void customer_arrived(struct customer *customer, void *arg)
{
    struct simulator *simulator = arg;
    struct chairs *chairs = &simulator->chairs;
    
    sem_init(&customer->mutex, 0, 0);

    /* --- Sauðakóði
        IF (laust pláss)
            accepta viðskiptavin
        ELSE
            recjecta viðskiptavin
    */
    int slts;
    int itm;
    sem_getvalue(&chairs->barberShop.slots, &slts);
    sem_getvalue(&chairs->barberShop.items, &itm);

    //if- setningin
    if(slts != 0)
    {
    	sem_wait(&chairs->chair);
    	sem_wait(&chairs->mutex);

        sbuf_insert(&chairs->barberShop, customer);

        thrlab_accept_customer(customer);

    	sem_post(&chairs->mutex);
    	sem_post(&chairs->barber);

    	sem_wait(&customer->mutex);
    }
    else
    {
	   thrlab_reject_customer(customer);
    }
}

static void *barber_work(void *arg)
{
    struct barber *barber = arg;
    struct chairs *chairs = &barber->simulator->chairs;
    struct customer *customer;

    /* Main barber loop */
    while (true) 
    {
        sem_wait(&chairs->barber);
        sem_wait(&chairs->mutex);

        customer = sbuf_remove(&chairs->barberShop);

        thrlab_prepare_customer(customer, barber->room);
        sem_post(&chairs->mutex);
        sem_post(&chairs->chair);
        	
        thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));
        thrlab_dismiss_customer(customer, barber->room);
       
    	sem_post(&customer->mutex);
    }
    return NULL;
}
// Create an empty, bounded, shared FIFO (queue) buffer with n slots
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = calloc(n, sizeof(int));
    sp->n = n;                      // Buffer holds max of n items
    sp->front = sp->rear = 0;       // Empty buffer is front == rear
    sem_init(&sp->mutex, 0, 1);     // Binary semaphore for locking
    sem_init(&sp->slots, 0, n);     // Initially, buf has n empty slots
    sem_init(&sp->items, 0, 0);     // Initially, buf has zero items
}
 
// Clean up buffer sp
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}

// Insert item onto the rear of shared buffer sp
void sbuf_insert(sbuf_t *sp, struct customer *customer)
{
    P(&sp->slots);                              // Wait for available slot
    P(&sp->mutex);                              // Lock the buffer
    sp->buf[(++sp->rear) % (sp->n)] = customer; // Insert the item
    V(&sp->mutex);                              // Unlock the buffer
    V(&sp->items);                              // Announce available item
}

// Remove and return the first item for buffer sp
struct customer* sbuf_remove(sbuf_t *sp)
{
    struct customer *item;

    P(&sp->items);                              // Wait for available item
    P(&sp->mutex);                              // Lock the buffer
    item = sp->buf[(++sp->front) % (sp->n)];    // Remove the item
    V(&sp->mutex);                              // Unlock the buffer
    V(&sp->slots);                              // Announce the available slot

    return item;                                // Return the item
}

int main (int argc, char **argv)
{
    struct simulator simulator;

    thrlab_setup(&argc, &argv);
    setup(&simulator);

    thrlab_wait_for_customers(customer_arrived, &simulator);

    thrlab_cleanup();
    cleanup(&simulator);

    return EXIT_SUCCESS;
}
