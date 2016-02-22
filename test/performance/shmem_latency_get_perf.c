/*
 *  Copyright (c) 2015 Intel Corporation. All rights reserved.
 *  This software is available to you under the BSD license below:
 *
 * *	Redistribution and use in source and binary forms, with or
 *	without modification, are permitted provided that the following
 *	conditions are met:
 *
 *	- Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**
**  Notice: micro benchmark ~ two nodes only
**
**  Features of Test:
**  1) small put pingpong latency test
**  2) one sided latency test to calculate latency of various sizes
**    to the network stack
**
*/

#include <latency_common.h>

void round_trp_latency_get(long *target, perf_metrics_t data,
                int my_node, int npes);

void round_trp_latency_getmem(char *buf, int len, perf_metrics_t data,
                int my_node);

int main(int argc, char *argv[])
{
    int len;
    int my_node, num_pes, partner_pe;
    long *target;
    char * buf = NULL;
    perf_metrics_t data;
    data_init(&data);

    shmem_init();
    my_node = shmem_my_pe();
    num_pes = shmem_n_pes();

    only_two_PEs_check(my_node, num_pes);

    /* init data */
    partner_pe = partner_node(my_node);
    command_line_arg_check(argc, argv, &data, my_node);
    buf = aligned_buffer_alloc(data.max_len);
    init_array(buf, data.max_len, my_node);
    target = shmem_malloc(sizeof(long));
    *target = my_node;

    if(my_node == 0)
       printf("\nResults for %d trials each of length %d through %d in"\
              " increments of %d\n", data.trials, data.start_len,
              data.max_len, data.inc);


/**************************************************************/
/*                   Latency: shmem_long_g                    */
/**************************************************************/

    if (my_node == 0) {
        printf("\nshmem_long_g results:\n");
        print_results_header(data.mega);
    }

    round_trp_latency_get(target, data, my_node, num_pes);

    if((my_node == 0) && data.validate) {
        if(*target != partner_pe)
            printf("validation error shmem_long_g target = %ld != %d\n", *target,
                    partner_pe);
    }

/**************************************************************/
/*                   Latency: shmem_getmem                    */
/**************************************************************/

    if (my_node == 0) {
       printf("\nshmem_getmem results:\n");
       print_results_header(data.mega);
    }

    for (len = data.start_len; len <= data.max_len; len *= data.inc) {

        shmem_barrier_all();

        round_trp_latency_getmem(buf, len, data, my_node);

        shmem_barrier_all();
    }

    shmem_barrier_all();

    if((my_node == 0) && data.validate)
        validate_recv(buf, data.max_len, partner_pe);

    shmem_finalize();
    return 0;
}  /* end of main() */


void
round_trp_latency_get(long * target, perf_metrics_t data, int my_node,
                int npes)
{
    double start, end;
    int dest = 1, i = 0;

    shmem_barrier_all();

    if (my_node == 0) {
        for (i = 0; i < data.trials + data.warmup; i++) {
            if(i == data.warmup)
                start = shmemx_wtime();

            *target = shmem_long_g(target, dest);
        }
        end = shmemx_wtime();

        calc_and_print_results(start, end, data, sizeof(long));
   }
} /*gauge small put pathway round trip latency*/


void
round_trp_latency_getmem(char * buf, int len, perf_metrics_t data, int my_node)
{
    double start, end;
    int i = 0;

    if (my_node == 0) {

        for (i = 0; i < data.trials + data.warmup; i++) {
            if(i == data.warmup)
                start = shmemx_wtime();

            shmem_getmem(buf, buf, len, 1);
        }
        end = shmemx_wtime();

        calc_and_print_results(start, end, data, len);
    }
} /* latency/bw for one-way trip */
