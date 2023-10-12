/*
 * Copyright 2008 Google Inc.
 * Copyright 2014-2020 Andreas Schneider <asn@cryptomilk.org>
 * Copyright 2015      Jakub Hrozek <jakub.hrozek@posteo.se>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <stdint.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <float.h>

/*
 * This allows to add a platform specific header file. Some embedded platforms
 * sometimes miss certain types and definitions.
 *
 * Example:
 *
 * typedef unsigned long int uintptr_t
 * #define _UINTPTR_T 1
 * #define _UINTPTR_T_DEFINED 1
 */
#ifdef CMOCKA_PLATFORM_INCLUDE
# include "cmocka_platform.h"
#endif /* CMOCKA_PLATFORM_INCLUDE */

#include <cmocka.h>
#include <cmocka_private.h>

/* Size of guard bytes around dynamically allocated blocks. */
#define MALLOC_GUARD_SIZE 16
/* Pattern used to initialize guard blocks. */
#define MALLOC_GUARD_PATTERN 0xEF
/* Pattern used to initialize memory allocated with test_malloc(). */
#define MALLOC_ALLOC_PATTERN 0xBA
#define MALLOC_FREE_PATTERN 0xCD
/* Alignment of allocated blocks.  NOTE: This must be base2. */
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT sizeof(size_t)
#endif

/* Printf formatting for source code locations. */
#define SOURCE_LOCATION_FORMAT "%s:%u"

#if defined(HAVE_GCC_THREAD_LOCAL_STORAGE)
# define CMOCKA_THREAD __thread
#elif defined(HAVE_MSVC_THREAD_LOCAL_STORAGE)
# define CMOCKA_THREAD __declspec(thread)
#else
# define CMOCKA_THREAD
#endif

#ifdef HAVE_CLOCK_REALTIME
#define CMOCKA_CLOCK_GETTIME(clock_id, ts) clock_gettime((clock_id), (ts))
#else
#define CMOCKA_CLOCK_GETTIME(clock_id, ts)
#endif

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

/**
 * POSIX has sigsetjmp/siglongjmp, while Windows only has setjmp/longjmp.
 */
#ifdef HAVE_SIGLONGJMP
# define cm_jmp_buf             sigjmp_buf
# define cm_setjmp(env)         sigsetjmp(env, 1)
# define cm_longjmp(env, val)   siglongjmp(env, val)
#else
# define cm_jmp_buf             jmp_buf
# define cm_setjmp(env)         setjmp(env)
# define cm_longjmp(env, val)   longjmp(env, val)
#endif

/*
 * Declare and initialize a uintmax_t variable name
 * with value the conversion of ptr.
 */
#define declare_initialize_value_pointer_pointer(name, ptr) \
    uintmax_t name ; \
    name = (uintmax_t)((uintptr_t)(ptr))

/* Cast a uintmax_t to pointer_type. */
#define cast_uintmax_type_to_pointer( \
    pointer_type, largest_integral_type) \
    ((pointer_type)(uintptr_t)(largest_integral_type))

/* Doubly linked list node. */
typedef struct ListNode {
    const void *value;
    int refcount;
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

/* Debug information for malloc(). */
struct MallocBlockInfoData {
    void* block;              /* Address of the block returned by malloc(). */
    size_t allocated_size;    /* Total size of the allocated block. */
    size_t size;              /* Request block size. */
    SourceLocation location;  /* Where the block was allocated. */
    ListNode node;            /* Node within list of all allocated blocks. */
};

typedef union {
    struct MallocBlockInfoData *data;
    char *ptr;
} MallocBlockInfo;

/* State of each test. */
typedef struct TestState {
    const ListNode *check_point; /* Check point of the test if there's a */
                                 /* setup function. */
    void *state;                 /* State associated with the test. */
} TestState;

/* Determines whether two values are the same. */
typedef int (*EqualityFunction)(const void *left, const void *right);

/* Value of a symbol and the place it was declared. */
typedef struct SymbolValue {
    SourceLocation location;
    uintmax_t value;
} SymbolValue;

/*
 * Contains a list of values for a symbol.
 * NOTE: Each structure referenced by symbol_values_list_head must have a
 * SourceLocation as its' first member.
 */
typedef struct SymbolMapValue {
    const char *symbol_name;
    ListNode symbol_values_list_head;
} SymbolMapValue;

/* Where a particular ordering was located and its symbol name */
typedef struct FuncOrderingValue {
    SourceLocation location;
    const char * function;
} FuncOrderingValue;

/* Used by list_free() to deallocate values referenced by list nodes. */
typedef void (*CleanupListValue)(const void *value, void *cleanup_value_data);

/* Structure used to check the range of integer types.a */
typedef struct CheckIntegerRange {
    CheckParameterEvent event;
    uintmax_t minimum;
    uintmax_t maximum;
} CheckIntegerRange;

/* Structure used to check whether an integer value is in a set. */
typedef struct CheckIntegerSet {
    CheckParameterEvent event;
    const uintmax_t *set;
    size_t size_of_set;
} CheckIntegerSet;

/* Used to check whether a parameter matches the area of memory referenced by
 * this structure.  */
typedef struct CheckMemoryData {
    CheckParameterEvent event;
    const void *memory;
    size_t size;
} CheckMemoryData;

static ListNode* list_initialize(ListNode * const node);
static ListNode* list_add(ListNode * const head, ListNode *new_node);
static ListNode* list_add_value(ListNode * const head, const void *value,
                                     const int count);
static ListNode* list_remove(
    ListNode * const node, const CleanupListValue cleanup_value,
    void * const cleanup_value_data);
static void list_remove_free(
    ListNode * const node, const CleanupListValue cleanup_value,
    void * const cleanup_value_data);
static int list_empty(const ListNode * const head);
static int list_find(
    ListNode * const head, const void *value,
    const EqualityFunction equal_func, ListNode **output);
static int list_first(ListNode * const head, ListNode **output);
static ListNode* list_free(
    ListNode * const head, const CleanupListValue cleanup_value,
    void * const cleanup_value_data);

static void add_symbol_value(
    ListNode * const symbol_map_head, const char * const symbol_names[],
    const size_t number_of_symbol_names, const void* value, const int count);
static int get_symbol_value(
    ListNode * const symbol_map_head, const char * const symbol_names[],
    const size_t number_of_symbol_names, void **output);
static void free_value(const void *value, void *cleanup_value_data);
static void free_symbol_map_value(
    const void *value, void *cleanup_value_data);
static void remove_always_return_values(ListNode * const map_head,
                                        const size_t number_of_symbol_names);

static size_t check_for_leftover_values_list(const ListNode * head,
                                             const char * const error_message);

static size_t check_for_leftover_values(
    const ListNode * const map_head, const char * const error_message,
    const size_t number_of_symbol_names);

static void remove_always_return_values_from_list(ListNode * const map_head);

/*
 * This must be called at the beginning of a test to initialize some data
 * structures.
 */
static void initialize_testing(const char *test_name);

/* This must be called at the end of a test to free() allocated structures. */
static void teardown_testing(const char *test_name);

static uint32_t cm_get_output(void);

static int cm_error_message_enabled = 1;
static CMOCKA_THREAD char *cm_error_message;

/*
 * Keeps track of the calling context returned by setenv() so that the fail()
 * method can jump out of a test.
 */
static CMOCKA_THREAD cm_jmp_buf global_run_test_env;
static CMOCKA_THREAD int global_running_test = 0;

/* Keeps track of the calling context returned by setenv() so that */
/* mock_assert() can optionally jump back to expect_assert_failure(). */
jmp_buf global_expect_assert_env;
int global_expecting_assert = 0;
const char *global_last_failed_assert = NULL;
static int global_skip_test;
static int global_stop_test;
static int global_list_test;

/* Keeps a map of the values that functions will have to return to provide */
/* mocked interfaces. */
static CMOCKA_THREAD ListNode global_function_result_map_head;
/* Location of the last mock value returned was declared. */
static CMOCKA_THREAD SourceLocation global_last_mock_value_location;

/* Keeps a map of the values that functions expect as parameters to their
 * mocked interfaces. */
static CMOCKA_THREAD ListNode global_function_parameter_map_head;
/* Location of last parameter value checked was declared. */
static CMOCKA_THREAD SourceLocation global_last_parameter_location;

/* List (acting as FIFO) of call ordering. */
static CMOCKA_THREAD ListNode global_call_ordering_head;
/* Location of last call ordering that was declared. */
static CMOCKA_THREAD SourceLocation global_last_call_ordering_location;

/* List of all currently allocated blocks. */
static CMOCKA_THREAD ListNode global_allocated_blocks;

static uint32_t global_msg_output = CM_OUTPUT_STANDARD;

static const char *global_test_filter_pattern;

static const char *global_skip_filter_pattern;

#ifndef _WIN32
/* Signals caught by exception_handler(). */
static const int exception_signals[] = {
    SIGFPE,
    SIGILL,
    SIGSEGV,
#ifdef SIGBUS
    SIGBUS,
#endif
#ifdef SIGSYS
    SIGSYS,
#endif
};

/* Default signal functions that should be restored after a test is complete. */
typedef void (*SignalFunction)(int signal);
static SignalFunction default_signal_functions[
    ARRAY_SIZE(exception_signals)];

#else /* _WIN32 */

/* The default exception filter. */
static LPTOP_LEVEL_EXCEPTION_FILTER previous_exception_filter;

/* Fatal exceptions. */
typedef struct ExceptionCodeInfo {
    DWORD code;
    const char* description;
} ExceptionCodeInfo;

#define EXCEPTION_CODE_INFO(exception_code) {exception_code, #exception_code}

static const ExceptionCodeInfo exception_codes[] = {
    EXCEPTION_CODE_INFO(EXCEPTION_ACCESS_VIOLATION),
    EXCEPTION_CODE_INFO(EXCEPTION_ARRAY_BOUNDS_EXCEEDED),
    EXCEPTION_CODE_INFO(EXCEPTION_DATATYPE_MISALIGNMENT),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_DENORMAL_OPERAND),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_DIVIDE_BY_ZERO),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_INEXACT_RESULT),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_INVALID_OPERATION),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_OVERFLOW),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_STACK_CHECK),
    EXCEPTION_CODE_INFO(EXCEPTION_FLT_UNDERFLOW),
    EXCEPTION_CODE_INFO(EXCEPTION_GUARD_PAGE),
    EXCEPTION_CODE_INFO(EXCEPTION_ILLEGAL_INSTRUCTION),
    EXCEPTION_CODE_INFO(EXCEPTION_INT_DIVIDE_BY_ZERO),
    EXCEPTION_CODE_INFO(EXCEPTION_INT_OVERFLOW),
    EXCEPTION_CODE_INFO(EXCEPTION_INVALID_DISPOSITION),
    EXCEPTION_CODE_INFO(EXCEPTION_INVALID_HANDLE),
    EXCEPTION_CODE_INFO(EXCEPTION_IN_PAGE_ERROR),
    EXCEPTION_CODE_INFO(EXCEPTION_NONCONTINUABLE_EXCEPTION),
    EXCEPTION_CODE_INFO(EXCEPTION_PRIV_INSTRUCTION),
    EXCEPTION_CODE_INFO(EXCEPTION_STACK_OVERFLOW),
};
#endif /* !_WIN32 */

enum CMUnitTestStatus {
    CM_TEST_NOT_STARTED,
    CM_TEST_PASSED,
    CM_TEST_FAILED,
    CM_TEST_ERROR,
    CM_TEST_SKIPPED,
};

struct CMUnitTestState {
    const ListNode *check_point; /* Check point of the test if there's a setup function. */
    const struct CMUnitTest *test; /* Point to array element in the tests we get passed */
    void *state; /* State associated with the test */
    const char *error_message; /* The error messages by the test */
    enum CMUnitTestStatus status; /* PASSED, FAILED, ABORT ... */
    double runtime; /* Time calculations */
};

