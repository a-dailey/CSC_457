
#include "lwp.h"
#include <stdio.h>

void test_function(void) {
    printf("Greetings from Thread 0.  Yielding...\n");
    lwp_yield();  // Yield to itself
    printf("I'm still alive.  Goodbye\n");
}

int main() {
    lwp_create((lwpfun)test_function, NULL);
    lwp_start();
    printf("After thread termination\n");
    return 0;
}
