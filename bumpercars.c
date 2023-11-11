#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <stdint.h>
#include <stdbool.h>

#define N_CARS 2
#define N_RIDERS 5
#define T_WANDER 20 /* each wandering time is between 0 to T_WANDER */
#define T_BUMP 4 /* each bumping time is between 0 to T_BUMP */
int COUNT_DOWN = 10; /* Number of bumper car rides */

//Semaphores
sem_t WaitForRideBegin[N_RIDERS + 1];
sem_t WaitForRideOver[N_RIDERS + 1];
sem_t waitingLineMutux;
sem_t lineAccessMutex;
sem_t exitConditionMutux;
sem_t startRideMutux; 

int LineHead = 0; // Next rider
int LineIndex = 0; // Line iteration variable
int amountofRiders[N_RIDERS]; // riders in line
int WaitArea[N_RIDERS]; // the riders in the actual line
int currentRider[N_CARS]; // riders in cars

// Boolean function to determine if COUNT_DOWN rides have ran 
int finish() {
    if (COUNT_DOWN==0)
        return 1; 
    else         
        return 0;
}

/*
Function that sends a rider into the next aviable spot in line
The function uses the waitingLineMutux to ensure that only one thread can modify the waiting line at a time. This prevents a race condition where multiple riders update the line at the same time.
*/
void GetInLine(int rid) 
{
    while (sem_wait(&waitingLineMutux) != 0); // Wait for access to the waiting line mutex

    printf("Rider %d gets in the waiting line.\n", rid);

    // Add the rider to the waiting line
    WaitArea[LineIndex] = rid;
    LineIndex = (LineIndex + 1) % 5;

    // Set the number of riders for this particular rider
    amountofRiders[rid - 1] = 2;

    sem_post(&waitingLineMutux); // Release the waiting line mutex
}

/*
Function that moves the rider from the front of the line into a bumber car. The semaphore WaitForRideBegin prevents a race condition where the next rider in the bumper car is unpredicatble.
*/
void TakeASeat(int rid)
{
    while (sem_wait(&WaitForRideBegin[rid]) != 0); // Wait for the ride to be available

    amountofRiders[rid - 1] = 1; // Set the number for this particular rider
    printf("Rider %d is taking a seat.\n", rid);

    // Release the lock, at this point the next queued rider may enter
    sem_post(&WaitForRideBegin[rid]);
}


/*
Function that simulates a rider taking a bumper car ride. The semaphore startRideMutux is used to signal the start of the ride to prevent a race condition where the bumper car might start the ride based on signals from multiple riders, leading to unpredictable behavior and unintended order from the queue. Additionally, the semaphore WaitForRideOver is used to prevent multiple riders attempting to update their status after signaling the start of the ride.
*/
void TakeARide(int rid) 
{
    printf("Rider %d is taking the ride.\n", rid);

    sem_post(&startRideMutux);  // Signal the bumper car to start the ride

    while (sem_wait(&WaitForRideOver[rid]) != 0); // Wait for the ride to be over

    amountofRiders[rid - 1] = 0; // Set the number of riders to (not in a car)
}

/*
Function for the rider to wander a random time before entering the line, no semaphore is needed as the code is not critical, i.e. there is no interaction among other threads.
*/
void Wander(int rid, int interval) 
{
    printf("Rider %d is wandering around the park.\n", rid);
    sleep(interval);
}

/*
Function that simulates loading a rider into a bumper car.

The lineAccessMutex semaphore is used to ensure exclusive access to shared data during the loading process. Such as if Car 1 and Car 2 both check if there is a rider at the front of the line simultaneously. The waitingLineMutux semaphore is used to ensure exclusive access to the waiting line when a rider gets in line. This could result in two riders being assigned the same spot in the waiting line.
*/
void Load(int cid) 
{
    // Wait until there is at least one rider in the line
    while (!WaitArea[LineHead]);

    // Wait for access to the lineAccessMutex semaphore
    while (sem_wait(&lineAccessMutex) != 0);

    // Get the rider at the front of the line
    int this_guy = WaitArea[LineHead]; 

    // Update the waiting line and rider status
    WaitArea[LineHead] = 0;  
    amountofRiders[this_guy - 1] = 1;
    LineHead = (LineHead + 1) % 5; 
    currentRider[cid - 1] = this_guy; 

    printf("Car %d takes the rider %d.\n", cid, this_guy);

    // Release the lineAccessMutex semaphore
    sem_post(&lineAccessMutex);

    // Signal the WaitForRideBegin semaphore to indicate the ride has begun for the rider
    sem_post(&WaitForRideBegin[this_guy]);
}

/*
Function that simulates the end of a bumper car ride.
The semaphore prevents a race condition where multiple cars might try to signal the end of the ride for the same rider simultaneously, or if two cars finish their rides simultaneously, both might update the status of the same rider.
*/
void Unload(int cid) 
{
    printf("This ride of Car %d is over.\n", cid);

    // If the car is Car 1 or Car 2, signal the WaitForRideOver semaphore for the corresponding rider
    if (cid == 1 || cid == 2) {
        sem_post(&WaitForRideOver[currentRider[cid - 1]]);
    }

    // Update currentRider array to indicate that the car is no longer carrying a rider
    currentRider[cid - 1] = 0;
}


