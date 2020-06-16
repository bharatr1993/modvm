#pragma once

#include <stdio.h>


void __test_begin(char const * name);
void __test_end();
void __test_expect(int line, char const * condition_repr, char const * message, int result);
void __test_info_man();


#define _AOD_0() ((char const*) 0)
#define _AOD_1(A) A
#define _AOD_X(x, A, B, FUNC, ...)  FUNC  
#define _AOD(...) _AOD_X(,##__VA_ARGS__, _AOD_2(__VA_ARGS__), _AOD_1(__VA_ARGS__), _AOD_0(__VA_ARGS__))


#define TEST(name) __test_begin(name);
#define END_TEST __test_end();
#define EXPECT(condition, ...) __test_expect(__LINE__, #condition, _AOD(__VA_ARGS__), condition);
#define PASS(...) EXPECT(1, __VA_ARGS__)
#define FAIL(...) EXPECT(0, __VA_ARGS__)
#define INFO_MANUAL(code) __test_info_man(); code; printf("%c", '\n');
