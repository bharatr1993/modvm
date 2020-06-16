#include <stdio.h>
#include <stdlib.h>

#include "test.h"


static struct TestNode
{
    char const * name;

    int depth;
    int total_count;
    int passed_count;

    struct TestNode * next;
    struct TestNode * prev;
} *test_node;


void __test_begin(char const * name)
{
    struct TestNode * node = (struct TestNode *) calloc(1, sizeof (struct TestNode));
    node->name = name;

    if (test_node)
    {
        test_node->next = node;
        node->prev = test_node;
        node->depth = test_node->depth + 1;
    }

    test_node = node;
    printf("%-*stest: \x1b[37;1m%s\x1b[0m\n", test_node->depth * 4, "", name);
}

void __test_end()
{
    printf("%-*send : \x1b[37;1m%s\x1b[0m -- \x1b[3%cm%d / %d\x1b[0m tests passed\n",
        (test_node->depth) * 4, "",
        test_node->name,
        '1' + (test_node->passed_count == test_node->total_count),
        test_node->passed_count,
        test_node->total_count 
    );
    struct TestNode * tmp = test_node;
    test_node = test_node->prev;
    if (test_node)
    {
        test_node->total_count += test_node->next->total_count;
        test_node->passed_count += test_node->next->passed_count;
    }
    free(tmp);
}

void __test_expect(int line, char const * condition_repr, char const * message, int result)
{
    test_node->total_count += 1;
    test_node->passed_count += result ? 1 : 0;
    printf("%-*s[%s]: \x1b[33m%s\x1b[0m\n",
        (test_node->depth + 1) * 4, "",
        result ? "\x1b[32mPASS\x1b[0m" : "\x1b[31mFAIL\x1b[0m",
        condition_repr);
    if (message) printf("%-*s%s\n", (test_node->depth + 2) * 4, "", message);
}

void __test_info_man()
{
    printf("%-*s[\x1b[36mINFO\x1b[0m]: ", (test_node->depth + 1) * 4, "");
}
