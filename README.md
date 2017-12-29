# ASYNCHRONOUS-I-O-FILE-COPY
CME 3205 Operating Systems  III - FALL 2017

### Goal
 In this assignment you are asked to copy a txt file to a defined destination path using Asynchronous I/O.

### General Requirements

•	For this assignment you will work in groups of ONE.

### Implementation Requirements

•	You have to use C prog. lang. for this assignment.
•	The POSIX AIO interface will be used for asynchronous I/O process.
•	Define a function to prepare a source file randomly. The content of the source file must be readable.
•	For example; let’s say that the user enters 5 threads and the size of the source file as 10 bytes. So each thread must copy 2 bytes for each. In order to be readable, your source file must be created as “aabbccddee”.

•	Create threads (user will be able to define the number of threads from 1 to 10) for asynchronous I/O copy process(read + write).
•	Each thread will copy equal sized partition of the file. The percentage of the coping process must be displayed on the screen dynamically.
•	On the screen user must be able to enter source and destination paths, the number of the threads, type size(Byte(0),MB(1)), and the size of the source file to be created(1 Byte to 200 MB) from command-line interface. Put a space between each parameter.
•	Source and destination paths can be given as real paths OR  as the character (–) which means that related txt file will be created under the same path as executable file. 
•	An Example to command-line interface 
./run - -10 0 20  where 
first (-) source path.(same as executable file) 
second (-) destination path.(same as executable file) 
10  number of threads
0  Bytes
20 the source file will be created in the size of  20 bytes