/* Exit the currently executing test. */
static void exit_test(const bool quit_application)
{
    const char *env = getenv("CMOCKA_TEST_ABORT");
    int abort_test = 0;

    if (env != NULL && strlen(env) == 1) {
        abort_test = (env[0] == '1');
    }

    if (global_skip_test == 0 && abort_test == 1) {
        if (cm_error_message != NULL) {
            print_error("%s", cm_error_message);
        }
        abort();
    } else if (global_running_test) {
        cm_longjmp(global_run_test_env, 1);
    } else if (quit_application) {
        exit(EXIT_FAILURE);
    }
}

void _skip(const char * const file, const int line)
{
    cmocka_print_error(SOURCE_LOCATION_FORMAT ": Skipped!\n", file, line);
    global_skip_test = 1;
    exit_test(true);

    /* Unreachable */
    exit(EXIT_FAILURE);
}

void _stop(void)
{
    global_stop_test = 1;
    exit_test(true);

    /* Unreachable */
    exit(EXIT_FAILURE);
}

/* Initialize a SourceLocation structure. */
static void initialize_source_location(SourceLocation * const location) {
    assert_non_null(location);
    location->file = NULL;
    location->line = 0;
}


/* Determine whether a source location is currently set. */
static int source_location_is_set(const SourceLocation * const location) {
    assert_non_null(location);
    return location->file && location->line;
}


/* Set a source location. */
static void set_source_location(
    SourceLocation * const location, const char * const file,
    const int line) {
    assert_non_null(location);
    location->file = file;
    location->line = line;
}


static int c_strreplace(char *src,
                        size_t src_len,
                        const char *pattern,
                        const char *repl,
                        int *str_replaced)
{
    char *p = NULL;

    p = strstr(src, pattern);
    if (p == NULL) {
        return -1;
    }

    do {
        size_t of = p - src;
        size_t l  = strlen(src);
        size_t pl = strlen(pattern);
        size_t rl = strlen(repl);

        /* overflow check */
        if (src_len <= l + MAX(pl, rl) + 1) {
            return -1;
        }

        if (rl != pl) {
            memmove(src + of + rl, src + of + pl, l - of - pl + 1);
        }

        memcpy(src + of, repl, rl);

        if (str_replaced != NULL) {
            *str_replaced = 1;
        }
        p = strstr(src, pattern);
    } while (p != NULL);

    return 0;
}

static int c_strmatch(const char *str, const char *pattern)
{
    int ok;

    if (str == NULL || pattern == NULL) {
        return 0;
    }

    for (;;) {
        /* Check if pattern is done */
        if (*pattern == '\0') {
            /* If string is at the end, we're good */
            if (*str == '\0') {
                return 1;
            }

            return 0;
        }

        if (*pattern == '*') {
            /* Move on */
            pattern++;

            /* If we are at the end, everything is fine */
            if (*pattern == '\0') {
                return 1;
            }

            /* Try to match each position */
            for (; *str != '\0'; str++) {
                ok = c_strmatch(str, pattern);
                if (ok) {
                    return 1;
                }
            }

            /* No match */
            return 0;
        }

        /* If we are at the end, leave */
        if (*str == '\0') {
            return 0;
        }

        /* Check if we have a single wildcard or matching char */
        if (*pattern != '?' && *str != *pattern) {
            return 0;
        }

        /* Move string and pattern */
        str++;
        pattern++;
    }

    return 0;
}

/* Create function results and expected parameter lists. */
void initialize_testing(const char *test_name) {
    (void)test_name;
    list_initialize(&global_function_result_map_head);
    initialize_source_location(&global_last_mock_value_location);
    list_initialize(&global_function_parameter_map_head);
    initialize_source_location(&global_last_parameter_location);
    list_initialize(&global_call_ordering_head);
    initialize_source_location(&global_last_parameter_location);
}

static int has_leftover_values(const char *test_name) {
    int leftover_values = 0;
    (void)test_name;
    remove_always_return_values(&global_function_result_map_head, 1);
    if (check_for_leftover_values(
            &global_function_result_map_head,
            "Has remaining non-returned values", 1)) {
        leftover_values = 1;
    }

    remove_always_return_values(&global_function_parameter_map_head, 2);
    if (check_for_leftover_values(
            &global_function_parameter_map_head,
            "Parameter still has values that haven't been checked",
            2)) {
        leftover_values = 1;
    }

    remove_always_return_values_from_list(&global_call_ordering_head);
    if (check_for_leftover_values_list(&global_call_ordering_head,
        "Function was expected to be called but was not")) {
        leftover_values = 1;
    }
    return (leftover_values);
}

static void fail_if_leftover_values(const char *test_name) {
    if (has_leftover_values(test_name) != 0) {
        exit_test(true);
    }
}


static void teardown_testing(const char *test_name) {
    (void)test_name;
    list_free(&global_function_result_map_head, free_symbol_map_value,
              (void*)0);
    initialize_source_location(&global_last_mock_value_location);
    list_free(&global_function_parameter_map_head, free_symbol_map_value,
              (void*)1);
    initialize_source_location(&global_last_parameter_location);
    list_free(&global_call_ordering_head, free_value,
              (void*)0);
    initialize_source_location(&global_last_call_ordering_location);
}

/* Initialize a list node. */
static ListNode* list_initialize(ListNode * const node) {
    node->value = NULL;
    node->next = node;
    node->prev = node;
    node->refcount = 1;
    return node;
}


/*
 * Adds a value at the tail of a given list.
 * The node referencing the value is allocated from the heap.
 */
static ListNode* list_add_value(ListNode * const head, const void *value,
                                     const int refcount) {
    ListNode * const new_node = (ListNode*)malloc(sizeof(ListNode));
    assert_non_null(head);
    assert_non_null(value);
    new_node->value = value;
    new_node->refcount = refcount;
    return list_add(head, new_node);
}


/* Add new_node to the end of the list. */
static ListNode* list_add(ListNode * const head, ListNode *new_node) {
    assert_non_null(head);
    assert_non_null(new_node);
    new_node->next = head;
    new_node->prev = head->prev;
    head->prev->next = new_node;
    head->prev = new_node;
    return new_node;
}


/* Remove a node from a list. */
static ListNode* list_remove(
        ListNode * const node, const CleanupListValue cleanup_value,
        void * const cleanup_value_data) {
    assert_non_null(node);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    if (cleanup_value) {
        cleanup_value(node->value, cleanup_value_data);
    }
    return node;
}


/* Remove a list node from a list and free the node. */
static void list_remove_free(
        ListNode * const node, const CleanupListValue cleanup_value,
        void * const cleanup_value_data) {
    assert_non_null(node);
    free(list_remove(node, cleanup_value, cleanup_value_data));
}


/*
 * Frees memory kept by a linked list The cleanup_value function is called for
 * every "value" field of nodes in the list, except for the head.  In addition
 * to each list value, cleanup_value_data is passed to each call to
 * cleanup_value.  The head of the list is not deallocated.
 */
static ListNode* list_free(
        ListNode * const head, const CleanupListValue cleanup_value,
        void * const cleanup_value_data) {
    assert_non_null(head);
    while (!list_empty(head)) {
        list_remove_free(head->next, cleanup_value, cleanup_value_data);
    }
    return head;
}


/* Determine whether a list is empty. */
static int list_empty(const ListNode * const head) {
    assert_non_null(head);
    return head->next == head;
}


/*
 * Find a value in the list using the equal_func to compare each node with the
 * value.
 */
static int list_find(ListNode * const head, const void *value,
                     const EqualityFunction equal_func, ListNode **output) {
    ListNode *current;
    assert_non_null(head);
    for (current = head->next; current != head; current = current->next) {
        if (equal_func(current->value, value)) {
            *output = current;
            return 1;
        }
    }
    return 0;
}

/* Returns the first node of a list */
static int list_first(ListNode * const head, ListNode **output) {
    ListNode *target_node = NULL;
    assert_non_null(head);
    if (list_empty(head)) {
        return 0;
    }
    target_node = head->next;
    *output = target_node;
    return 1;
}


/* Deallocate a value referenced by a list. */
static void free_value(const void *value, void *cleanup_value_data) {
    (void)cleanup_value_data;
    assert_non_null(value);
    free((void*)value);
}


/* Releases memory associated to a symbol_map_value. */
static void free_symbol_map_value(const void *value,
                                  void *cleanup_value_data) {
    SymbolMapValue * const map_value = (SymbolMapValue*)value;
    const uintmax_t children = cast_ptr_to_uintmax_type(cleanup_value_data);
    assert_non_null(value);
    if (children == 0) {
        list_free(&map_value->symbol_values_list_head,
                  free_value,
                  NULL);
    } else {
        list_free(&map_value->symbol_values_list_head,
                  free_symbol_map_value,
                  (void *)((uintptr_t)children - 1));
    }

    free(map_value);
}


/*
 * Determine whether a symbol name referenced by a symbol_map_value matches the
 * specified function name.
 */
static int symbol_names_match(const void *map_value, const void *symbol) {
    return !strcmp(((SymbolMapValue*)map_value)->symbol_name,
                   (const char*)symbol);
}

/*
 * Adds a value to the queue of values associated with the given hierarchy of
 * symbols.  It's assumed value is allocated from the heap.
 */
static void add_symbol_value(ListNode * const symbol_map_head,
                             const char * const symbol_names[],
                             const size_t number_of_symbol_names,
                             const void* value, const int refcount) {
    const char* symbol_name;
    ListNode *target_node;
    SymbolMapValue *target_map_value;
    assert_non_null(symbol_map_head);
    assert_non_null(symbol_names);
    assert_true(number_of_symbol_names);
    symbol_name = symbol_names[0];

    if (!list_find(symbol_map_head, symbol_name, symbol_names_match,
                   &target_node)) {
        SymbolMapValue * const new_symbol_map_value =
            (SymbolMapValue*)malloc(sizeof(*new_symbol_map_value));
        new_symbol_map_value->symbol_name = symbol_name;
        list_initialize(&new_symbol_map_value->symbol_values_list_head);
        target_node = list_add_value(symbol_map_head, new_symbol_map_value,
                                          1);
    }

    target_map_value = (SymbolMapValue*)target_node->value;
    if (number_of_symbol_names == 1) {
            list_add_value(&target_map_value->symbol_values_list_head,
                                value, refcount);
    } else {
        add_symbol_value(&target_map_value->symbol_values_list_head,
                         &symbol_names[1], number_of_symbol_names - 1, value,
                         refcount);
    }
}


/*
 * Gets the next value associated with the given hierarchy of symbols.
 * The value is returned as an output parameter with the function returning the
 * node's old refcount value if a value is found, 0 otherwise.  This means that
 * a return value of 1 indicates the node was just removed from the list.
 */
static int get_symbol_value(
        ListNode * const head, const char * const symbol_names[],
        const size_t number_of_symbol_names, void **output) {
    const char* symbol_name = NULL;
    ListNode *target_node = NULL;
    assert_non_null(head);
    assert_non_null(symbol_names);
    assert_true(number_of_symbol_names);
    assert_non_null(output);
    symbol_name = symbol_names[0];

    if (list_find(head, symbol_name, symbol_names_match, &target_node)) {
        SymbolMapValue *map_value = NULL;
        ListNode *child_list = NULL;
        int return_value = 0;
        assert_non_null(target_node);
        assert_non_null(target_node->value);

        map_value = (SymbolMapValue*)target_node->value;
        child_list = &map_value->symbol_values_list_head;

        if (number_of_symbol_names == 1) {
            ListNode *value_node = NULL;
            return_value = list_first(child_list, &value_node);
            assert_true(return_value);
            /* Add a check to silence clang analyzer */
            if (return_value == 0) {
                goto out;
            }
            *output = (void*) value_node->value;
            return_value = value_node->refcount;
            if (value_node->refcount - 1 == 0) {
                list_remove_free(value_node, NULL, NULL);
            } else if (value_node->refcount > WILL_RETURN_ONCE) {
                --value_node->refcount;
            }
        } else {
            return_value = get_symbol_value(
                child_list, &symbol_names[1], number_of_symbol_names - 1,
                output);
        }
        if (list_empty(child_list)) {
            list_remove_free(target_node, free_symbol_map_value, (void*)0);
        }
        return return_value;
    }
out:
    cmocka_print_error("No entries for symbol %s.\n", symbol_name);
    return 0;
}

