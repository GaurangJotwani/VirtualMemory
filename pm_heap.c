/*
 * pm_heap.c / Practicium 1
 * Gaurav Ramnani & Gaurang Jotwani / CS5600 / Northeastern University
 * Spring 2023 / Mar 14, 2023
 * This file has the implemenations of functions in pm_heap.h
 */

#include "pm_heap.h"
#include <pthread.h>

/*
 * Mutex to ensure thread-safety.
 */
pthread_mutex_t lock;

char* generate_filename(int page_num) {
  static char file_name[14];
  snprintf(file_name, 14, "page_%d.bin", page_num);
  return file_name;
}

void save_page_to_disk(page* page_ptr) {
  int swap_page = -1;
  swap_page = get_idx_from_page(page_ptr);

  char* file_name = generate_filename(swap_page);

  FILE* fp;
  if (fp = fopen(file_name, "wb+")) {
    if (page_ptr->frame_ref) {
      fwrite(page_ptr->frame_ref, sizeof(uint8_t), PM_PAGE_SIZE, fp);    
    }
    fclose(fp);
  } else {
    printf("Error writing to disk: %s\n", file_name);
    return;
  }
  
  page_ptr->frame_ref = NULL;
  page_ptr->on_disc = true;

  return;
}

void load_page_to_memory(page* page_ptr) {
  int cur_page_num = get_idx_from_page(page_ptr);
  char* file_name = generate_filename(cur_page_num);
  FILE *fp;

  int page_to_write = -1;
  
  if (fp = fopen(file_name, "rb")) {
    for (int i = 0; i < PM_PAGE_COUNT; ++i) {
      if (pm_page_map[i] == 0) {
        page_to_write = i;
        break;
      }
    }

    uint8_t* page_start;
    if (page_to_write == -1) {
      page* lru_page = find_page_to_swap();
      page_to_write = get_idx_from_page(lru_page);
      page_start = lru_page -> frame_ref; 
      if (lru_page -> is_dirty) {
        save_page_to_disk(lru_page);
        lru_page -> is_dirty = false;
      }
      lru_page -> on_disc = true;
      lru_page -> frame_ref = NULL;
    } else {
      page_start = pm_heap + (page_to_write * PM_PAGE_SIZE);
    }
    fread(page_start, sizeof(uint8_t), PM_PAGE_SIZE, fp);
    page_ptr -> on_disc = false;
    page_ptr -> frame_ref = page_start;
    pm_page_map[page_to_write] = 1;
    fclose(fp);
  } else {
    // File does not exist
    printf("Error reading from disk: %s\n", file_name);
    return;
  }
}

void delete_page_from_disk(int page_num) {
  char* file_name = generate_filename(page_num);

  if (remove(file_name) == 0) {
    return;
  } else {
    printf("Unable to delete file for page %d\n", page_num);
    return;
  }
}

void* pm_malloc(size_t size) {
  pthread_mutex_lock(&lock);
  if (size > PM_PAGE_SIZE || size == 0) {
    printf("Invalid Memory size requested\n");
    pthread_mutex_unlock(&lock);
    return NULL;
  }
  for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
    if (!page_table[i].in_use) {
      page_table[i].in_use = true;
      page_table[i].on_disc = true;
      page_table[i].time_stamp = LRU_Counter++;
      save_page_to_disk(&page_table[i]);
      pthread_mutex_unlock(&lock);
      return (void*) &page_table[i];
    }
  }
  pthread_mutex_unlock(&lock);
  printf("Not enough memory in heap\n");
  return NULL;
}


void pm_free(void* ptr) {
  if (!ptr) return;
  page* temp = (page*) ptr;
  if (temp < page_table || temp > &page_table[PAGE_TABLE_ENTRY_COUNT - 1]) return;
  pthread_mutex_lock(&lock);
  if (!temp -> on_disc) {
    int mem_page_num = get_heap_idx_from_page(temp);
    pm_page_map[mem_page_num] = 0;
  }
  temp -> on_disc = false;
  int page_num = get_idx_from_page(temp);
  delete_page_from_disk(page_num);
  temp -> in_use = false;
  temp -> frame_ref = NULL;
  pthread_mutex_unlock(&lock);
}


int init_mutex() {
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("Mutex init failed\n");
    return 1;
  }
  return 0;
}


void delete_mutex() {
  pthread_mutex_destroy(&lock);
}


page* find_page_to_swap(void) {
  page* res;
  int lru_max = 2147483647;
  for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
    if (page_table[i].in_use == true && page_table[i].on_disc == false) {
      if (page_table[i].time_stamp < lru_max) {
        res = &page_table[i];
        lru_max = page_table[i].time_stamp;
      }
    }
  }
  return lru_max == 2147483647 ? NULL : res;
}


int get_idx_from_page(page* p) {
  int res = ((size_t)p - (size_t)page_table) / sizeof(page);
  return res;
}

int get_heap_idx_from_page(page* p) {
  int res = ((size_t)p -> frame_ref - (size_t)pm_heap) / PM_PAGE_SIZE;
  return res;
}

void read_page_data(page* p, size_t offset, size_t bytes_to_read, uint8_t* buffer) {
  if (!p || p < page_table || p > &page_table[PAGE_TABLE_ENTRY_COUNT - 1]) {
    printf("Invalid Pointer\n");
    return;
  };
  pthread_mutex_lock(&lock);
  if (p -> in_use && p -> on_disc) {
    load_page_to_memory(p);
  };
  p -> time_stamp = LRU_Counter++;
  uint8_t* temp = (p -> frame_ref) + offset;
  // Check if its in bound
  if ((temp + bytes_to_read) > ((p -> frame_ref) + PM_PAGE_SIZE)) {
    printf("Cannot read out of bounds of the page\n");
    return;
  }
  uint8_t* temp2 = buffer;
  for (int i = 0; i < bytes_to_read; i++) {
    *temp2 = *(temp + i);
    temp2++;
  }
  pthread_mutex_unlock(&lock);
}

void write_page_data(page* p, size_t offset, size_t bytes_to_write, uint8_t* buffer) {
  if (!p || p < page_table || p > &page_table [PAGE_TABLE_ENTRY_COUNT - 1]) {
    printf("Invalid Pointer\n");
    return;
  };
  pthread_mutex_lock(&lock);
  if (p -> in_use && p -> on_disc) load_page_to_memory(p);
  p -> is_dirty = true;
  p -> time_stamp = LRU_Counter++;
  uint8_t* temp = (p -> frame_ref) + offset;
  // Check if its in bound
  if ((temp + bytes_to_write) > ((p -> frame_ref) + PM_PAGE_SIZE)) {
    printf("Cannot write out of bounds of the page\n");
    return;
  }
  uint8_t* temp2 = buffer;
  for (int i = 0; i < bytes_to_write; i++) {
    *temp = *temp2;
    temp++;
    temp2++;
  }
  pthread_mutex_unlock(&lock);
}