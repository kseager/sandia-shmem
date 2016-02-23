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
**  This is a bandwidth centric test for get: back-to-back message rate
**
**  Notice: micro benchmark ~ two nodes only
**
**  Features of Test:
**  1) bidirection bandwidth
**  2) unidirectional bandwidth
**
**  -by default megabytes/second results
**
**NOTE: this test assumes correctness of reduction algorithm
*/

#include <bw_common.h>

int main(int argc, char *argv[])
{
    perf_metrics_t metric_info;

    bw_init_two_buff_data_stream(&metric_info, argc, argv);

    bi_dir_bw_test_and_output(metric_info);

    uni_dir_bw_test_and_output(metric_info);

    bw_free_two_buff_data_stream(&metric_info);

    return 0;
}  /* end of main() */

/*even PE's get to their odd counterpart (my_node + 1), which does a get
 * back to them at the same time*/
void
bi_dir_bw(int len, perf_metrics_t *metric_info)
{
    double start = 0.0, end = 0.0;
    int dest = partner_node(metric_info->my_node);
    int i = 0, j = 0;
    static double bw = 0.0; /*must be symmetric for reduction*/

    shmem_barrier_all();

    if (metric_info->my_node % 2 == 0) {
        for (i = 0; i < metric_info->trials + metric_info->warmup; i++) {
            if(i == metric_info->warmup)
                start = shmemx_wtime();

            for(j = 0; j < metric_info->window_size; j++)
                shmem_getmem(metric_info->buf2, metric_info->buf2, len, dest);
        }
        end = shmemx_wtime();

        calc_and_print_results((end - start), bw, len, *metric_info);

    } else {
        for (i = 0; i < metric_info->trials + metric_info->warmup; i++) {
            for(j = 0; j < metric_info->window_size; j++)
                shmem_getmem(metric_info->buf, metric_info->buf, len, dest);
        }
    }
}

/*only even PE's get to my_node + 1*/
void
uni_dir_bw(int len, perf_metrics_t *metric_info)
{
    double start = 0.0, end = 0.0;
    int i = 0, j = 0;
    int dest = partner_node(metric_info->my_node);
    static double bw = 0.0; /*must be symmetric for reduction*/

    shmem_barrier_all();

    if (metric_info->my_node % 2 == 0) {
        for (i = 0; i < metric_info->trials + metric_info->warmup; i++) {
            if(i == metric_info->warmup)
                start = shmemx_wtime();

            for(j = 0; j < metric_info->window_size; j++)
                shmem_getmem(metric_info->buf, metric_info->buf, len, dest);
        }
        end = shmemx_wtime();

        calc_and_print_results((end - start), bw, len, *metric_info);
    }
}

int node_to_check(int my_node) {
    return get_node_to_check(my_node);
}