/**
 * Taverse a list of nodes and remove first symbol value in list that has a
 * refcount < -1 (i.e. should always be returned and has been returned at
 * least once).
 */

static void remove_always_return_values_from_list(ListNode * const map_head)
{
    ListNode * current = NULL;
    ListNode * next = NULL;
    assert_non_null(map_head);

    for (current = map_head->next, next = current->next;
            current != map_head;
            current = next, next = current->next) {
        if (current->refcount < -1) {
            list_remove_free(current, free_value, NULL);
        }
    }
}

/*
 * Traverse down a tree of symbol values and remove the first symbol value
 * in each branch that has a refcount < -1 (i.e should always be returned
 * and has been returned at least once).
 */
static void remove_always_return_values(ListNode * const map_head,
                                        const size_t number_of_symbol_names) {
    ListNode *current;
    assert_non_null(map_head);
    assert_true(number_of_symbol_names);
    current = map_head->next;
    while (current != map_head) {
        SymbolMapValue * const value = (SymbolMapValue*)current->value;
        ListNode * const next = current->next;
        ListNode *child_list;
        assert_non_null(value);
        child_list = &value->symbol_values_list_head;

        if (!list_empty(child_list)) {
            if (number_of_symbol_names == 1) {
                ListNode * const child_node = child_list->next;
                /* If this item has been returned more than once, free it. */
                if (child_node->refcount < -1) {
                    list_remove_free(child_node, free_value, NULL);
                }
            } else {
                remove_always_return_values(child_list,
                                            number_of_symbol_names - 1);
            }
        }

        if (list_empty(child_list)) {
            list_remove_free(current, free_value, NULL);
        }
        current = next;
    }
}

static size_t check_for_leftover_values_list(const ListNode * head,
                                             const char * const error_message)
{
    ListNode *child_node;
    size_t leftover_count = 0;
    if (!list_empty(head))
    {
        for (child_node = head->next; child_node != head;
                 child_node = child_node->next, ++leftover_count) {
            const FuncOrderingValue *const o =
                    (const FuncOrderingValue*) child_node->value;
            cmocka_print_error("%s: %s\n", error_message, o->function);
            cmocka_print_error(SOURCE_LOCATION_FORMAT
                    ": note: remaining item was declared here\n",
                    o->location.file, o->location.line);
        }
    }
    return leftover_count;
}

/*
 * Checks if there are any leftover values set up by the test that were never
 * retrieved through execution, and fail the test if that is the case.
 */
static size_t check_for_leftover_values(
        const ListNode * const map_head, const char * const error_message,
        const size_t number_of_symbol_names) {
    const ListNode *current;
    size_t symbols_with_leftover_values = 0;
    assert_non_null(map_head);
    assert_true(number_of_symbol_names);

    for (current = map_head->next; current != map_head;
         current = current->next) {
        const SymbolMapValue * const value =
            (SymbolMapValue*)current->value;
        const ListNode *child_list;
        assert_non_null(value);
        child_list = &value->symbol_values_list_head;

        if (!list_empty(child_list)) {
            if (number_of_symbol_names == 1) {
                const ListNode *child_node;
                cmocka_print_error("%s: %s\n", error_message, value->symbol_name);

                for (child_node = child_list->next; child_node != child_list;
                     child_node = child_node->next) {
                    const SourceLocation * const location =
                        (const SourceLocation*)child_node->value;
                    cmocka_print_error(SOURCE_LOCATION_FORMAT
                                   ": note: remaining item was declared here\n",
                                   location->file, location->line);
                }
            } else {
                cmocka_print_error("%s: ", value->symbol_name);
                check_for_leftover_values(child_list, error_message,
                                          number_of_symbol_names - 1);
            }
            symbols_with_leftover_values ++;
        }
    }
    return symbols_with_leftover_values;
}


/* Get the next return value for the specified mock function. */
uintmax_t _mock(const char * const function, const char* const file,
                          const int line) {
    void *result;
    const int rc = get_symbol_value(&global_function_result_map_head,
                                    &function, 1, &result);
    if (rc) {
        SymbolValue * const symbol = (SymbolValue*)result;
        const uintmax_t value = symbol->value;
        global_last_mock_value_location = symbol->location;
        if (rc == 1) {
            free(symbol);
        }
        return value;
    } else {
        cmocka_print_error(SOURCE_LOCATION_FORMAT ": error: Could not get value "
                       "to mock function %s\n", file, line, function);
        if (source_location_is_set(&global_last_mock_value_location)) {
            cmocka_print_error(SOURCE_LOCATION_FORMAT
                           ": note: Previously returned mock value was declared here\n",
                           global_last_mock_value_location.file,
                           global_last_mock_value_location.line);
        } else {
            cmocka_print_error("There were no previously returned mock values for "
                           "this test.\n");
        }
        exit_test(true);
    }
    return 0;
}

/* Ensure that function is being called in proper order */
void _function_called(const char *const function,
                      const char *const file,
                      const int line)
{
    if (list_empty(&global_call_ordering_head)) {
        cmocka_print_error(SOURCE_LOCATION_FORMAT
                       ": error: No mock calls expected but called() was "
                       "invoked in %s\n",
                       file, line,
                       function);
        exit_test(true);
    } else {
        const ListNode * const head = &global_call_ordering_head;
        ListNode *current = head->next;
        FuncOrderingValue *expected_call;
        bool found = false;

        /*
         * Search through value nodes until either function is found or
         * encounter a non-zero refcount greater than -2
         */
        do {
            expected_call = (FuncOrderingValue *)current->value;
            if (expected_call != NULL) {
                found = strcmp(expected_call->function, function) == 0;
            } else {
                found = false;
            }
            if (found || current->refcount > -2) {
                break;
            }

            current = current->next;
        } while (current != NULL && current != head);

        if (expected_call == NULL || current == head) {
            cmocka_print_error(SOURCE_LOCATION_FORMAT
                           ": error: No expected mock calls matching "
                           "called() invocation in %s\n",
                           file, line,
                           function);
            exit_test(true);
        }

        if (found) {
            if (current->refcount > -2) {
                current->refcount--;
                if (current->refcount == 0) {
                    list_remove_free(current, free_value, NULL);
                }
            }
        } else {
            cmocka_print_error(SOURCE_LOCATION_FORMAT
                           ": error: Expected call to %s but received called() "
                           "in %s\n",
                           file, line,
                           expected_call != NULL ?
                               expected_call->function : "(null)",
                           function);
            exit_test(true);
        }
    }
}

/* Add a return value for the specified mock function name. */
void _will_return(const char * const function_name, const char * const file,
                  const int line, const uintmax_t value,
                  const int count) {
    SymbolValue * const return_value =
        (SymbolValue*)malloc(sizeof(*return_value));
    assert_true(count != 0);
    return_value->value = value;
    set_source_location(&return_value->location, file, line);
    add_symbol_value(&global_function_result_map_head, &function_name, 1,
                     return_value, count);
}


/*
 * Add a custom parameter checking function.  If the event parameter is NULL
 * the event structure is allocated internally by this function.  If event
 * parameter is provided it must be allocated on the heap and doesn't need to
 * be deallocated by the caller.
 */
void _expect_check(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const CheckParameterValue check_function,
        const uintmax_t check_data,
        CheckParameterEvent * const event, const int count) {
    CheckParameterEvent * const check =
        event ? event : (CheckParameterEvent*)malloc(sizeof(*check));
    const char* symbols[] = {function, parameter};
    check->parameter_name = parameter;
    check->check_value = check_function;
    check->check_value_data = check_data;
    set_source_location(&check->location, file, line);
    add_symbol_value(&global_function_parameter_map_head, symbols, 2, check,
                     count);
}

/*
 * Add an call expectations that a particular function is called correctly.
 * This is used for code under test that makes calls to several functions
 * in depended upon components (mocks).
 */

void _expect_function_call(
    const char * const function_name,
    const char * const file,
    const int line,
    const int count)
{
    FuncOrderingValue *ordering;

    assert_non_null(function_name);
    assert_non_null(file);
    assert_true(count != 0);

    ordering = (FuncOrderingValue *)malloc(sizeof(*ordering));

    set_source_location(&ordering->location, file, line);
    ordering->function = function_name;

    list_add_value(&global_call_ordering_head, ordering, count);
}

/* Returns 1 if the specified float values are equal, else returns 0. */
static int float_compare(const float left,
                         const float right,
                         const float epsilon) {
    float absLeft;
    float absRight;
    float largest;
    float relDiff;

    float diff = left - right;
    diff = (diff >= 0.f) ? diff : -diff;

    // Check if the numbers are really close -- needed
        // when comparing numbers near zero.
        if (diff <= epsilon) {
            return 1;
    }

    absLeft = (left >= 0.f) ? left : -left;
    absRight = (right >= 0.f) ? right : -right;

    largest = (absRight > absLeft) ? absRight : absLeft;
    relDiff = largest * FLT_EPSILON;

    if (diff > relDiff) {
        return 0;
    }
    return 1;
}

/* Returns 1 if the specified float values are equal. If the values are not equal
 * an error is displayed and 0 is returned. */
static int float_values_equal_display_error(const float left,
                                            const float right,
                                            const float epsilon) {
    const int equal = float_compare(left, right, epsilon);
    if (!equal) {
        cmocka_print_error("%f != %f\n", left, right);
    }
    return equal;
}

/* Returns 1 if the specified float values are different. If the values are equal
 * an error is displayed and 0 is returned. */
static int float_values_not_equal_display_error(const float left,
                                                const float right,
                                                const float epsilon) {
    const int not_equal = (float_compare(left, right, epsilon) == 0);
    if (!not_equal) {
        cmocka_print_error("%f == %f\n", left, right);
    }
    return not_equal;
}

/* Returns 1 if the specified double values are equal, else returns 0. */
static int double_compare(const double left,
                          const double right,
                          const double epsilon) {
    double absLeft;
    double absRight;
    double largest;
    double relDiff;

    double diff = left - right;
    diff = (diff >= 0.f) ? diff : -diff;

    /*
     * Check if the numbers are really close -- needed
     * when comparing numbers near zero.
     */
    if (diff <= epsilon) {
        return 1;
    }

    absLeft = (left >= 0.f) ? left : -left;
    absRight = (right >= 0.f) ? right : -right;

    largest = (absRight > absLeft) ? absRight : absLeft;
    relDiff = largest * FLT_EPSILON;

    if (diff > relDiff) {
        return 0;
    }

    return 1;
}

/*
 * Returns 1 if the specified double values are equal. If the values are not
 * equal an error is displayed and 0 is returned.
 */
static int double_values_equal_display_error(const double left,
                                             const double right,
                                             const double epsilon) {
    const int equal = double_compare(left, right, epsilon);

    if (!equal) {
        cmocka_print_error("%f != %f\n", left, right);
    }

    return equal;
}

/*
 * Returns 1 if the specified double values are different. If the values are
 * equal an error is displayed and 0 is returned.
 */
static int double_values_not_equal_display_error(const double left,
                                                 const double right,
                                                 const double epsilon) {
    const int not_equal = (double_compare(left, right, epsilon) == 0);

    if (!not_equal) {
        cmocka_print_error("%f == %f\n", left, right);
    }

    return not_equal;
}

/* Returns 1 if the specified values are equal.  If the values are not equal
 * an error is displayed and 0 is returned. */
