#include "pm_heap.h"


void test_pm_malloc_and_pm_free() {
  page* mem1 = pm_malloc(10);
  page* mem2 = pm_malloc(10);
  page* mem3 = pm_malloc(10);
  pm_free(mem2);
  page* mem4 = pm_malloc(10);
  page* mem[PAGE_TABLE_ENTRY_COUNT];
  for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT - 3; i++) {
    mem[i] = pm_malloc(50);
  }
  for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT - 3; i++) {
    pm_free(mem[i]);
  }
  pm_free(mem1);
  pm_free(mem3);
  pm_free(mem4);
  printf("=====================Test pm_free and pm_malloc Passed!=====================\n");
}

void test_read_and_write_when_on_disc() {
  page* mem1 = pm_malloc(10);
  uint8_t buffer[8];
  int* temp = (int*) buffer;
  *temp = 500;
  write_page_data(mem1, 0, 8, buffer);
  uint8_t buffer2[8];
  read_page_data(mem1, 0, 8, buffer2);
  pm_free(mem1);
  printf("=====================Test read and write when on disc Passed!=====================\n");
}

void test_read_and_write_when_given_out_of_bounds_params() {
  page* mem1 = pm_malloc(PM_PAGE_SIZE);
  uint8_t buffer[PM_PAGE_SIZE];
  write_page_data(mem1, 1, PM_PAGE_SIZE, buffer);
  uint8_t buffer2[PM_PAGE_SIZE];
  read_page_data(mem1, 1, PM_PAGE_SIZE, buffer2);
  pm_free(mem1);
  printf("=====================Test read and write when given out of bound param Passed!=====================\n");
}

void test_read_and_write_when_LRU_kicked_out() {
  page* mem[PM_PAGE_COUNT];
  uint8_t buffer[8];
  int* temp = (int*) buffer;
  *temp = 500;
  for (int i = 0; i < PM_PAGE_COUNT; i++) {
    mem[i] = pm_malloc(50);
    write_page_data(mem[i], 0, 8, buffer);
  }
  uint8_t buffer2[PM_PAGE_SIZE];
  read_page_data(mem[1], 1, 4, buffer2);
  // At this point all pages are in use;
  page* mem1 = pm_malloc(10);
  write_page_data(mem1, 0, 8, buffer);
  page* mem2 = pm_malloc(10);
  write_page_data(mem2, 0, 8, buffer);
  page* mem3 = pm_malloc(10);
  write_page_data(mem3, 0, 8, buffer);
  write_page_data(mem1, 0, 8, buffer);
  write_page_data(mem[10], 0, 8, buffer);
  for (int i = 0; i < PM_PAGE_COUNT; i++) {
      pm_free(mem[i]);
  }
  pm_free(mem1);
  pm_free(mem2);
  pm_free(mem3);
  printf("=====================Test LRU behaviour Passed!=====================\n");
}

static int thread_id = 1;

void thread_safe_allocate_int(int val) {
  printf("Starting thread %d\n", thread_id);
  int int_size = sizeof(int);
  page* int_page_ptr =  pm_malloc(int_size);
  uint8_t buffer[int_size];
  int* temp = (int*) buffer;
  *temp = val;
  write_page_data(int_page_ptr, 0, int_size, buffer);

  uint8_t buffer2[int_size];
  read_page_data(int_page_ptr, 0, int_size, buffer2);
  printf("int_ptr address: %p\n", int_page_ptr);
  printf("int_ptr value: %d\n", *((int*) buffer2));
  if (int_page_ptr == NULL) {
   printf("Error while allocating int_ptr\n");
  }
  // pm_free(int_page_ptr);
  thread_id++;
  usleep(500);
}

void thread_safe_allocate_str(char* val) {
  printf("Starting thread %d\n", thread_id);
  int str_size = strlen(val);
  page* str_page_ptr = pm_malloc(str_size);
  uint8_t buffer[str_size];

  char* temp = (char*) buffer;
  for (int i = 0; i < str_size; ++i) {
    *temp++ = val[i];
  }

  write_page_data(str_page_ptr, 0, str_size, buffer);

  uint8_t buffer2[str_size];
  read_page_data(str_page_ptr, 0, str_size, buffer2);

  printf("str_ptr address: %p\n", str_page_ptr);
  printf("str_ptr value: %s\n", ((char*) buffer2));
  if (str_page_ptr == NULL) {
   printf("Error while allocating str_ptr\n");
  }
  thread_id++;
  // pm_free(str_page_ptr);
  usleep(500);
}

void test_concurrency_malloc() {
  init_mutex();
  for(int i = 1; i <= 5000; i++) {
    pthread_t thread;
    pthread_create(&thread, NULL, thread_safe_allocate_int, 31*i);
    pthread_join(thread, NULL);
    pthread_t str_thread;
    pthread_create(&str_thread, NULL, thread_safe_allocate_str, "ababab");
    pthread_join(str_thread, NULL);
  }
  delete_mutex();
  printf("=====================Testing Concurrency Passed!=====================\n");
}

void test_if_data_is_saved_after_loading_to_disc() {
  page* mem1 = pm_malloc(PM_PAGE_SIZE);
  uint8_t buffer[8];
  int* temp = (int*) buffer;
  *temp = 500;
  temp++;
  *temp = 1000;
  write_page_data(mem1, 0, 8, buffer);
  uint8_t buffer2[PM_PAGE_SIZE];
  read_page_data(mem1, 0, 8, buffer2);
  save_page_to_disk(mem1);
  load_page_to_memory(mem1);
  read_page_data(mem1, 0, 8, buffer2);
  pm_free(mem1);
  printf("=====================Test data is persistance in disc Passed!=====================\n");
}

int main() {
  test_pm_malloc_and_pm_free();
  test_read_and_write_when_on_disc();
  test_read_and_write_when_LRU_kicked_out();
  test_if_data_is_saved_after_loading_to_disc();
  test_read_and_write_when_given_out_of_bounds_params();
  sleep(5);
  test_concurrency_malloc();
  return 0;
}