#include "tinyunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <Psapi.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#ifdef THTHREAD_API
#include <pthread.h>
#endif // THREAD_API
#define stricmp strcasecmp
#endif

#define STDOUT(format,...) \
  fprintf(stdout, format, __VA_ARGS__); \
  fflush(stdout);

#define MY_STR_TOK_START(str)  \
  char* p = strdup(str);       \
  char* token;                 \
  int cnt = 0;                 \
                               \
  token = strtok(p, ",");      \
  while (token != NULL) {      \

#define MY_STR_TOK_END         \
    cnt++;                     \
    token = strtok(NULL, ","); \
  }                            \
  free(p);                     \
  
static char selected_categories[16][64] = { 0 }; // we supported at most 16 categories
static char excluded_categories[16][64] = { 0 };

static int win_enable_virtual_terminal_processing() {
#ifdef WIN32
  // Set output mode to handle virtual terminal sequences
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    return GetLastError();
  }
  
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) {
    return GetLastError();
  }
  
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode)) {
    return GetLastError();
  }
#endif
  return 0;
}

static int is_category_selected(const char* category) {
  for (int i = 0; i < 16; i++) {
    if (stricmp(category, (char*)selected_categories[i]) == 0) {
      return 1;
    }
  }
  return 0;
}
static int is_category_excluded(const char* category) {
  for (int i = 0; i < 16; i++) {
    if (stricmp(category, (char*)excluded_categories[i]) == 0) {
      return 1;
    }
  }
  return 0;
}
static int should_run_test(test_entry_t* t) {
  int is_selected = selected_categories[0][0] == 0; // no filter, run ALL tests

  if (!is_selected) {
    MY_STR_TOK_START(t->categories)
      if (is_category_selected(token)) {
        is_selected = 1;
        break;
      }
    MY_STR_TOK_END
    
  }
  
  if (is_selected && excluded_categories[0][0] != 0) {
    MY_STR_TOK_START(t->categories)
      if (is_category_excluded(token)) {
        is_selected = 0;
        break;
      }
    MY_STR_TOK_END
  }
  
  return is_selected;
}

static void run_test(test_entry_t* t) {
  long long start_time = get_current_time_millis();
  t->entry();
  long long time_used = get_current_time_millis() - start_time;
  STDOUT("\x1b[34m##### Time used: %lld ms\x1b[m\n", time_used);
}

int run_tests() {
  win_enable_virtual_terminal_processing();
  
  long long start_time = get_current_time_millis();
  int total_tests_count = 0;
  int index = 1;
  test_entry_t* test;
  
  for (test = TESTS; test->entry; test++) {
    if (!should_run_test(test)) {
      continue;
    }
    total_tests_count++;
  }

  for (test = TESTS; test->entry; test++) {
    if (!should_run_test(test)) {
      continue;
    }
    
    STDOUT("\x1b[34m##### [%d/%d][%s]\x1b[m\n", index, total_tests_count, test->name);
    
    run_test(test);
    index++;
  }

  long long time_used = get_current_time_millis() - start_time;
  STDOUT("\x1b[34m##### Total Time Used: %lld ms\x1b[m\n", time_used);

  return 0;
}

int set_tests_categories(const char* categories) {
  for (int i = 0; i < 16; i++) selected_categories[i][0] = 0;
  
  MY_STR_TOK_START(categories)
    strcpy((char*)selected_categories[cnt], token);
  MY_STR_TOK_END
 
  return 0;
}
int set_tests_excluded_categories(const char* categories) {
  // excluded_categories is used to exclude unit tests from the selected unit tests
  for (int i = 0; i < 16; i++) excluded_categories[i][0] = 0;
  
  MY_STR_TOK_START(categories)
    strcpy((char*)excluded_categories[cnt], token);
  MY_STR_TOK_END
  
  return 0;
}

long long get_current_time_millis() {
#ifdef _WIN32
  FILETIME ft;
  LARGE_INTEGER li;
  
  GetSystemTimePreciseAsFileTime(&ft);
  
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  
  return (long long)(li.QuadPart / 10000LL - 11644473600000LL);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

long get_current_process_memory_usage() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
    return pmc.PrivateUsage;
  } else {
    return -1;
  }
#else
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) {
      return ru.ru_maxrss * 1024; // Convert kilobytes to bytes
  } else {
      return -1;
  }
#endif
}

void wait(int milliseconds) {
  unsigned long long end_time_marker = get_current_time_millis() + milliseconds;
  while (get_current_time_millis() < end_time_marker) {
    mysleep(20);
  }
}

void mysleep(int milliseconds) {
#ifdef _WIN32
  Sleep(milliseconds);
#else
  usleep(milliseconds);
#endif
}

#ifdef THREAD_API

#if _WIN32
static DWORD thread_entry(void* arg) {
  mythread_t* t = (mythread_t*)arg;
  t->cb(t->arg);
  return 0;
}
#else
static void* thread_entry(void* arg) {
  mythread_t* t = (mythread_t*)arg;
  t->cb(t->arg);
  return NULL;
}
#endif

int thread_create(mythread_t* t, mythread_callback cb, void* arg) {
  t->arg = arg;
  t->cb = cb;
  
#ifdef _WIN32
  HANDLE thread;
  
  thread = CreateThread(
      NULL,
      0,
      thread_entry,
      t,
      0,
      NULL
  );
  if (thread == NULL) {
    return (int) GetLastError();
  }
  t->handle = thread;
#else
  int err;
  pthread_t thread;
  err = pthread_create(
      &thread,
      NULL,
      thread_entry,
      t
  );
  if (err != 0) {
    return err;
  }
  t->handle = thread;
#endif
  return 0;
}
int thread_join(mythread_t* t) {
#if _WIN32
  return (int)WaitForSingleObject(t->handle, INFINITE);
#else
  return pthread_join(t->handle, NULL);
#endif
}
int thread_destroy(mythread_t* t) {
#if _WIN32
  return CloseHandle(t->handle);
#else
  // pthread_detach(t->handle);
  return 0;
#endif
}

#endif // THREAD_API