/*
   Function that simulates the bumping action during a bumper car ride.
   It waits for the startRideMutux semaphore then it sleeps for a specified interval to simulate the bumping time. The semaphore line ensures that a car must wait until the startRideMutux semaphore is available before proceeding.
*/
void Bump(int cid, int interval)
{
    // Wait for the startRideMutux semaphore to indicate the start of the bumping action
    while(sem_wait(&startRideMutux) != 0);

    // Print a message indicating that the rider in the car is bumping
    printf("This rider of Car %d is bumping.\n", cid);

    // Sleep for the specified interval to simulate the bumping time
    sleep(interval);
}


/*
   Bumper car thread function.

   The car continuously goes through the cycle of loading a rider, bumping, and unloading.
   The COUNT_DOWN variable is decreased after each ride, and the thread exits when it reaches 0.
   
   The semaphore Prevents multiple cars from simultaneously checking and modifying the waiting line and other shared data.
*/
void *Car(void *arg)
{
    int carID = (intptr_t)arg; // Cast the argument back to an integer, see read me file

    while (true)
    {
        Load(carID);
        Bump(carID, random() % T_BUMP);
        Unload(carID);

        // Ride is over
        COUNT_DOWN--;

        // Wait for the exitConditionMutux semaphore to check the exit condition
        while (sem_wait(&exitConditionMutux) != 0);

        if (finish())
            exit(0);

        // Release the exitConditionMutux semaphore to allow other threads to check the exit condition
        sem_post(&exitConditionMutux);
    }
    pthread_exit(0); // Exit the thread
}
/* 
Rider thread function. This function represents a rider's behavior.

The rider first wanders around the park for a random amount of time.

Then, the rider gets in line by calling the GetInLine function, uses the waitingLineMutux semaphore. 

After getting in line, the rider takes a seat by calling the TakeASeat function, which utilizes the 
WaitForRideBegin semaphore to avoid a race condition.

Finally, the rider takes a bumper car ride using the TakeARide function, which uses the semaphores 
startRideMutux and WaitForRideOver to multiple riders signaling the start and end of a other's rides.

The rider thread continues this cycle until the program finishes, as determined by the finish 
function.
*/
void *Rider(void *arg)
{
    int rid = (intptr_t)arg; // Cast the argument back to an integer, see read me file

    while (true)
    {
        /* Wander around for a random amount of time. Cast as an int to pass into functions */
        Wander(rid, random() % T_WANDER);

        GetInLine(rid);
        TakeASeat(rid);
        TakeARide(rid);

        /* Check for the condition to exit the while loop */
        while (sem_wait(&exitConditionMutux) != 0);

        /* Check if the program is over and exit if true */
        if (finish())
        {
            exit(0);
        }

        sem_post(&exitConditionMutux); // Release the lock
    }
    pthread_exit(0); // Exit the thread when finished
}

/* 
Displaying information about the state of each rider 
The function doesn't perform any critical operations that require synchronization with other threads. Therefore, there is no need to use semaphores
*/
void *Display(void *dummy) {
    while(!finish())
    {
        printf("The current situation in the park is:\nThere are %i rides left\n", COUNT_DOWN);
        for (int i=0; i < N_CARS; i++)
        {
            if(currentRider[i] !=0)
                printf("Car %d is running. The rider is %d\n", i + 1, currentRider[i]);
            else
                printf("Car %d is not running.\n", i + 1);
        }
        for (int i=0; i < N_RIDERS; i++)
        {
            if(amountofRiders[i] == 0)
                printf("Rider %d is wandering\n", i + 1);
            else if(amountofRiders[i] == 2)
                printf("Rider %d is waiting in line\n", i + 1);
            else if(amountofRiders[i] == 1)
                printf("Rider %d is in a car.\n", i + 1);
        }
        printf("\n");
    }
    return dummy;
}

int main() 
{
    // Create threads
    pthread_t riders[N_RIDERS];
    pthread_t cars[N_CARS];
    pthread_t display_thread;

    // Initialize semaphores
    for (int i = 0; i < N_RIDERS + 1; i++) {
        sem_init(&WaitForRideBegin[i], 0, 0);
        sem_init(&WaitForRideOver[i], 0, 0);
    }

    sem_init(&waitingLineMutux, 0, 1);
    sem_init(&lineAccessMutex, 0, 1);
    sem_init(&exitConditionMutux, 0, 1);
    sem_init(&startRideMutux, 0, 0);

    // Create threads for each rider, see read me file
    for (int i = 1; i <= N_RIDERS; i++)
        pthread_create(&riders[i - 1], NULL, Rider, (void *)(intptr_t)i);

    // Create threads for each car, see read me file
    for (int i = 1; i <= N_CARS; i++)
        pthread_create(&cars[i - 1], NULL, Car, (void *)(intptr_t)i);

    // Create one thread for display
    pthread_create(&display_thread, NULL, Display, NULL);

    // Join threads for riders
    for (int i = 0; i < N_RIDERS; i++) pthread_join(riders[i], NULL);

    // Join threads for cars
    for (int i = 0; i < N_CARS; i++) pthread_join(cars[i], NULL);

    // Join thread for display
    pthread_join(display_thread, NULL);

    return 0;
}

