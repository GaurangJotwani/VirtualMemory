# Practicum Virtual Memory Manager - Gaurav Ramnani & Gaurang Jotwani

This project implements a simple virtual memory manager using paging with Least Recently Used (LRU) page replacement algorithm. The manager provides basic memory allocation, deallocation, reading, and writing functions.


## Instruction to compile our code
1.  Run the make command on terminal
2.  Run the executable produced which will be called heaptest
3.  (optional) Run rm *.bin to remove all bin files generated

## Assumptions
1.  The user will not ask for more memory than a page size (4096 bytes)
2.  All data storage and access will be in units of "bytes" or "char"

## Implementation

The virtual memory manager uses the following data structures:

1. `page` - A structure representing a page in memory or on disk. It contains fields like `in_use`, `on_disc`, `frame_ref`, `is_dirty`, and `time_stamp`.
2. `page_table` - An array of `page` structures, which stores the metadata for each page.
3. `pm_heap` - A contiguous block of memory, which represents the physical memory for our manager.
4. `pm_page_map` - An array of flags, representing whether a page is in use or not in `pm_heap`.

The manager uses a mutex lock to ensure synchronization between different threads accessing the memory.

## Functions

The following functions are provided to interact with the virtual memory manager:

1. `pm_malloc` - Allocates a page in memory and returns a pointer to the `page` structure.
2. `pm_free` - Frees a previously allocated page and releases the associated resources.
3. `read_page_data` - Reads data from a page, given an offset and the number of bytes to read.
4. `write_page_data` - Writes data to a page, given an offset and the number of bytes to write.

## Page Replacement - Least Recently Used (LRU)

For page replacement, we used the Least Recently Used (LRU) algorithm. The main idea behind LRU is to replace the page that has not been used for the longest time. To implement LRU, we introduced a time stamp for each page, represented by the `time_stamp` field in the `page` structure. The `LRU_Counter` is a global variable that is incremented each time a page is accessed. When a page is accessed, its `time_stamp` is updated with the value of `LRU_Counter`. The page with the smallest `time_stamp` value is considered the least recently used page.

The LRU algorithm is implemented in the `find_page_to_swap` function, which iterates through the `page_table` and finds the page with the smallest `time_stamp` value that is currently in memory and not on disk.

When a page needs to be loaded into memory, and there is no available space in `pm_heap`, the LRU algorithm is used to select a page to swap out. The selected page is saved to disk if it is dirty (i.e., its `is_dirty` field is set to `true`). After that, the new page is loaded into memory, replacing the swapped-out page.

## File Operations

Page data is saved to and loaded from disk using the following functions:

1. `save_page_to_disk` - Saves the contents of a page to a binary file on disk.
2. `load_page_to_memory` - Loads the contents of a page from a binary file on disk to memory.
3. `delete_page_from_disk` - Deletes the binary file associated with a page from disk.

The naming convention for the binary files is "page_X.bin", where X is the index of the page in the `page_table`.

The virtual memory manager uses the `fread` and `fwrite` functions to read and write the contents of the binary files, ensuring that the data is stored consistently on disk.

## Internal Fragmentation of a Page

Internal fragmentation refers to the unused memory within a memory allocation unit, such as a page, due to the difference between the size of the allocated block and the actual amount of data stored within it. This type of fragmentation can lead to inefficient use of memory resources, as the unused portions of memory within a page cannot be utilized for other allocations.

In our virtual memory manager, each page has a fixed size defined by `PM_PAGE_SIZE`. When a user requests memory allocation using `pm_malloc`, the manager allocates an entire page, regardless of the requested size. This design choice simplifies the implementation, but can lead to internal fragmentation if the requested size is significantly smaller than the page size.

For example, if `PM_PAGE_SIZE` is set to 4096 bytes, and a user requests a memory allocation of 100 bytes, the remaining 3996 bytes in the page will remain unused, leading to internal fragmentation.

In summary, our virtual memory manager is prone to internal fragmentation due to the fixed page size allocation. Although this simplifies the implementation, it may lead to inefficient use of memory resources in cases where the allocated memory sizes are significantly smaller than the page size.

## Multi-threading Management

In our virtual memory manager implementation, multi-threading support is crucial to ensure that multiple concurrent users can safely and efficiently use the memory without causing race conditions or other synchronization issues. To achieve this, we employ a mutex lock, which is a synchronization primitive used to protect access to shared resources in a multi-threaded environment.

The following steps outline how our implementation in `pm_heap.c` manages multi-threading:

1. **Mutex initialization**: We initialize the mutex lock using the `init_mutex` function, which calls `pthread_mutex_init` to set up the mutex. This function should be called before any memory management operations.

2. **Locking and unlocking the mutex**: In each of the memory management functions, such as `pm_malloc`, `pm_free`, `save_page_to_disk`, `load_page_to_memory`, `read_page_data`, and `write_page_data`, we lock the mutex at the beginning of the function by calling `pthread_mutex_lock(&lock)`. This ensures that only one thread can execute the critical section of the code at a time. Once the operation is complete, we unlock the mutex using `pthread_mutex_unlock(&lock)` to allow other threads to access the shared resources.

3. **Mutex destruction**: Finally, when the virtual memory manager is no longer needed, we call the `delete_mutex` function to destroy the mutex lock using `pthread_mutex_destroy`. This function should be called after all memory management operations have been completed to clean up the resources associated with the mutex.

By incorporating a mutex lock in our implementation, we ensure that multiple concurrent users can safely use the memory without causing race conditions or other synchronization issues. The mutex lock guarantees that only one thread can access shared resources at a time, thereby providing a consistent and synchronized environment for memory management operations.

## Testing

In order to test the functionality and performance of our virtual memory manager implementation, we have created a test file named `heap_test.c`. This file contains a series of test functions that exercise various aspects of the virtual memory manager, including memory allocation and deallocation, reading and writing data, and handling page replacement using the LRU algorithm.

The test file contains the following test functions:

- `test_pm_malloc_and_pm_free()`: Tests basic memory allocation and deallocation using `pm_malloc` and `pm_free` functions.
- `test_read_and_write_when_on_disc()`: Tests reading and writing data when a page is on disk.
- `test_read_and_write_when_given_out_of_bounds_params()`: Tests reading and writing data when given out-of-bounds parameters.
- `test_read_and_write_when_LRU_kicked_out()`: Tests reading and writing data when a page is evicted from memory due to the LRU page replacement policy.
- `test_concurrency_malloc()`: Tests support for multi-threaded execution by running a loop of 5000 iterations, where each iteration spawns two threads. One thread allocates from the heap for an integer value, the other allocates for a string. The results of the allocation can be seen as each thread executes, where it shows the address of the allocated memory as well as the value read from this memory after allocation. The user would also around 10000 .bin files created after running this function. Uses helper functions `thread_safe_allocate_int()` and `thread_safe_allocate_str()`.


The `main()` function in the `heap_test.c` file calls each of these test functions to perform the tests. After compiling the code using the provided Makefile, you can run the `heaptest` executable to execute the tests and validate the correctness of our virtual memory manager implementation.