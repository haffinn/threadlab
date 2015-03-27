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
 * User 2:
 * SSN:
 * === End User Information ===
 ********************************************************/

static void *barber_work(void *arg);
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
    sem_init(&chairs->chair, 0, 1);
    sem_init(&chairs->barber, 0, 0); 

    /* Create chairs*/
    chairs->customer = malloc(sizeof(struct customer *) * thrlab_get_num_chairs());
    	
    /* Create barber thread data */
    simulator->barberThread = malloc(sizeof(pthread_t) * thrlab_get_num_barbers());
    simulator->barber = malloc(sizeof(struct barber*) * thrlab_get_num_barbers());

    /* Start barber threads */
    struct barber *barber;
    for (unsigned int i = 0; i < thrlab_get_num_barbers(); i++) {
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
    
    
    //if- setningin
    if(thrlab_get_num_chairs() == 0){
	sem_wait(&chairs->chair);
	sem_wait(&chairs->mutex);
    	thrlab_accept_customer(customer);
	chairs->customer[0] = customer;
	sem_post(&chairs->mutex);
	sem_post(&chairs->barber);
	sem_wait(&customer->mutex);
    }else
	thrlab_reject_customer(customer);
}

static void *barber_work(void *arg)
{
    struct barber *barber = arg;
    struct chairs *chairs = &barber->simulator->chairs;
    struct customer *customer = 0; /* TODO: Fetch a customer from a chair */

    /* Main barber loop */
    //
    while (true) {
	sem_wait(&chairs->barber);
	sem_wait(&chairs->mutex);
        /* TODO: Here you must add you semaphores and locking logic */
        customer = chairs->customer[0]; /* TODO: You must choose the customer */
        thrlab_prepare_customer(customer, barber->room);
        sem_post(&chairs->mutex);//bætt vid i fyrirlestri
	sem_post(&chairs->chair);//bætt vid, er samt i raun vitlaust , þvi vid turfum ad visa fra ef ekki laust sæti
	
	thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));
        thrlab_dismiss_customer(customer, barber->room);
       
	//Hækka semaphoruna / mun hleypa þessum customer af stad. Hleypa customer ur stofunni
	sem_post(&customer->mutex);
    }
    return NULL;
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
