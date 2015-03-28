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
    int *buf;       // Buffer array
    int n;          // Maximum number of slots
    int front;      // buf[(front+1)%n] is first item
    int rear;       // buf[rear%n] is last item
    sem_t mutex;    // Protects accesses to buf
    sem_t slots;    // Counts available slots for insertion
    sem_t items;    // Counts available items for use
} sbuf_t;

//Halda utan um biðstólana í stofunni
struct chairs
{
    struct customer **customer; /* Array of customers */
    int max;
    //Leid til ad læsa þegar vid erum ad uppfæra 
    //t.d customer (fyrir ofan) gagnagrindina
    sem_t mutex;
    sem_t chair; 
    sem_t barber;

    sbuf_t barberShop; // Circular buffer - contains customer info if not empty

    /* TODO: Add more variables related to threads */
    /* Hint: Think of the consumer producer thread problem */
};

//Halda utan um barber
struct barber
{
    int room;
    struct simulator *simulator;
};

//Halda utan um allt saman a einum stad
struct simulator
{
    struct chairs chairs;
    
    pthread_t *barberThread;
    struct barber **barber;
};

// Sbuf helper functions
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

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

    /* Create chairs*/
    chairs->customer = malloc(sizeof(struct customer *) * thrlab_get_num_chairs());

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
    free(simulator->chairs.customer);

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
    
    //Byr til semaphoru(semaphora er lás - hleypur i gegn einum eda fleiri eftir hvernig vid skilgreinum lasinn)
    //sem_init er constructor fyrir semaphoru 
    sem_init(&customer->mutex, 0, 0);
    //Passa ad vid seum ekki ad samþykkja alla vidskiptavini - þarf að visa fra þeim sem komast ekki i sætid
     
    
    /* TODO: Accept if there is an available chair */
    //Bida eftir lausum stól
    //IFSETNINGsem_wait(&chairs->chair);
    //Passa ad þad se bara einn sem ma breyta semaphorunni (gagnagrindinni) í einu
    //IFSETNINGsem_wait(&chairs->mutex);
    //IFSETNINGthrlab_accept_customer(customer);
    //Halda utan um customer - finna bestu leid til ad halda utan um (ekki stack!)
    //engin virkni eins og er - tarf ad utfaera tad 
    //IFSETNINGchairs->customer[0] = customer;

    //Ferlid fra TO DO er ad vidskiptavinur kemur og bidur, 
    //eg samþykki hann og nu leyfi eg öðrum að koma inn:
    //IFSETNINGsem_post(&chairs->mutex);
     
    //IFSETNINGsem_post(&chairs->barber); 
    //Passa ad customer stingi ekki af - halda honum - getum læst honum 
    //IFSETNINGsem_wait(&customer->mutex); //nu er hann læstur
    
    /* TODO: Reject if there are no available chairs */
    //if setning sem segir 'ef tad er laust plass , ta accepta eg, annars rejecta tessum vidskiptavin
    //thrlab_reject_customer(customer);
    
    
    /* --- Sauðakóði
        IF (laust pláss)
            accepta viðskiptavin
        ELSE
            recjecta viðskiptavin
    */

    int slts;
    int itm;

    printf("Front: %d \n", chairs->barberShop.front);
    printf("Rear: %d \n", chairs->barberShop.rear);
    
    sem_getvalue(&chairs->barberShop.slots, &slts);
    sem_getvalue(&chairs->barberShop.items, &itm);

    printf("Slots: %d \n", slts);
    printf("Items: %d \n", itm);

    //if- setningin
    if(slts != 0)
    {
        //sem_wait(&customer->mutex);
    	sem_wait(&chairs->chair);
    	sem_wait(&chairs->mutex);

        thrlab_accept_customer(customer);
    	chairs->customer[0] = customer; // Ath laga Allir viðskipavinir yfirskrifa hvaða viðskiptavinur kemur næst

        sbuf_insert(&chairs->barberShop, (int)customer);

        printf("value: %d", *chairs->barberShop.buf);

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
    struct customer *customer = 0; /* TODO: Fetch a customer from a chair */

    /* Main barber loop */
    //
    while (true) 
    {
        sem_wait(&chairs->barber);
        sem_wait(&chairs->mutex);
        /* TODO: Here you must add you semaphores and locking logic */

        //customer = chairs->customer[0];     /* TODO: You must choose the customer */
        thrlab_prepare_customer(customer, barber->room);
        sem_post(&chairs->mutex);           //bætt vid i fyrirlestri
        sem_post(&chairs->chair);           //bætt vid, er samt i raun vitlaust , þvi vid turfum ad visa fra ef ekki laust sæti
        	
        thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));
        thrlab_dismiss_customer(customer, barber->room);
       
    	//Hækka semaphoruna / mun hleypa þessum customer af stad. Hleypa customer ur stofunni
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
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                              // Wait for available slot
    P(&sp->mutex);                              // Lock the buffer
    sp->buf[(++sp->rear) % (sp->n)] = item;     // Insert the item
    V(&sp->mutex);                              // Unlock the buffer
    V(&sp->items);                              // Announce available item
}

// Remove and return the first item for buffer sp                   ATH Þarf möguleg að skila struct en ekki int
int sbuf_remove(sbuf_t *sp)
{
    int item;

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
