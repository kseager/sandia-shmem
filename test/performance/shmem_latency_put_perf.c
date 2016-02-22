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

void pingpong_p(long *pingpong_ball, perf_metrics_t data,
                int my_node, int npes);
void ping_put(char *buf, int len, perf_metrics_t data, int my_node);



int main(int argc, char *argv[])
{
    int len;
    int my_node, num_pes;
    long *pingpong_ball;
    char * buf = NULL;
    perf_metrics_t data;
    data_init(&data);

    shmem_init();
    my_node = shmem_my_pe();
    num_pes = shmem_n_pes();

    only_two_PEs_check(my_node, num_pes);

    /* init data */
    command_line_arg_check(argc, argv, &data, my_node);
    buf = aligned_buffer_alloc(data.max_len);
    init_array(buf, data.max_len, my_node);
    pingpong_ball = shmem_malloc(sizeof(long));
    *pingpong_ball = INIT_VALUE;

    if(my_node == 0)
       printf("\nResults for %d trials each of length %d through %d in"\
              " increments of %d\n", data.trials, data.start_len,
              data.max_len, data.inc);


/**************************************************************/
/*                   PINGPONG with small put                  */
/**************************************************************/

    if (my_node == 0) {
        printf("\nPing-Pong shmem_long_p results:\n");
        print_results_header(data.mega);
    }

    pingpong_p(pingpong_ball, data, my_node, num_pes);

/**************************************************************/
/*                   PING over varying message sizes          */
/**************************************************************/

    if (my_node == 0) {
       printf("\nPing shmem_putmem results:\n");
       print_results_header(data.mega);
    }

    for (len = data.start_len; len <= data.max_len; len *= data.inc) {

        shmem_barrier_all();

        ping_put(buf, len, data, my_node);

        shmem_barrier_all();
    }

    shmem_barrier_all();

    if((my_node % 2 != 0) && data.validate)
        validate_recv(buf, data.max_len, partner_node(my_node));

    shmem_finalize();
    return 0;
}  /* end of main() */


void
pingpong_p(long * pingpong_ball, perf_metrics_t data, int my_node, int npes)
{
    double start, end;
    long tmp;
    int dest = (my_node + 1) % npes, i = 0;
    tmp = *pingpong_ball = INIT_VALUE;

    shmem_barrier_all();

    if (my_node == 0) {
        for (i = 0; i < data.trials + data.warmup; i++) {
            if(i == data.warmup)
                start = shmemx_wtime();

            shmem_long_p(pingpong_ball, ++tmp, dest);

            shmem_long_wait_until(pingpong_ball, SHMEM_CMP_EQ, tmp);
        }
        end = shmemx_wtime();

        calc_and_print_results(start, end, data, sizeof(long));

   } else {
        for (i = 0; i < data.trials + data.warmup; i++) {
            shmem_long_wait_until(pingpong_ball, SHMEM_CMP_EQ, ++tmp);

            shmem_long_p(pingpong_ball, tmp, dest);
        }
   }

} /*gauge small put pathway round trip latency*/


void
ping_put(char * buf, int len, perf_metrics_t data, int my_node)
{
    double start, end;
    int i = 0;

    if (my_node == 0) {

        for (i = 0; i < data.trials + data.warmup; i++) {
            if(i == data.warmup)
                start = shmemx_wtime();

            shmem_putmem(buf, buf, len, 1 );
            shmem_quiet();

        }
        end = shmemx_wtime();

        calc_and_print_results(start, end, data, len);
    }
} /* latency/bw for one-way trip */