static bool uint_values_equal_display_error(const uintmax_t left,
                                            const uintmax_t right)
{
    const bool equal = left == right;
    if (!equal) {
        cmocka_print_error("%ju (%#jx) != %ju (%#jx)\n",
                           left,
                           left,
                           right,
                           right);
    }
    return equal;
}


/* Returns 1 if the specified values are equal.  If the values are not equal
 * an error is displayed and 0 is returned. */
static bool int_values_equal_display_error(const intmax_t left,
                                           const intmax_t right)
{
    const bool equal = left == right;
    if (!equal) {
        cmocka_print_error("%jd != %jd\n", left, right);
    }
    return equal;
}


/*
 * Returns 1 if the specified values are not equal.  If the values are equal
 * an error is displayed and 0 is returned. */
static bool uint_values_not_equal_display_error(const uintmax_t left,
                                                const uintmax_t right) {
    const bool not_equal = left != right;
    if (!not_equal) {
        cmocka_print_error("%ju (%#jx) == %ju (%#jx)\n",
                           left,
                           left,
                           right,
                           right);
    }
    return not_equal;
}


/*
 * Returns 1 if the specified values are not equal.  If the values are equal
 * an error is displayed and 0 is returned. */
static bool int_values_not_equal_display_error(const intmax_t left,
                                               const intmax_t right)
{
    const bool not_equal = left != right;
    if (!not_equal) {
        cmocka_print_error("%jd == %jd\n", left, right);
    }
    return not_equal;
}


/*
 * Determine whether value is contained within check_integer_set.
 * If invert is 0 and the value is in the set 1 is returned, otherwise 0 is
 * returned and an error is displayed.  If invert is 1 and the value is not
 * in the set 1 is returned, otherwise 0 is returned and an error is
 * displayed.
 */
static int value_in_set_display_error(
        const uintmax_t value,
        const CheckIntegerSet * const check_integer_set, const int invert) {
    int succeeded = invert;
    assert_non_null(check_integer_set);
    {
        const uintmax_t * const set = check_integer_set->set;
        const size_t size_of_set = check_integer_set->size_of_set;
        size_t i;
        for (i = 0; i < size_of_set; i++) {
            if (set[i] == value) {
                /* If invert = 0 and item is found, succeeded = 1. */
                /* If invert = 1 and item is found, succeeded = 0. */
                succeeded = !succeeded;
                break;
            }
        }
        if (succeeded) {
            return 1;
        }
        cmocka_print_error("%ju is %sin the set (",
                       value, invert ? "" : "not ");
        for (i = 0; i < size_of_set; i++) {
            cmocka_print_error("%#jx, ", set[i]);
        }
        cmocka_print_error(")\n");
    }
    return 0;
}


/*
 * Determine whether a value is within the specified range.
 */
static bool uint_in_range_display_error(const uintmax_t value,
                                        const uintmax_t range_min,
                                        const uintmax_t range_max)
{
    if (value >= range_min && value <= range_max) {
        return true;
    }

    cmocka_print_error("%ju is not within the range [%ju, %ju]\n",
                       value,
                       range_min,
                       range_max);

    return false;
}


/*
 * Determine whether a value is within the specified range.
 */
static bool int_in_range_display_error(const intmax_t value,
                                       const intmax_t range_min,
                                       const intmax_t range_max)
{
    if (value >= range_min && value <= range_max) {
        return true;
    }

    cmocka_print_error("%jd is not within the range [%jd, %jd]\n",
                       value,
                       range_min,
                       range_max);

    return false;
}


/*
 * Determine whether a value is within the specified range.  If the value
 * is not within the range 1 is returned.  If the value is within the
 * specified range an error is displayed and zero is returned.
 */
static int integer_not_in_range_display_error(
        const uintmax_t value, const uintmax_t range_min,
        const uintmax_t range_max) {
    if (value < range_min || value > range_max) {
        return 1;
    }
    cmocka_print_error("%ju is within the range %ju-%ju\n",
                       value,
                       range_min,
                       range_max);
    return 0;
}


/*
 * Determine whether the specified strings are equal.  If the strings are equal
 * 1 is returned.  If they're not equal an error is displayed and 0 is
 * returned.
 */
static int string_equal_display_error(
        const char * const left, const char * const right) {
    if (strcmp(left, right) == 0) {
        return 1;
    }
    cmocka_print_error("\"%s\" != \"%s\"\n", left, right);
    return 0;
}


/*
 * Determine whether the specified strings are equal.  If the strings are not
 * equal 1 is returned.  If they're not equal an error is displayed and 0 is
 * returned
 */
static int string_not_equal_display_error(
        const char * const left, const char * const right) {
    if (strcmp(left, right) != 0) {
        return 1;
    }
    cmocka_print_error("\"%s\" == \"%s\"\n", left, right);
    return 0;
}


/*
 * Determine whether the specified areas of memory are equal.  If they're equal
 * 1 is returned otherwise an error is displayed and 0 is returned.
 */
static int memory_equal_display_error(const char* const a, const char* const b,
                                      const size_t size) {
    size_t differences = 0;
    size_t i;
    for (i = 0; i < size; i++) {
        const char l = a[i];
        const char r = b[i];
        if (l != r) {
            if (differences < 16) {
                cmocka_print_error("difference at offset %" PRIdS " 0x%02x 0x%02x\n",
                               i, l, r);
            }
            differences ++;
        }
    }
    if (differences > 0) {
        if (differences >= 16) {
            cmocka_print_error("...\n");
        }
        cmocka_print_error("%"PRIdS " bytes of %p and %p differ\n",
                       differences, (void *)a, (void *)b);
        return 0;
    }
    return 1;
}


/*
 * Determine whether the specified areas of memory are not equal.  If they're
 * not equal 1 is returned otherwise an error is displayed and 0 is
 * returned.
 */
static int memory_not_equal_display_error(
        const char* const a, const char* const b, const size_t size) {
    size_t same = 0;
    size_t i;
    for (i = 0; i < size; i++) {
        const char l = a[i];
        const char r = b[i];
        if (l == r) {
            same ++;
        }
    }
    if (same == size) {
        cmocka_print_error("%"PRIdS "bytes of %p and %p the same\n",
                       same, (void *)a, (void *)b);
        return 0;
    }
    return 1;
}


/* CheckParameterValue callback to check whether a value is within a set. */
static int check_in_set(const uintmax_t value,
                        const uintmax_t check_value_data) {
    return value_in_set_display_error(value,
        cast_uintmax_type_to_pointer(CheckIntegerSet*,
                                              check_value_data), 0);
}


/* CheckParameterValue callback to check whether a value isn't within a set. */
static int check_not_in_set(const uintmax_t value,
                            const uintmax_t check_value_data) {
    return value_in_set_display_error(value,
        cast_uintmax_type_to_pointer(CheckIntegerSet*,
                                              check_value_data), 1);
}


/* Create the callback data for check_in_set() or check_not_in_set() and
 * register a check event. */
static void expect_set(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t values[], const size_t number_of_values,
        const CheckParameterValue check_function, const int count) {
    CheckIntegerSet * const check_integer_set =
        (CheckIntegerSet*)malloc(sizeof(*check_integer_set) +
               (sizeof(values[0]) * number_of_values));
    uintmax_t * const set = (uintmax_t*)(
        check_integer_set + 1);
    declare_initialize_value_pointer_pointer(check_data, check_integer_set);
    assert_non_null(values);
    assert_true(number_of_values);
    memcpy(set, values, number_of_values * sizeof(values[0]));
    check_integer_set->set = set;
    check_integer_set->size_of_set = number_of_values;
    _expect_check(
        function, parameter, file, line, check_function,
        check_data, &check_integer_set->event, count);
}


/* Add an event to check whether a value is in a set. */
void _expect_in_set(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t values[], const size_t number_of_values,
        const int count) {
    expect_set(function, parameter, file, line, values, number_of_values,
               check_in_set, count);
}


/* Add an event to check whether a value isn't in a set. */
void _expect_not_in_set(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t values[], const size_t number_of_values,
        const int count) {
    expect_set(function, parameter, file, line, values, number_of_values,
               check_not_in_set, count);
}


/* CheckParameterValue callback to check whether a value is within a range. */
static int check_in_range(const uintmax_t value,
                          const uintmax_t check_value_data) {
    CheckIntegerRange * const check_integer_range =
        cast_uintmax_type_to_pointer(CheckIntegerRange*,
                                              check_value_data);
    assert_non_null(check_integer_range);

    return uint_in_range_display_error(value,
                                       check_integer_range->minimum,
                                       check_integer_range->maximum);
}


/* CheckParameterValue callback to check whether a value is not within a range. */
static int check_not_in_range(const uintmax_t value,
                              const uintmax_t check_value_data) {
    CheckIntegerRange * const check_integer_range =
        cast_uintmax_type_to_pointer(CheckIntegerRange*,
                                              check_value_data);
    assert_non_null(check_integer_range);
    return integer_not_in_range_display_error(
        value, check_integer_range->minimum, check_integer_range->maximum);
}


/* Create the callback data for check_in_range() or check_not_in_range() and
 * register a check event. */
static void expect_range(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t minimum, const uintmax_t maximum,
        const CheckParameterValue check_function, const int count) {
    CheckIntegerRange * const check_integer_range =
        (CheckIntegerRange*)malloc(sizeof(*check_integer_range));
    declare_initialize_value_pointer_pointer(check_data, check_integer_range);
    check_integer_range->minimum = minimum;
    check_integer_range->maximum = maximum;
    _expect_check(function, parameter, file, line, check_function,
                  check_data, &check_integer_range->event, count);
}


/* Add an event to determine whether a parameter is within a range. */
void _expect_in_range(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t minimum, const uintmax_t maximum,
        const int count) {
    expect_range(function, parameter, file, line, minimum, maximum,
                 check_in_range, count);
}


/* Add an event to determine whether a parameter is not within a range. */
void _expect_not_in_range(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t minimum, const uintmax_t maximum,
        const int count) {
    expect_range(function, parameter, file, line, minimum, maximum,
                 check_not_in_range, count);
}


/* CheckParameterValue callback to check whether a value is equal to an
 * expected value. */
static int check_value(const uintmax_t value,
                       const uintmax_t check_value_data) {
    return uint_values_equal_display_error(value, check_value_data);
}


/* Add an event to check a parameter equals an expected value. */
void _expect_value(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t value, const int count) {
    _expect_check(function, parameter, file, line, check_value, value, NULL,
                  count);
}


/* CheckParameterValue callback to check whether a value is not equal to an
 * expected value. */
static int check_not_value(const uintmax_t value,
                           const uintmax_t check_value_data) {
    return uint_values_not_equal_display_error(value, check_value_data);
}


/* Add an event to check a parameter is not equal to an expected value. */
void _expect_not_value(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const uintmax_t value, const int count) {
    _expect_check(function, parameter, file, line, check_not_value, value,
                  NULL, count);
}


/* CheckParameterValue callback to check whether a parameter equals a string. */
static int check_string(const uintmax_t value,
                        const uintmax_t check_value_data) {
    return string_equal_display_error(
        cast_uintmax_type_to_pointer(char*, value),
        cast_uintmax_type_to_pointer(char*, check_value_data));
}


/* Add an event to check whether a parameter is equal to a string. */
void _expect_string(
        const char* const function, const char* const parameter,
        const char* const file, const int line, const char* string,
        const int count) {
    declare_initialize_value_pointer_pointer(string_pointer,
                                             discard_const(string));
    _expect_check(function, parameter, file, line, check_string,
                  string_pointer, NULL, count);
}


/* CheckParameterValue callback to check whether a parameter is not equals to
 * a string. */
static int check_not_string(const uintmax_t value,
                            const uintmax_t check_value_data) {
    return string_not_equal_display_error(
        cast_uintmax_type_to_pointer(char*, value),
        cast_uintmax_type_to_pointer(char*, check_value_data));
}


