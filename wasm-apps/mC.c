#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

__attribute__((import_module("mA")))
__attribute__((import_name("A"))) extern int A();

__attribute__((import_module("mB")))
__attribute__((import_name("B"))) extern int B();

int C() {
    return 12;
}

int call_A() {
    return A();
}

int call_B() {
    return B();
}

float generate_float(int iteration, double seed1, float seed2) {
    float ret;
    printf ("calling into WASM function: %s\n", __FUNCTION__);

    for (int i=0; i<iteration; i++){
        ret += 1.0f/seed1 + seed2;
    }

    return ret;
}


/*
int32_t mul7(int32_t n)
{
    printf ("calling into WASM function: %s,", __FUNCTION__);
    n = n * 7;
    printf ("    %s return %d \n", __FUNCTION__, n);
    return n;
}

int32_t mul5(int32_t n)
{
    printf ("calling into WASM function: %s,", __FUNCTION__);
    n = n * 5;
    printf ("    %s return %d \n", __FUNCTION__, n);
    return n;
}

int32_t calculate(int32_t n)
{
    printf ("calling into WASM function: %s\n", __FUNCTION__);
    int32_t (*f1)(int32_t) = &mul5;
    int32_t (*f2)(int32_t) = &mul7;
    return calculate_native(n, (uint32_t)f1, (uint32_t)f2);
}
*/
