# Bumper Cars Simulation

# Overview:
This program simulates a bumper cars ride with multiple riders and cars using POSIX threads and semaphores. The simulation includes riders wandering around the park, getting in line, taking a seat in a bumper car, and going through a ride. Cars continuously load, bump, and unload riders until a specified number of rides are completed.



## Compilation:
To compile the program, use the provided Makefile. Open a terminal and navigate to the directory containing the source code and the Makefile. Run the following command:

    make

If you prefer to compile manually, use the following commands:

    gcc -Wall -Wextra -pthread -o bumpercars bumpercars.c

After compilation, run the program using the following command:

    ./bumpercars



## Semaphores:
The simulation uses semaphores for synchronization between threads to ensure proper execution. Here is a brief explanation of key semaphores:

    WaitForRideBegin[]: Ensures that riders wait for their turn to start the ride.

    WaitForRideOver[]: Ensures that riders wait for their turn to signal the end of the ride.

    waitingLineMutex: Controls access to the waiting line to prevent race conditions.

    lineAccessMutex: Ensures exclusive access to shared data during the loading process.

    exitConditionMutex: Controls access to the exit condition to avoid conflicts.

    startRideMutex: Signals the start of the ride to prevent multiple cars from starting simultaneously.
    

## Acknowledgments

This project utilizes the `intptr_t` type to safely convert between pointers and integer types.

In C programming, the `intptr_t` type is an integer type used to hold a pointer. It is defined in the `<stdint.h>` header and is particularly useful when there's a need to convert between pointers and integers. In this project, `intptr_t` is used to cast thread arguments.

To implement and understand the usage of `intptr_t`, I referred to the following:
https://stackoverflow.com/questions/35071200/what-is-the-use-of-intptr-t

Additional information on semaphores:
https://stackoverflow.com/questions/34519/what-is-a-semaphore