/* Add an event to check whether a parameter is not equal to a string. */
void _expect_not_string(
        const char* const function, const char* const parameter,
        const char* const file, const int line, const char* string,
        const int count) {
    declare_initialize_value_pointer_pointer(string_pointer,
                                             discard_const(string));
    _expect_check(function, parameter, file, line, check_not_string,
                  string_pointer, NULL, count);
}

/* CheckParameterValue callback to check whether a parameter equals an area of
 * memory. */
static int check_memory(const uintmax_t value,
                        const uintmax_t check_value_data) {
    CheckMemoryData * const check = cast_uintmax_type_to_pointer(
        CheckMemoryData*, check_value_data);
    assert_non_null(check);
    return memory_equal_display_error(
        cast_uintmax_type_to_pointer(const char*, value),
        (const char*)check->memory, check->size);
}


/* Create the callback data for check_memory() or check_not_memory() and
 * register a check event. */
static void expect_memory_setup(
        const char* const function, const char* const parameter,
        const char* const file, const int line,
        const void * const memory, const size_t size,
        const CheckParameterValue check_function, const int count) {
    CheckMemoryData * const check_data =
        (CheckMemoryData*)malloc(sizeof(*check_data) + size);
    void * const mem = (void*)(check_data + 1);
    declare_initialize_value_pointer_pointer(check_data_pointer, check_data);
    assert_non_null(memory);
    assert_true(size);
    memcpy(mem, memory, size);
    check_data->memory = mem;
    check_data->size = size;
    _expect_check(function, parameter, file, line, check_function,
                  check_data_pointer, &check_data->event, count);
}


/* Add an event to check whether a parameter matches an area of memory. */
void _expect_memory(
        const char* const function, const char* const parameter,
        const char* const file, const int line, const void* const memory,
        const size_t size, const int count) {
    expect_memory_setup(function, parameter, file, line, memory, size,
                        check_memory, count);
}


/* CheckParameterValue callback to check whether a parameter is not equal to
 * an area of memory. */
static int check_not_memory(const uintmax_t value,
                            const uintmax_t check_value_data) {
    CheckMemoryData * const check = cast_uintmax_type_to_pointer(
        CheckMemoryData*, check_value_data);
    assert_non_null(check);
    return memory_not_equal_display_error(
        cast_uintmax_type_to_pointer(const char*, value),
        (const char*)check->memory,
        check->size);
}


/* Add an event to check whether a parameter doesn't match an area of memory. */
void _expect_not_memory(
        const char* const function, const char* const parameter,
        const char* const file, const int line, const void* const memory,
        const size_t size, const int count) {
    expect_memory_setup(function, parameter, file, line, memory, size,
                        check_not_memory, count);
}


/* CheckParameterValue callback that always returns 1. */
static int check_any(const uintmax_t value,
                     const uintmax_t check_value_data) {
    (void)value;
    (void)check_value_data;
    return 1;
}


/* Add an event to allow any value for a parameter. */
void _expect_any(
        const char* const function, const char* const parameter,
        const char* const file, const int line, const int count) {
    _expect_check(function, parameter, file, line, check_any, 0, NULL,
                  count);
}


void _check_expected(
        const char * const function_name, const char * const parameter_name,
        const char* file, const int line, const uintmax_t value) {
    void *result = NULL;
    const char* symbols[] = {function_name, parameter_name};
    const int rc = get_symbol_value(&global_function_parameter_map_head,
                                    symbols, 2, &result);
    if (rc) {
        CheckParameterEvent * const check = (CheckParameterEvent*)result;
        int check_succeeded;
        global_last_parameter_location = check->location;
        check_succeeded = check->check_value(value, check->check_value_data);
        if (rc == 1) {
            free(check);
        }
        if (!check_succeeded) {
            cmocka_print_error(SOURCE_LOCATION_FORMAT
                           ": error: Check of parameter %s, function %s failed\n"
                           SOURCE_LOCATION_FORMAT
                           ": note: Expected parameter declared here\n",
                           file, line,
                           parameter_name, function_name,
                           global_last_parameter_location.file,
                           global_last_parameter_location.line);
            _fail(file, line);
        }
    } else {
        cmocka_print_error(SOURCE_LOCATION_FORMAT ": error: Could not get value "
                    "to check parameter %s of function %s\n", file, line,
                    parameter_name, function_name);
        if (source_location_is_set(&global_last_parameter_location)) {
            cmocka_print_error(SOURCE_LOCATION_FORMAT
                        ": note: Previously declared parameter value was declared here\n",
                        global_last_parameter_location.file,
                        global_last_parameter_location.line);
        } else {
            cmocka_print_error("There were no previously declared parameter values "
                        "for this test.\n");
        }
        exit_test(true);
    }
}


/* Replacement for assert. */
void mock_assert(const int result, const char* const expression,
                 const char* const file, const int line) {
    if (!result) {
        if (global_expecting_assert) {
            global_last_failed_assert = expression;
            longjmp(global_expect_assert_env, result);
        } else {
            cmocka_print_error("ASSERT: %s\n", expression);
            _fail(file, line);
        }
    }
}


void _assert_true(const uintmax_t result,
                  const char * const expression,
                  const char * const file, const int line) {
    if (!result) {
        cmocka_print_error("%s\n", expression);
        _fail(file, line);
    }
}

void _assert_return_code(const intmax_t result,
                         const int32_t error,
                         const char * const expression,
                         const char * const file,
                         const int line)
{
    if (result < 0) {
        if (error > 0) {
            cmocka_print_error("%s < 0, errno(%d): %s\n",
                           expression,
                           error,
                           strerror(error));
        } else {
            cmocka_print_error("%s < 0\n", expression);
        }
        _fail(file, line);
    }
}

void _assert_float_equal(const float a,
                         const float b,
                         const float epsilon,
                         const char * const file,
                         const int line) {
    if (!float_values_equal_display_error(a, b, epsilon)) {
        _fail(file, line);
    }
}

void _assert_float_not_equal(const float a,
                             const float b,
                             const float epsilon,
                             const char * const file,
                             const int line) {
    if (!float_values_not_equal_display_error(a, b, epsilon)) {
        _fail(file, line);
    }
}

void _assert_double_equal(const double a,
                          const double b,
                          const double epsilon,
                          const char * const file,
                          const int line) {
    if (!double_values_equal_display_error(a, b, epsilon)) {
        _fail(file, line);
    }
}

void _assert_double_not_equal(const double a,
                              const double b,
                              const double epsilon,
                              const char * const file,
                              const int line) {
    if (!double_values_not_equal_display_error(a, b, epsilon)) {
        _fail(file, line);
    }
}

void _assert_int_equal(const intmax_t a,
                       const intmax_t b,
                       const char * const file,
                       const int line)
{
    if (!int_values_equal_display_error(a, b)) {
        _fail(file, line);
    }
}


void _assert_int_not_equal(const intmax_t a,
                           const intmax_t b,
                           const char * const file,
                           const int line)
{
    if (!int_values_not_equal_display_error(a, b)) {
        _fail(file, line);
    }
}


void _assert_uint_equal(const uintmax_t a,
                        const uintmax_t b,
                        const char * const file,
                        const int line)
{
    if (!uint_values_equal_display_error(a, b)) {
        _fail(file, line);
    }
}


void _assert_uint_not_equal(const uintmax_t a,
                            const uintmax_t b,
                            const char * const file,
                            const int line)
{
    if (!uint_values_not_equal_display_error(a, b)) {
        _fail(file, line);
    }
}


void _assert_string_equal(const char * const a, const char * const b,
                          const char * const file, const int line) {
    if (!string_equal_display_error(a, b)) {
        _fail(file, line);
    }
}


void _assert_string_not_equal(const char * const a, const char * const b,
                              const char *file, const int line) {
    if (!string_not_equal_display_error(a, b)) {
        _fail(file, line);
    }
}


void _assert_memory_equal(const void * const a, const void * const b,
                          const size_t size, const char* const file,
                          const int line) {
    if (!memory_equal_display_error((const char*)a, (const char*)b, size)) {
        _fail(file, line);
    }
}


void _assert_memory_not_equal(const void * const a, const void * const b,
                              const size_t size, const char* const file,
                              const int line) {
    if (!memory_not_equal_display_error((const char*)a, (const char*)b,
                                        size)) {
        _fail(file, line);
    }
}


void _assert_int_in_range(const intmax_t value,
                          const intmax_t minimum,
                          const intmax_t maximum,
                          const char* const file,
                          const int line)
{
    if (!int_in_range_display_error(value, minimum, maximum)) {
        _fail(file, line);
    }
}


void _assert_uint_in_range(const uintmax_t value,
                           const uintmax_t minimum,
                           const uintmax_t maximum,
                           const char* const file,
                           const int line)
{
    if (!uint_in_range_display_error(value, minimum, maximum)) {
        _fail(file, line);
    }
}


void _assert_in_range(
        const uintmax_t value, const uintmax_t minimum,
        const uintmax_t maximum, const char* const file,
        const int line) {
    if (!uint_in_range_display_error(value, minimum, maximum)) {
        _fail(file, line);
    }
}

void _assert_not_in_range(
        const uintmax_t value, const uintmax_t minimum,
        const uintmax_t maximum, const char* const file,
        const int line) {
    if (!integer_not_in_range_display_error(value, minimum, maximum)) {
        _fail(file, line);
    }
}

void _assert_in_set(const uintmax_t value,
                    const uintmax_t values[],
                    const size_t number_of_values, const char* const file,
                    const int line) {
    CheckIntegerSet check_integer_set;
    check_integer_set.set = values;
    check_integer_set.size_of_set = number_of_values;
    if (!value_in_set_display_error(value, &check_integer_set, 0)) {
        _fail(file, line);
    }
}

void _assert_not_in_set(const uintmax_t value,
                        const uintmax_t values[],
                        const size_t number_of_values, const char* const file,
                        const int line) {
    CheckIntegerSet check_integer_set;
    check_integer_set.set = values;
    check_integer_set.size_of_set = number_of_values;
    if (!value_in_set_display_error(value, &check_integer_set, 1)) {
        _fail(file, line);
    }
}


/* Get the list of allocated blocks. */
static ListNode* get_allocated_blocks_list(void) {
    /* If it initialized, initialize the list of allocated blocks. */
    if (!global_allocated_blocks.value) {
        list_initialize(&global_allocated_blocks);
        global_allocated_blocks.value = (void*)1;
    }
    return &global_allocated_blocks;
}

static void *libc_calloc(size_t nmemb, size_t size)
{
#undef calloc
    return calloc(nmemb, size);
#define calloc test_calloc
}

static void libc_free(void *ptr)
{
#undef free
    free(ptr);
#define free test_free
}

static void *libc_realloc(void *ptr, size_t size)
{
#undef realloc
    return realloc(ptr, size);
#define realloc test_realloc
}

static void vcmocka_print_error(const char* const format,
                            va_list args) CMOCKA_PRINTF_ATTRIBUTE(1, 0);

/* It's important to use the libc malloc and free here otherwise
 * the automatic free of leaked blocks can reap the error messages
 */
static void vcmocka_print_error(const char* const format, va_list args)
{
    char buffer[1024];
    size_t msg_len = 0;
    va_list ap;
    int len;
    va_copy(ap, args);

    len = vsnprintf(buffer, sizeof(buffer), format, args);
    if (len < 0) {
        /* TODO */
        goto end;
    }

    if (cm_error_message == NULL) {
        /* CREATE MESSAGE */

        cm_error_message = libc_calloc(1, len + 1);
        if (cm_error_message == NULL) {
            /* TODO */
            goto end;
        }
    } else {
        /* APPEND MESSAGE */
        char *tmp;

        msg_len = strlen(cm_error_message);
        tmp = libc_realloc(cm_error_message, msg_len + len + 1);
        if (tmp == NULL) {
            goto end;
        }
        cm_error_message = tmp;
    }

    if (((size_t)len) < sizeof(buffer)) {
        /* Use len + 1 to also copy '\0' */
        memcpy(cm_error_message + msg_len, buffer, len + 1);
    } else {
        vsnprintf(cm_error_message + msg_len, len, format, ap);
    }
end:
    va_end(ap);

}

