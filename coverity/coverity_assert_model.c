#define LargestIntegralType unsigned long long


void _assert_true(const LargestIntegralType result,
                  const char* const expression,
                  const char * const file, const int line)
{
    if (!result) {
        __coverity_panic__();
    }
}

void _assert_int_equal(
    const LargestIntegralType a, const LargestIntegralType b,
    const char * const file, const int line)
{
    if (a != b) {
        __coverity_panic__();
    }
}

void _assert_int_not_equal(
    const LargestIntegralType a, const LargestIntegralType b,
    const char * const file, const int line)
{
    if (a == b) {
        __coverity_panic__();
    }
}

void _assert_return_code(const LargestIntegralType result,
                         size_t rlen,
                         const LargestIntegralType error,
                         const char * const expression,
                         const char * const file,
                         const int line)
{
    if (result != 0) {
        __coverity_panic__();
    }
}

void _assert_string_equal(const char * const a, const char * const b,
                          const char * const file, const int line)
{
    int cmp;

    cmp = __coverity_strcmp(a, b);
    if (cmp != 0) {
        __coverity_panic__();
    }
}

void _assert_string_not_equal(const char * const a, const char * const b,
                              const char *file, const int line)
{
    int cmp;

    cmp = __coverity_strcmp(a, b);
    if (cmp == 0) {
        __coverity_panic__();
    }
}

void _assert_memory_equal(const void * const a, const void * const b,
                          const size_t size, const char* const file,
                          const int line)
{
    int cmp;

    cmp = memcmp(a, b, size);
    if (cmp != 0) {
        __coverity_panic__();
    }
}

void _assert_memory_not_equal(const void * const a, const void * const b,
                              const size_t size, const char* const file,
                              const int line)
{
    int cmp;

    cmp = memcmp(a, b, size);
    if (cmp == 0) {
        __coverity_panic__();
    }
}

void _assert_in_range(
    const LargestIntegralType value, const LargestIntegralType minimum,
    const LargestIntegralType maximum, const char* const file, const int line)
{
    if (value < minimum || value > maximum) {
        __coverity_panic__();
    }
}

void _assert_not_in_range(
    const LargestIntegralType value, const LargestIntegralType minimum,
    const LargestIntegralType maximum, const char* const file, const int line)
{
    if (value > minimum && value < maximum) {
        __coverity_panic__();
    }
}

void _assert_in_set(
    const LargestIntegralType value, const LargestIntegralType values[],
    const size_t number_of_values, const char* const file, const int line)
{
    int i;

    if (i = 0; i < number_of_values; i++) {
        if (value == values[i]) {
            return;
        }
    }
    __coverity_panic__();
}

void _assert_not_in_set(
    const LargestIntegralType value, const LargestIntegralType values[],
    const size_t number_of_values, const char* const file, const int line)
{
    int i;

    if (i = 0; i < number_of_values; i++) {
        if (value == values[i]) {
            __coverity_panic__();
        }
    }
}
