#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <shmem.h>
#include <shmemx.h>
#include <string.h>
#include <assert.h>

char * aligned_buffer_alloc(int len) {
    unsigned long page_align;
    char *buf;

    page_align = getpagesize();
    buf = shmem_malloc(len + page_align);
    buf = (char *) (((unsigned long) buf + (page_align - 1)) /
            page_align * page_align);

    return buf;
}

int static inline is_divisible_by_4(int num)
{
    assert(sizeof(int) == 4);
    return (!(num & 0x00000003));
}

/*to be a power of 2 must only have 1 set bit*/
int static inline is_pow_of_2(unsigned int num)
{
    int i = 0;
    unsigned int count = 0;

    do {
        if(num & 1)
            count++;
        if(count > 1)
            return 0;
        num >>= 1;
    } while(num);

    return 1;
}

void init_array(char * const buf, int len, int my_pe_num)
{
    int i = 0;
    int array_size = len / sizeof(int);
    int * ibuf = (int *)buf;

    assert(is_divisible_by_4(len));

    for(i = 0; i < array_size; i++)
        ibuf[i] = my_pe_num;

}

void static inline validate_recv(char * buf, int len, int partner_pe)
{
    int i = 0;
    int array_size = len / sizeof(int);
    int * ibuf = (int *)buf;

    assert(is_divisible_by_4(len));

    for(i = 0; i < array_size; i++) {
        if(ibuf[i] != partner_pe)
            printf("validation error at index %d: %d != %d \n", i, ibuf[i],
                    partner_pe);
    }
}

int static inline partner_node(int my_node)
{
    return ((my_node % 2 == 0) ? (my_node + 1) : (my_node - 1));
}
