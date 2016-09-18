# multiple_writers_readers_buffer_baton_passing
Solution to the 1st project of the "Concurrent and Parallel Programming"


Problem Specification
---------------------

Implement a data structure for a limited buffer with N positions, which is used to broadcast
between P producers (writers) and C consumers (readers). The buffer should offer operations for depositing (writing) and consuming (reading).

By calling deposit, the producer should block until it can insert the new item. Similarly, by calling consume, the consumer should remain blocked until it can get an item to consume. A position in the buffer can only be reused when all C consumers have read the message. Every consumer should receive the messages in the order they were deposited.

Use C (or C++) with pthread and semaphore libraries using the baton passing technique.


How to use and Configure
------------------------

Since this code uses booleans (stdbool.h), it should be compiled with C99 or superior.

At the beginning of main.C, one can find the (global) configuration parameters such as number of total messages per thread, number of producer and consumer threads and buffer size. The code prints at stdout what it is doing at each moment. If one wants to be able to follow in real time, uncomment the code in the wait_for function or just limit the number of total messages.