static void vcm_free_error(char *err_msg)
{
    libc_free(err_msg);
}

/* Use the real malloc in this function. */
#undef malloc
void* _test_malloc(const size_t size, const char* file, const int line) {
    char *ptr = NULL;
    MallocBlockInfo block_info;
    ListNode * const block_list = get_allocated_blocks_list();
    size_t allocate_size;
    char *block = NULL;

    allocate_size = size + (MALLOC_GUARD_SIZE * 2) +
                    sizeof(struct MallocBlockInfoData) + MALLOC_ALIGNMENT;
    assert_true(allocate_size > size);

    block = (char *)malloc(allocate_size);
    assert_non_null(block);

    /* Calculate the returned address. */
    ptr = (char*)(((size_t)block + MALLOC_GUARD_SIZE +
                  sizeof(struct MallocBlockInfoData) +
                  MALLOC_ALIGNMENT) & ~(MALLOC_ALIGNMENT - 1));

    /* Initialize the guard blocks. */
    memset(ptr - MALLOC_GUARD_SIZE, MALLOC_GUARD_PATTERN, MALLOC_GUARD_SIZE);
    memset(ptr + size, MALLOC_GUARD_PATTERN, MALLOC_GUARD_SIZE);
    memset(ptr, MALLOC_ALLOC_PATTERN, size);

    block_info.ptr = ptr - (MALLOC_GUARD_SIZE +
                            sizeof(struct MallocBlockInfoData));
    set_source_location(&block_info.data->location, file, line);
    block_info.data->allocated_size = allocate_size;
    block_info.data->size = size;
    block_info.data->block = block;
    block_info.data->node.value = block_info.ptr;
    list_add(block_list, &block_info.data->node);
    return ptr;
}
#define malloc test_malloc


void* _test_calloc(const size_t number_of_elements, const size_t size,
                   const char* file, const int line) {
    void* const ptr = _test_malloc(number_of_elements * size, file, line);
    if (ptr) {
        memset(ptr, 0, number_of_elements * size);
    }
    return ptr;
}
#define calloc test_calloc


/* Use the real free in this function. */
#undef free
void _test_free(void* const ptr, const char* file, const int line) {
    unsigned int i;
    char *block = discard_const_p(char, ptr);
    MallocBlockInfo block_info;

    if (ptr == NULL) {
        return;
    }

    _assert_true(cast_ptr_to_uintmax_type(ptr), "ptr", file, line);
    block_info.ptr = block - (MALLOC_GUARD_SIZE +
                              sizeof(struct MallocBlockInfoData));
    /* Check the guard blocks. */
    {
        char *guards[2] = {block - MALLOC_GUARD_SIZE,
                           block + block_info.data->size};
        for (i = 0; i < ARRAY_SIZE(guards); i++) {
            unsigned int j;
            char * const guard = guards[i];
            for (j = 0; j < MALLOC_GUARD_SIZE; j++) {
                const char diff = guard[j] - MALLOC_GUARD_PATTERN;
                if (diff) {
                    cmocka_print_error(SOURCE_LOCATION_FORMAT
                                   ": error: Guard block of %p size=%lu is corrupt\n"
                                   SOURCE_LOCATION_FORMAT ": note: allocated here at %p\n",
                                   file,
                                   line,
                                   ptr,
                                   (unsigned long)block_info.data->size,
                                   block_info.data->location.file,
                                   block_info.data->location.line,
                                   (void *)&guard[j]);
                    _fail(file, line);
                }
            }
        }
    }
    list_remove(&block_info.data->node, NULL, NULL);

    block = discard_const_p(char, block_info.data->block);
    memset(block, MALLOC_FREE_PATTERN, block_info.data->allocated_size);
    free(block);
}
#define free test_free

#undef realloc
void *_test_realloc(void *ptr,
                   const size_t size,
                   const char *file,
                   const int line)
{
    MallocBlockInfo block_info;
    char *block = ptr;
    size_t block_size = size;
    void *new_block;

    if (ptr == NULL) {
        return _test_malloc(size, file, line);
    }

    if (size == 0) {
        _test_free(ptr, file, line);
        return NULL;
    }

    block_info.ptr = block - (MALLOC_GUARD_SIZE +
                              sizeof(struct MallocBlockInfoData));

    new_block = _test_malloc(size, file, line);
    if (new_block == NULL) {
        return NULL;
    }

    if (block_info.data->size < size) {
        block_size = block_info.data->size;
    }

    memcpy(new_block, ptr, block_size);

    /* Free previous memory */
    _test_free(ptr, file, line);

    return new_block;
}
#define realloc test_realloc

/* Crudely checkpoint the current heap state. */
static const ListNode* check_point_allocated_blocks(void) {
    return get_allocated_blocks_list()->prev;
}


/* Display the blocks allocated after the specified check point.  This
 * function returns the number of blocks displayed. */
static size_t display_allocated_blocks(const ListNode * const check_point) {
    const ListNode * const head = get_allocated_blocks_list();
    const ListNode *node;
    size_t allocated_blocks = 0;
    assert_non_null(check_point);
    assert_non_null(check_point->next);

    for (node = check_point->next; node != head; node = node->next) {
        const MallocBlockInfo block_info = {
            .ptr = discard_const(node->value),
        };
        assert_non_null(block_info.ptr);

        if (allocated_blocks == 0) {
            cmocka_print_error("Blocks allocated...\n");
        }
        cmocka_print_error(SOURCE_LOCATION_FORMAT ": note: block %p allocated here\n",
                       block_info.data->location.file,
                       block_info.data->location.line,
                       block_info.data->block);
        allocated_blocks++;
    }
    return allocated_blocks;
}


/* Free all blocks allocated after the specified check point. */
static void free_allocated_blocks(const ListNode * const check_point) {
    const ListNode * const head = get_allocated_blocks_list();
    const ListNode *node;
    assert_non_null(check_point);

    node = check_point->next;
    assert_non_null(node);

    while (node != head) {
        const MallocBlockInfo block_info = {
            .ptr = discard_const(node->value),
        };
        node = node->next;
        free(discard_const_p(char, block_info.data) +
             sizeof(struct MallocBlockInfoData) +
             MALLOC_GUARD_SIZE);
    }
}


/* Fail if any any blocks are allocated after the specified check point. */
static void fail_if_blocks_allocated(const ListNode * const check_point,
                                     const char * const test_name) {
    const size_t allocated_blocks = display_allocated_blocks(check_point);
    if (allocated_blocks > 0) {
        free_allocated_blocks(check_point);
        cmocka_print_error("ERROR: %s leaked %zu block(s)\n", test_name,
                       allocated_blocks);
        exit_test(true);
    }
}


void _fail(const char * const file, const int line) {
    uint32_t output = cm_get_output();

    if (output & CM_OUTPUT_STANDARD) {
        cmocka_print_error("[   LINE   ] --- " SOURCE_LOCATION_FORMAT
                       ": error: Failure!",
                       file, line);
    }
    if ((output & CM_OUTPUT_SUBUNIT) || (output & CM_OUTPUT_TAP) ||
        (output & CM_OUTPUT_XML)) {
        cmocka_print_error(SOURCE_LOCATION_FORMAT ": error: Failure!", file, line);
    }
    exit_test(true);

    /* Unreachable */
    exit(EXIT_FAILURE);
}


#ifndef _WIN32
CMOCKA_NORETURN static void exception_handler(int sig) {
    const char *sig_strerror = "";

#ifdef HAVE_STRSIGNAL
    sig_strerror = strsignal(sig);
#endif

    cmocka_print_error("Test failed with exception: %s(%d)",
                   sig_strerror, sig);
    exit_test(true);

    /* Unreachable */
    exit(EXIT_FAILURE);
}

#else /* _WIN32 */

