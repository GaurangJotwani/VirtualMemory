/*
 * pm_heap.h / Practicium 1
 * Gaurav Ramnani & Gaurang Jotwani / CS5600 / Northeastern University
 * Spring 2023 / Mar 14, 2023
 * This file has the function signatures, constants and library imports for our project
 */


#ifndef PM_HEAP_H
#define PM_HEAP_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>



/*
 * Size of the entire heap.
 * Size of each page in the heap.
 * Number of pages in the heap.
 * Number of pages in the page_table
 */
#define PM_HEAP_SIZE 10*1024*1024
#define PM_PAGE_SIZE 4096
#define PM_PAGE_COUNT (PM_HEAP_SIZE/PM_PAGE_SIZE)
#define PAGE_TABLE_ENTRY_COUNT 10000


/*
 * Structure to represent a virtual page.
 * in_use tells if the page is currently allocated
 * on_disc tells if the page is currently on_disc. If this value is false, then page is loaded in memory.
 * is_dirty tells if the page has been modified.
 * time_stamp tells us the when the page was last used (will be updated every time it is accessed)
 * frame stores a pointer to memory in heap (only has a value if on_disc is false)
 */
typedef struct page {
  bool in_use;
  bool on_disc;
  bool is_dirty;
  int time_stamp;
  uint8_t* frame_ref;
} page;


/*
 * Data structure representing the physical heap.
 */
static uint8_t pm_heap[PM_HEAP_SIZE];

/*
 * Global pm_page_map that contains PM_PAGE_COUNT entries
 * This keeps track of the free pages in the pm_heap
 */
static uint8_t pm_page_map[PM_PAGE_COUNT];

/*
 * Global page table that contains PAGE_TABLE_ENTRY_COUNT pages
 */
static page page_table[PAGE_TABLE_ENTRY_COUNT];

/*
 * Global LRU counter to keep track of least frequently used page
 */
static int LRU_Counter = 0;

/*
 * Mutex to ensure thread-safety.
 */
// pthread_mutex_t lock;

/*
 * Function To Allocate Memory in the heap
 * When allocating new memory, we will iteratively go through each page in page_table until we find a page
 * which is not in_use. 
 * If we find such page, we allocate it, else we return NULL;
 * If page is found, we will change in_use to true, update the time stamp with currently value of LRU counter and increase the LRU_counter by 1.
 * Inputs:
 *   size_t size: Number of Bytes to Allocate
 * Returns:
 *   NULL if not enough space is left in the heap
 *   Else
 *   pointer pointing to page table entry of the allocated page
 */
void* pm_malloc(size_t size);

/*
* Function To Free Memory in the space.
* Three Scenarios for this:
* 1. If page is in_use and on_disc:
*       delete the page_file, change in_use and on_disc flags to false
* 2. If page is in_use and not on_disc:
*       delete the page_file (it will always exist), change in_use and on_disc flags to false
*       Using the frame_ref pointer, update pm_page_map to false for this page (indicating memory is free)
*       Set frame_ref to NULL
* 3. Invalid page given (For eg a page that is not is use, or a page that is out of bounds)
*    Return False when invalid page is given.
* Inputs:
*   page* ptr: Pointer to the page that needs to be freed.
* Returns:
*   bool status: False if memory could not be freed else True
*/
void pm_free(void* ptr);

/*
* Function to save a page to disc in a binary file
* First find the page number of the page form the given pointer
* Make a file name form the page_num such as "page_num.bin"
* Check if the file already exist. If not, create the file.
* If frame_ref of the page is not NULL (is loaded on heap), save the data of the page in binary file
* set on_disc flag to true and frame reference to NULL.
* Input:
*   page* ptr: Pointer to the page that needs to be saved to disc.
*/
void save_page_to_disk(page* page_ptr);

/*
* Function to load the page data from disc in a binary file to the virtual_heap
* First find the page number of the page form the given pointer
* Make a file name form the page_num such as "page_num.bin"
* Check if the file already exist. If not, return Error.
* Find a free page from pm_page_map to put the data in.
* Two things can happen at this point:
*  1. If space is found in pm_heap, put the data in that space.
*  2. else save the Least recently used page to disc and put the data of this page to the LRU page.
* Set on_disc to false, frame_ref to the memory block in pm_heap
* Input:
*   page* ptr: Pointer to the page that needs to be saved to disc.
*/
void load_page_to_memory(page* page_ptr);

/*
 * Returns pointer to the page that is in memory and least recently used.
 */
page* find_page_to_swap(void);

/*
 * Helper func that Gets the index of the page in page_table from the pointer to page
 * Input:
 *   page* ptr: Pointer to the page that needs to be saved to disc.
 * Returns:
 *   int index of the page in page_table
 */
int get_idx_from_page(page* p);

/*
 * Helper func Gets the index of the heap memory that page in pointing to from the pointer to page
 * Input:
 *   page* ptr: Pointer to the page that needs to be saved to disc.
 * Returns:
 *   int index of the page in page_table
 */
int get_heap_idx_from_page(page* p);

/*
 * Helper func Deletes the file on disc from page number
 * Input:
 *   int page_num: Page number of the page to delete
 */
void delete_page_from_disk(int page_num);

/*
 * Reads the data inside page and saves in the buffer provided
 * This function will load the page into memory, if its not already there.
 * Input:
 *   page* ptr: Pointer to the page that needs to be read.
 *   size_t bytes_to_read: how many bytes to read 
 *   size_t offset: where to start reading data from
 *   uint8_t* buffer: Buffer to store data in so it can be read
 */
void read_page_data(page* p, size_t bytes_to_read, size_t offset, uint8_t* buffer);

/*
 * Writes the data inside page form the buffer provided
 * This function will load the page into memory, if its not already there.
 * Input:
 *   page* ptr: Pointer to the page that needs to be written to.
 *   size_t bytes_to_write: how many bytes to write 
 *   size_t offset: where to start writing data from
 *   uint8_t* buffer: Buffer to write data from
 */
void write_page_data(page* p, size_t offset, size_t bytes_to_write, uint8_t* buffer);

/*
 * Destroys the mutex for the heap.
 */
void delete_mutex();

/*
 * Initializes the mutex for the heap.
 */
int init_mutex();

#endif
