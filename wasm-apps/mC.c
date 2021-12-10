#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include <unistd.h>

__attribute__((import_module("mA")))
__attribute__((import_name("A"))) extern int A();

__attribute__((import_module("mB")))
__attribute__((import_name("B"))) extern int B();

int intToStr(int x, char* str, int str_len, int digit);
int get_pow(int x, int y);
int32_t calculate_native(int32_t n, int32_t func1, int32_t func2);

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
	float ret = 0.0f;
	printf ("calling into WASM function: %s : interations %d seed1: %lf seed2: %f\n", __FUNCTION__, iteration, seed1, seed2);

	for (int i=0; i<iteration; i++){
		ret += 1.0f/seed1 + seed2;
	}

	printf("ret = %f\n",ret);
	//sync();

	return ret;
}


int32_t mul7(int32_t n) {
    printf ("calling into WASM function: %s, %d", __FUNCTION__, n);
    n = n * 7;
    printf ("    %s return %d \n", __FUNCTION__, n);
    return n;
}

int32_t mul5(int32_t n) {
    printf ("calling into WASM function: %s, %d", __FUNCTION__, n);
    n = n * 5;
    printf ("    %s return %d \n", __FUNCTION__, n);
    return n;
}

int32_t calculate(int32_t n) {
    printf ("calling into WASM function: %s: %d\n", __FUNCTION__, n);
    int32_t (*f1)(int32_t) = &mul5;
    int32_t (*f2)(int32_t) = &mul7;
    int32_t ret =  calculate_native(n, (uint32_t)f1, (uint32_t)f2);
	printf("\nret = %d\n",ret);
	return ret;
}