static LONG WINAPI exception_filter(EXCEPTION_POINTERS *exception_pointers) {
    EXCEPTION_RECORD * const exception_record =
        exception_pointers->ExceptionRecord;
    const DWORD code = exception_record->ExceptionCode;
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(exception_codes); i++) {
        const ExceptionCodeInfo * const code_info = &exception_codes[i];
        if (code == code_info->code) {
            static int shown_debug_message = 0;
            fflush(stdout);
            cmocka_print_error("%s occurred at %p.\n", code_info->description,
                        exception_record->ExceptionAddress);
            if (!shown_debug_message) {
                cmocka_print_error(
                    "\n"
                    "To debug in Visual Studio...\n"
                    "1. Select menu item File->Open Project\n"
                    "2. Change 'Files of type' to 'Executable Files'\n"
                    "3. Open this executable.\n"
                    "4. Select menu item Debug->Start\n"
                    "\n"
                    "Alternatively, set the environment variable \n"
                    "UNIT_TESTING_DEBUG to 1 and rebuild this executable, \n"
                    "then click 'Debug' in the popup dialog box.\n"
                    "\n");
                shown_debug_message = 1;
            }
            exit_test(false);
            return EXCEPTION_EXECUTE_HANDLER;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif /* !_WIN32 */

void cmocka_print_error(const char * const format, ...)
{
    va_list args;
    va_start(args, format);
    if (cm_error_message_enabled) {
        vcmocka_print_error(format, args);
    } else {
        vprint_error(format, args);
    }
    va_end(args);
}

/* Standard output and error print methods. */
void vprint_message(const char* const format, va_list args)
{
    char buffer[4096];

    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s", buffer);
    fflush(stdout);
#ifdef _WIN32
    OutputDebugString(buffer);
#endif /* _WIN32 */
}


void vprint_error(const char* const format, va_list args)
{
    char buffer[4096];

    vsnprintf(buffer, sizeof(buffer), format, args);
    fprintf(stderr, "%s", buffer);
    fflush(stderr);
#ifdef _WIN32
    OutputDebugString(buffer);
#endif /* _WIN32 */
}


void print_message(const char* const format, ...) {
    va_list args;
    va_start(args, format);
    vprint_message(format, args);
    va_end(args);
}


void print_error(const char* const format, ...) {
    va_list args;
    va_start(args, format);
    vprint_error(format, args);
    va_end(args);
}

/* New formatter */
static uint32_t cm_get_output(void)
{
    static bool env_checked = false;
    char *env = NULL;
    size_t len = 0;
    uint32_t new_output = 0;
    char *str_output_list = NULL;
    char *str_output = NULL;
    char *saveptr = NULL;

    if (env_checked) {
        return global_msg_output;
    }

    env = getenv("CMOCKA_MESSAGE_OUTPUT");
    if (env == NULL) {
        return global_msg_output;
    }

    len = strlen(env);
    if (len == 0 || len > 32) {
        return global_msg_output;
    }

    str_output_list = strdup(env);
    if (str_output_list == NULL) {
        return global_msg_output;
    }

    for (str_output = strtok_r(str_output_list, ",", &saveptr);
         str_output != NULL;
         str_output = strtok_r(NULL, ",", &saveptr)) {
        if (strcasecmp(str_output, "STANDARD") == 0) {
            new_output |= CM_OUTPUT_STANDARD;
        } else if (strcasecmp(str_output, "STDOUT") == 0) {
            new_output |= CM_OUTPUT_STANDARD;
        } else if (strcasecmp(str_output, "SUBUNIT") == 0) {
            new_output |= CM_OUTPUT_SUBUNIT;
        } else if (strcasecmp(str_output, "TAP") == 0) {
            new_output |= CM_OUTPUT_TAP;
        } else if (strcasecmp(str_output, "XML") == 0) {
            new_output |= CM_OUTPUT_XML;
        }
    }

    libc_free(str_output_list);

    if (new_output != 0) {
        global_msg_output = new_output;
    }

    env_checked = true;

    return global_msg_output;
}

enum cm_printf_type {
    PRINTF_TEST_START,
    PRINTF_TEST_SUCCESS,
    PRINTF_TEST_FAILURE,
    PRINTF_TEST_ERROR,
    PRINTF_TEST_SKIPPED,
};

static int xml_printed;
static int file_append;

static void cmprintf_group_finish_xml(const char *group_name,
                                      size_t total_executed,
                                      size_t total_failed,
                                      size_t total_errors,
                                      size_t total_skipped,
                                      double total_runtime,
                                      struct CMUnitTestState *cm_tests)
{
    FILE *fp = stdout;
    int file_opened = 0;
    int multiple_files = 0;
    char *env;
    size_t i;

    env = getenv("CMOCKA_XML_FILE");
    if (env != NULL) {
        char buf[1024];
        int rc;

        snprintf(buf, sizeof(buf), "%s", env);

        rc = c_strreplace(buf, sizeof(buf), "%g", group_name, &multiple_files);
        if (rc < 0) {
            snprintf(buf, sizeof(buf), "%s", env);
        }

        fp = fopen(buf, "r");
        if (fp == NULL) {
            fp = fopen(buf, "w");
            if (fp != NULL) {
                file_append = 1;
                file_opened = 1;
            } else {
                fp = stderr;
            }
        } else {
            fclose(fp);
            if (file_append) {
                fp = fopen(buf, "a");
                if (fp != NULL) {
                    file_opened = 1;
                    xml_printed = 1;
                } else {
                    fp = stderr;
                }
            } else {
                fp = stderr;
            }
        }
    }

    if (!xml_printed || (file_opened && !file_append)) {
        fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        if (!file_opened) {
            xml_printed = 1;
        }
    }

    fprintf(fp, "<testsuites>\n");
    fprintf(fp, "  <testsuite name=\"%s\" time=\"%.3f\" "
                "tests=\"%u\" failures=\"%u\" errors=\"%u\" skipped=\"%u\" >\n",
                group_name,
                total_runtime, /* seconds */
                (unsigned)total_executed,
                (unsigned)total_failed,
                (unsigned)total_errors,
                (unsigned)total_skipped);

    for (i = 0; i < total_executed; i++) {
        struct CMUnitTestState *cmtest = &cm_tests[i];

        fprintf(fp, "    <testcase name=\"%s\" time=\"%.3f\" >\n",
                cmtest->test->name, cmtest->runtime);

        switch (cmtest->status) {
        case CM_TEST_ERROR:
        case CM_TEST_FAILED:
            if (cmtest->error_message != NULL) {
                fprintf(fp, "      <failure><![CDATA[%s]]></failure>\n",
                        cmtest->error_message);
            } else {
                fprintf(fp, "      <failure message=\"Unknown error\" />\n");
            }
            break;
        case CM_TEST_SKIPPED:
            fprintf(fp, "      <skipped/>\n");
            break;

        case CM_TEST_PASSED:
        case CM_TEST_NOT_STARTED:
            break;
        }

        fprintf(fp, "    </testcase>\n");
    }

    fprintf(fp, "  </testsuite>\n");
    fprintf(fp, "</testsuites>\n");

    if (file_opened) {
        fclose(fp);
    }
}

static void cmprintf_group_start_standard(const char *group_name,
                                          const size_t num_tests)
{
    print_message("[==========] %s: Running %zu test(s).\n",
                  group_name,
                  num_tests);
}

static void cmprintf_group_finish_standard(const char *group_name,
                                           size_t total_executed,
                                           size_t total_passed,
                                           size_t total_failed,
                                           size_t total_errors,
                                           size_t total_skipped,
                                           struct CMUnitTestState *cm_tests)
{
    size_t i;

    print_message("[==========] %s: %zu test(s) run.\n",
                  group_name,
                  total_executed);
    print_error("[  PASSED  ] %u test(s).\n",
                (unsigned)(total_passed));

    if (total_skipped) {
        print_error("[  SKIPPED ] %s: %zu test(s), listed below:\n",
                    group_name,
                    total_skipped);
        for (i = 0; i < total_executed; i++) {
            struct CMUnitTestState *cmtest = &cm_tests[i];

            if (cmtest->status == CM_TEST_SKIPPED) {
                print_error("[  SKIPPED ] %s\n", cmtest->test->name);
            }
        }
        print_error("\n %zu SKIPPED TEST(S)\n", total_skipped);
    }

    if (total_failed) {
        print_error("[  FAILED  ] %s: %zu test(s), listed below:\n",
                    group_name,
                    total_failed);
        for (i = 0; i < total_executed; i++) {
            struct CMUnitTestState *cmtest = &cm_tests[i];

            if (cmtest->status == CM_TEST_FAILED) {
                print_error("[  FAILED  ] %s\n", cmtest->test->name);
            }
        }
        print_error("\n %zu FAILED TEST(S)\n",
                    (total_failed + total_errors));
    }
}

static void cmprintf_standard(enum cm_printf_type type,
                              const char *test_name,
                              const char *error_message)
{
    switch (type) {
    case PRINTF_TEST_START:
        print_message("[ RUN      ] %s\n", test_name);
        break;
    case PRINTF_TEST_SUCCESS:
        print_message("[       OK ] %s\n", test_name);
        break;
    case PRINTF_TEST_FAILURE:
        if (error_message != NULL) {
            print_error("[  ERROR   ] --- %s\n", error_message);
        }
        print_message("[  FAILED  ] %s\n", test_name);
        break;
    case PRINTF_TEST_SKIPPED:
        print_message("[  SKIPPED ] %s\n", test_name);
        break;
    case PRINTF_TEST_ERROR:
        if (error_message != NULL) {
            print_error("%s\n", error_message);
        }
        print_error("[  ERROR   ] %s\n", test_name);
        break;
    }
}

static void cmprintf_group_start_tap(const size_t num_tests)
{
    static bool version_printed = false;
    if (!version_printed) {
        print_message("TAP version 13\n");
        version_printed = true;
    }

    print_message("1..%u\n", (unsigned)num_tests);
}

static void cmprintf_group_finish_tap(const char *group_name,
                                      size_t total_executed,
                                      size_t total_passed,
                                      size_t total_skipped)
{
    const char *status = "not ok";
    if (total_passed + total_skipped == total_executed) {
        status = "ok";
    }
    print_message("# %s - %s\n", status, group_name);
}

static void cmprintf_tap(enum cm_printf_type type,
                         size_t test_number,
                         const char *test_name,
                         const char *error_message)
{
    switch (type) {
    case PRINTF_TEST_START:
        break;
    case PRINTF_TEST_SUCCESS:
        print_message("ok %u - %s\n", (unsigned)test_number, test_name);
        break;
    case PRINTF_TEST_FAILURE:
        print_message("not ok %u - %s\n", (unsigned)test_number, test_name);
        if (error_message != NULL) {
            char *msg;
            char *p;

            msg = strdup(error_message);
            if (msg == NULL) {
                return;
            }
            p = msg;

            while (p[0] != '\0') {
                char *q = p;

                p = strchr(q, '\n');
                if (p != NULL) {
                    p[0] = '\0';
                }

                print_message("# %s\n", q);

                if (p == NULL) {
                    break;
                }
                p++;
            }
            libc_free(msg);
        }
        break;
    case PRINTF_TEST_SKIPPED:
        print_message("ok %u # SKIP %s\n", (unsigned)test_number, test_name);
        break;
    case PRINTF_TEST_ERROR:
        print_message("not ok %u - %s %s\n",
                      (unsigned)test_number, test_name, error_message);
        break;
    }
}

static void cmprintf_subunit(enum cm_printf_type type,
                             const char *test_name,
                             const char *error_message)
{
    switch (type) {
    case PRINTF_TEST_START:
        print_message("test: %s\n", test_name);
        break;
    case PRINTF_TEST_SUCCESS:
        print_message("success: %s\n", test_name);
        break;
    case PRINTF_TEST_FAILURE:
        print_message("failure: %s", test_name);
        if (error_message != NULL) {
            print_message(" [\n%s\n]\n", error_message);
        }
        break;
    case PRINTF_TEST_SKIPPED:
        print_message("skip: %s\n", test_name);
        break;
    case PRINTF_TEST_ERROR:
        print_message("error: %s [ %s ]\n", test_name, error_message);
        break;
    }
}

static void cmprintf_group_start(const char *group_name,
                                 const size_t num_tests)
{
    uint32_t output;

    output = cm_get_output();

    if (output & CM_OUTPUT_STANDARD) {
        cmprintf_group_start_standard(group_name, num_tests);
    }
    if (output & CM_OUTPUT_TAP) {
        cmprintf_group_start_tap(num_tests);
    }
}

static void cmprintf_group_finish(const char *group_name,
                                  size_t total_executed,
                                  size_t total_passed,
                                  size_t total_failed,
                                  size_t total_errors,
                                  size_t total_skipped,
                                  double total_runtime,
                                  struct CMUnitTestState *cm_tests)
{
    uint32_t output;

    output = cm_get_output();

    if (output & CM_OUTPUT_STANDARD) {
        cmprintf_group_finish_standard(group_name,
                                       total_executed,
                                       total_passed,
                                       total_failed,
                                       total_errors,
                                       total_skipped,
                                       cm_tests);
    }
    if (output & CM_OUTPUT_TAP) {
        cmprintf_group_finish_tap(group_name,
                                  total_executed,
                                  total_passed,
                                  total_skipped);
    }
    if (output & CM_OUTPUT_XML) {
        cmprintf_group_finish_xml(group_name,
                                  total_executed,
                                  total_failed,
                                  total_errors,
                                  total_skipped,
                                  total_runtime,
                                  cm_tests);
    }
}

static void cmprintf(enum cm_printf_type type,
                     size_t test_number,
                     const char *test_name,
                     const char *error_message)
{
    uint32_t output;

    output = cm_get_output();

    if (output & CM_OUTPUT_STANDARD) {
        cmprintf_standard(type, test_name, error_message);
    }
    if (output & CM_OUTPUT_SUBUNIT) {
        cmprintf_subunit(type, test_name, error_message);
    }
    if (output & CM_OUTPUT_TAP) {
        cmprintf_tap(type, test_number, test_name, error_message);
    }
}

void cmocka_set_message_output(uint32_t output)
{
    global_msg_output = output;
}

void cmocka_set_test_filter(const char *pattern)
{
    global_test_filter_pattern = pattern;
}

void cmocka_set_skip_filter(const char *pattern)
{
    global_skip_filter_pattern = pattern;
}

void cmocka_set_list_test(int list_test)
{
    global_list_test = list_test;
}

/****************************************************************************
 * TIME CALCULATIONS
 ****************************************************************************/

#ifdef HAVE_STRUCT_TIMESPEC
static struct timespec cm_tspecdiff(struct timespec time1,
                                    struct timespec time0)
{
    struct timespec ret;
    int xsec = 0;
    int sign = 1;

    if (time0.tv_nsec > time1.tv_nsec) {
        xsec = (int) ((time0.tv_nsec - time1.tv_nsec) / (1E9 + 1));
        time0.tv_nsec -= (long int) (1E9 * xsec);
        time0.tv_sec += xsec;
    }

    if ((time1.tv_nsec - time0.tv_nsec) > 1E9) {
        xsec = (int) ((time1.tv_nsec - time0.tv_nsec) / 1E9);
        time0.tv_nsec += (long int) (1E9 * xsec);
        time0.tv_sec -= xsec;
    }

    ret.tv_sec = time1.tv_sec - time0.tv_sec;
    ret.tv_nsec = time1.tv_nsec - time0.tv_nsec;

    if (time1.tv_sec < time0.tv_sec) {
        sign = -1;
    }

    ret.tv_sec = ret.tv_sec * sign;

    return ret;
}

static double cm_secdiff(struct timespec clock1, struct timespec clock0)
{
    double ret;
    struct timespec diff;

    diff = cm_tspecdiff(clock1, clock0);

    ret = (double) diff.tv_sec;
    ret += (double) diff.tv_nsec / (double) 1E9;

    return ret;
}
#endif /* HAVE_STRUCT_TIMESPEC */

/****************************************************************************
 * CMOCKA TEST RUNNER
 ****************************************************************************/
static int cmocka_run_one_test_or_fixture(const char *function_name,
                                          CMUnitTestFunction test_func,
                                          CMFixtureFunction setup_func,
                                          CMFixtureFunction teardown_func,
                                          void ** const volatile state,
                                          const void *const heap_check_point)
{
    const ListNode * const volatile check_point = (const ListNode*)
        (heap_check_point != NULL ?
         heap_check_point : check_point_allocated_blocks());
    int handle_exceptions = 1;
    void *current_state = NULL;
    int rc = 0;

    /* FIXME check only one test or fixture is set */

    /* Detect if we should handle exceptions */
#ifdef _WIN32
    handle_exceptions = !IsDebuggerPresent();
#endif /* _WIN32 */
#ifdef UNIT_TESTING_DEBUG
    handle_exceptions = 0;
#endif /* UNIT_TESTING_DEBUG */


    if (handle_exceptions) {
#ifndef _WIN32
        unsigned int i;
        for (i = 0; i < ARRAY_SIZE(exception_signals); i++) {
            default_signal_functions[i] = signal(
                    exception_signals[i], exception_handler);
        }
#else /* _WIN32 */
        previous_exception_filter = SetUnhandledExceptionFilter(
                exception_filter);
#endif /* !_WIN32 */
    }

    /* Init the test structure */
    initialize_testing(function_name);

    global_running_test = 1;

    if (cm_setjmp(global_run_test_env) == 0) {
        if (test_func != NULL) {
            test_func(state != NULL ? state : &current_state);

            fail_if_blocks_allocated(check_point, function_name);
            rc = 0;
        } else if (setup_func != NULL) {
            rc = setup_func(state != NULL ? state : &current_state);

            /*
             * For setup we can ignore any allocated blocks. We just need to
             * ensure they're deallocated on tear down.
             */
        } else if (teardown_func != NULL) {
            rc = teardown_func(state != NULL ? state : &current_state);

            fail_if_blocks_allocated(check_point, function_name);
        } else {
            /* ERROR */
        }
        fail_if_leftover_values(function_name);
        global_running_test = 0;
    } else {
        /* TEST FAILED */
        global_running_test = 0;
        rc = -1;
        if (global_stop_test) {
            if (has_leftover_values(function_name) == 0) {
                rc = 0;
            }
            global_stop_test = 0;
        }
    }
    teardown_testing(function_name);

    if (handle_exceptions) {
#ifndef _WIN32
        unsigned int i;
        for (i = 0; i < ARRAY_SIZE(exception_signals); i++) {
            signal(exception_signals[i], default_signal_functions[i]);
        }
#else /* _WIN32 */
        if (previous_exception_filter) {
            SetUnhandledExceptionFilter(previous_exception_filter);
            previous_exception_filter = NULL;
        }
#endif /* !_WIN32 */
    }

    return rc;
}

static int cmocka_run_group_fixture(const char *function_name,
                                    CMFixtureFunction setup_func,
                                    CMFixtureFunction teardown_func,
                                    void **state,
                                    const void *const heap_check_point)
{
    int rc;

    if (setup_func != NULL) {
        rc = cmocka_run_one_test_or_fixture(function_name,
                                        NULL,
                                        setup_func,
                                        NULL,
                                        state,
                                        heap_check_point);
    } else {
        rc = cmocka_run_one_test_or_fixture(function_name,
                                        NULL,
                                        NULL,
                                        teardown_func,
                                        state,
                                        heap_check_point);
    }

    return rc;
}

static int cmocka_run_one_tests(struct CMUnitTestState *test_state)
{
#ifdef HAVE_STRUCT_TIMESPEC
    struct timespec start = {
        .tv_sec = 0,
        .tv_nsec = 0,
    };
    struct timespec finish = {
        .tv_sec = 0,
        .tv_nsec = 0,
    };
#endif
    int rc = 0;

    /* Run setup */
    if (test_state->test->setup_func != NULL) {
        /* Setup the memory check point, it will be evaluated on teardown */
        test_state->check_point = check_point_allocated_blocks();

        rc = cmocka_run_one_test_or_fixture(test_state->test->name,
                                            NULL,
                                            test_state->test->setup_func,
                                            NULL,
                                            &test_state->state,
                                            test_state->check_point);
        if (rc != 0) {
            test_state->status = CM_TEST_ERROR;
            cmocka_print_error("Test setup failed");
        }
    }

    /* Run test */
#ifdef HAVE_STRUCT_TIMESPEC
    CMOCKA_CLOCK_GETTIME(CLOCK_REALTIME, &start);
#endif

    if (rc == 0) {
        rc = cmocka_run_one_test_or_fixture(test_state->test->name,
                                            test_state->test->test_func,
                                            NULL,
                                            NULL,
                                            &test_state->state,
                                            NULL);
        if (rc == 0) {
            test_state->status = CM_TEST_PASSED;
        } else {
            if (global_skip_test) {
                test_state->status = CM_TEST_SKIPPED;
                global_skip_test = 0; /* Do not skip the next test */
            } else {
                test_state->status = CM_TEST_FAILED;
            }
        }
        rc = 0;
    }

    test_state->runtime = 0.0;

#ifdef HAVE_STRUCT_TIMESPEC
    CMOCKA_CLOCK_GETTIME(CLOCK_REALTIME, &finish);
    test_state->runtime = cm_secdiff(finish, start);
#endif

    /* Run teardown */
    if (rc == 0 && test_state->test->teardown_func != NULL) {
        rc = cmocka_run_one_test_or_fixture(test_state->test->name,
                                            NULL,
                                            NULL,
                                            test_state->test->teardown_func,
                                            &test_state->state,
                                            test_state->check_point);
        if (rc != 0) {
            test_state->status = CM_TEST_ERROR;
            cmocka_print_error("Test teardown failed");
        }
    }

    test_state->error_message = cm_error_message;
    cm_error_message = NULL;

    return rc;
}

int _cmocka_run_group_tests(const char *group_name,
                            const struct CMUnitTest * const tests,
                            const size_t num_tests,
                            CMFixtureFunction group_setup,
                            CMFixtureFunction group_teardown)
{
    struct CMUnitTestState *cm_tests;
    const ListNode *group_check_point = check_point_allocated_blocks();
    void *group_state = NULL;
    size_t total_tests = 0;
    size_t total_failed = 0;
    size_t total_passed = 0;
    size_t total_executed = 0;
    size_t total_errors = 0;
    size_t total_skipped = 0;
    double total_runtime = 0;
    size_t i;
    int rc;

    /* Make sure uintmax_t is at least the size of a pointer. */
    assert_true(sizeof(uintmax_t) >= sizeof(void*));

    /* Display name of testcase/testsuite but not run */
    if (global_list_test) {
        print_message("%s\n", group_name);
        for (i = 0; i < num_tests; i++) {
            if (tests[i].name != NULL &&
                (tests[i].test_func != NULL
                || tests[i].setup_func != NULL
                || tests[i].teardown_func != NULL)) {
                print_message("%4s%s\n", "", tests[i].name);
            };
        }

        return 0;
    }

    cm_tests = libc_calloc(1, sizeof(struct CMUnitTestState) * num_tests);
    if (cm_tests == NULL) {
        return -1;
    }

    /* Setup cmocka test array */
    for (i = 0; i < num_tests; i++) {
        if (tests[i].name != NULL &&
            (tests[i].test_func != NULL
             || tests[i].setup_func != NULL
             || tests[i].teardown_func != NULL)) {
            if (global_test_filter_pattern != NULL) {
                int match;

                match = c_strmatch(tests[i].name, global_test_filter_pattern);
                if (!match) {
                    continue;
                }
            }
            if (global_skip_filter_pattern != NULL) {
                int match;

                match = c_strmatch(tests[i].name, global_skip_filter_pattern);
                if (match) {
                    continue;
                }
            }
            cm_tests[total_tests] = (struct CMUnitTestState) {
                .test = &tests[i],
                .status = CM_TEST_NOT_STARTED,
                .state = NULL,
            };
            total_tests++;
        }
    }

    cmprintf_group_start(group_name, total_tests);

    rc = 0;

    /* Run group setup */
    if (group_setup != NULL) {
        rc = cmocka_run_group_fixture("cmocka_group_setup",
                                      group_setup,
                                      NULL,
                                      &group_state,
                                      group_check_point);
    }

    if (rc == 0) {
        /* Execute tests */
        for (i = 0; i < total_tests; i++) {
            struct CMUnitTestState *cmtest = &cm_tests[i];
            size_t test_number = i + 1;

            cmprintf(PRINTF_TEST_START, test_number, cmtest->test->name, NULL);

            if (group_state != NULL) {
                cmtest->state = group_state;
            } else if (cmtest->test->initial_state  != NULL) {
                cmtest->state = cmtest->test->initial_state;
            }

            rc = cmocka_run_one_tests(cmtest);
            total_executed++;
            total_runtime += cmtest->runtime;
            if (rc == 0) {
                switch (cmtest->status) {
                    case CM_TEST_PASSED:
                        cmprintf(PRINTF_TEST_SUCCESS,
                                 test_number,
                                 cmtest->test->name,
                                 cmtest->error_message);
                        total_passed++;
                        break;
                    case CM_TEST_SKIPPED:
                        cmprintf(PRINTF_TEST_SKIPPED,
                                 test_number,
                                 cmtest->test->name,
                                 cmtest->error_message);
                        total_skipped++;
                        break;
                    case CM_TEST_FAILED:
                        cmprintf(PRINTF_TEST_FAILURE,
                                 test_number,
                                 cmtest->test->name,
                                 cmtest->error_message);
                        total_failed++;
                        break;
                    default:
                        cmprintf(PRINTF_TEST_ERROR,
                                 test_number,
                                 cmtest->test->name,
                                 "Internal cmocka error");
                        total_errors++;
                        break;
                }
            } else {
                char err_msg[2048] = {0};

                snprintf(err_msg, sizeof(err_msg),
                         "Could not run test: %s",
                         cmtest->error_message);

                cmprintf(PRINTF_TEST_ERROR,
                         test_number,
                         cmtest->test->name,
                         err_msg);
                total_errors++;
            }
        }
    } else {
        if (cm_error_message != NULL) {
            print_error("[  ERROR   ] --- %s\n", cm_error_message);
            vcm_free_error(cm_error_message);
            cm_error_message = NULL;
        }
        cmprintf(PRINTF_TEST_ERROR, 0,
                 group_name, "[  FAILED  ] GROUP SETUP");
        total_errors++;
    }

    /* Run group teardown */
    if (group_teardown != NULL) {
        rc = cmocka_run_group_fixture("cmocka_group_teardown",
                                      NULL,
                                      group_teardown,
                                      &group_state,
                                      group_check_point);
        if (rc != 0) {
            if (cm_error_message != NULL) {
                print_error("[  ERROR   ] --- %s\n", cm_error_message);
                vcm_free_error(cm_error_message);
                cm_error_message = NULL;
            }
            cmprintf(PRINTF_TEST_ERROR, 0,
                     group_name, "[  FAILED  ] GROUP TEARDOWN");
        }
    }

    cmprintf_group_finish(group_name,
                          total_executed,
                          total_passed,
                          total_failed,
                          total_errors,
                          total_skipped,
                          total_runtime,
                          cm_tests);

    for (i = 0; i < total_tests; i++) {
        vcm_free_error(discard_const_p(char, cm_tests[i].error_message));
    }
    libc_free(cm_tests);
    fail_if_blocks_allocated(group_check_point, "cmocka_group_tests");

    return (int)(total_failed + total_errors);
}
