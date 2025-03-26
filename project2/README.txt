Project 2 - Networked Database Server

This project implements a multithreaded networked database server along with testing utilities. Below are the files that have been created/modified for this project:

-----------------------------------------------------
Created Files:
-----------------------------------------------------

1. dbserver.c
   - Implements the main server logic.
   - Listens on a TCP port for incoming connections.
   - Spawns worker threads that handle read, write, and delete requests.
   - Integrates with the database and queue modules for synchronized, concurrent processing.

2. database.c
   - Contains the implementation of database functions.
   - Provides routines to write, read, and delete records from the database.
   - Uses file I/O to store each record in a separate file under /tmp.

3. database.h
   - Declares the data structures and functions for the database module.
   - Defines constants such as the maximum number of keys and record formats.

4. queue.c
   - Implements a thread-safe work queue.
   - Provides functions to enqueue and dequeue incoming connection requests.
   - Handles proper cleanup and shutdown of the queue.

5. queue.h
   - Contains the declarations for the queue module.
   - Defines the work item structure and function prototypes for queue operations.

6. testing.sh
   - A shell script designed to test the server.
   - Runs a series of tests including set, get, delete, load, and random tests.
   - Helps verify that the server operates correctly under various conditions.

-----------------------------------------------------
Modified Files:
-----------------------------------------------------

1. Makefile
   - The Makefile has been updated to include additional targets for the new test files (`dbtest_fixed` and `dbtest_seeded`) along with the original targets (`dbserver` and `dbtest`).
   - Duplicate target definitions (e.g., multiple definitions for `dbserver`) have been removed to simplify the build process.
   - Compiler flags (CFLAGS) and linker flags (LDLIBS) have been standardized to include debugging information (`-ggdb3`), strict warnings (`-Wall`), and proper linking with pthread and zlib (`-lpthread` and `-lz`).
   - The list of executables (EXES) has been updated so that running `make` builds all necessary components. The `clean` target has been enhanced to remove the generated executables and intermediate files.