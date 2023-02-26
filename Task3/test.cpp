#include "rtos-alloc.h"

#include <libgrading.h>

using namespace grading;
using namespace std;

const TestSuite tests = {
        {
                "basic allocation",
                " - allocate a pointer with rtos_malloc()\n"
                " - check that ptr is not NULL"
                ,
                []()
                {
                    void *p = rtos_malloc(8);
                    CheckNonNull(p, "allocated pointer should not be NULL");
                }
        },

        {
                "valid allocation",
                " - allocate a pointer with rtos_malloc(8)\n"
                " - check that rtos_is_valid(ptr) returns true\n"
                " - check that rtos_alloc_size(ptr) returns no less than 8"
                ,
                []()
                {
                    void *p = rtos_malloc(8);
                    Check(rtos_is_valid(p),
                          "allocated pointer should be valid");
                    Check(rtos_alloc_size(p) >= 8,
                          "allocation should be no smaller than requested");
                }
        },

        {
                "NULL is not a valid allocation",
                " - check that rtos_is_valid(NULL) returns false"
                ,
                []()
                {
                    Check(not rtos_is_valid(NULL),
                          "NULL is not a valid allocation");
                }
        },

        {
                "free()",
                " - allocate a pointer; check that it's valid\n"
                " - free the pointer\n"
                " - check that the pointer is no longer valid\n"
                ,
                []()
                {
                    void *p = rtos_malloc(16);
                    Check(rtos_is_valid(p),
                          "allocated pointer should be valid");

                    rtos_free(p);
                    Check(not rtos_is_valid(p),
                          "pointer should not be valid after freeing it");
                }
        },

        {
                "free(NULL)",
                " - free the NULL pointer (should cause no action)\n"
                " - check that NULL remains invalid\n"
                ,
                []()
                {
                    rtos_free(NULL);
                    Check(not rtos_is_valid(NULL),
                          "NULL is not a valid allocation");
                }
        },

        {
                "multiple allocations",
                " - allocate several objects of different sizes\n"
                " - check that each allocation is no smaller than requested\n"
                " - check that the total allocation size is the sum of parts\n"
                ,
                []()
                {
                    size_t total = rtos_total_allocated();
                    std::vector<void*> pointers;

                    for (size_t s : { 1, 2, 17, 42, 1049, 50 })
                    {
                        void *p = rtos_malloc(s);
                        size_t size = rtos_alloc_size(p);
                        Check(size >= s,
                              "allocation size must be >= requested")
                                << "requested " << s
                                << ", got " << size;

                        pointers.push_back(p);
                        total += size;
                    }

                    CheckInt(total, rtos_total_allocated())
                            << "invalid total allocation size";

                    for (void *p : pointers)
                    {
                        rtos_free(p);
                    }
                }
        },
};

int main(int argc, char *argv[])
{
    const TestSuite::Statistics stats = tests.Run(argc, argv);
    std::cout << "Score: " << (2 + 5 * stats.score) << "/7" << std::endl;

    return 0;
}