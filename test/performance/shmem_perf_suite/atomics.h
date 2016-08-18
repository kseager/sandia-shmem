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
*/


#define NUM_INC 100

/*  must override putmems size assumption: only iterate over single message size */
static inline void bw_set_metric_info_len(perf_metrics_t *metric_info, unsigned int size)
{
    metric_info->start_len = size;
    metric_info->max_len = size;
    metric_info->size_inc = NUM_INC;
    uni_bw(size, metric_info, streaming_node(*metric_info));
}

#define SIZE 3

/* iterate over atomic specific types and insert the atomic c11 call */
void uni_dir_bw(int len, perf_metrics_t *metric_info)
{
    int i = 0;
    unsigned int atomic_sizes[SIZE] = {sizeof(int), sizeof(long),
                                        sizeof(long long)};
#define TYPE add
if(metric_info->my_node == 0)
    printf("\n add \n");

#define shmem_putmem(dest, source, nelems, pe) \
            shmem_##TYPE##(dest, *source, pe)

    for(i = 0; i < SIZE; i++)
        bw_set_metric_info_len(metric_info, atomic_sizes[i]);

#undef TYPE

#define TYPE inc
if(metric_info->my_node == 0)
    printf("\n inc \n");

#undef shmem_putmem

#define shmem_putmem(dest, source, nelems, pe) \
            shmem_##TYPE##(dest, pe)

    for(i = 0; i < SIZE; i++)
        bw_set_metric_info_len(metric_info, atomic_sizes[i]);

/*NO-OP for fetch*/
#define shmem_quiet();

#undef TYPE

#define TYPE fadd
if(metric_info->my_node == 0)
    printf("\n fadd \n");

#undef shmem_putmem

#define shmem_putmem(dest, source, nelems, pe) \
            shmem_##TYPE##(dest, *source, pe)

    for(i = 0; i < SIZE; i++)
        bw_set_metric_info_len(metric_info, atomic_sizes[i]);

#undef TYPE

#define TYPE finc
if(metric_info->my_node == 0)
    printf("\n finc \n");

#undef shmem_putmem

#define shmem_putmem(dest, source, nelems, pe) \
            shmem_##TYPE##(dest, pe)

    for(i = 0; i < SIZE; i++)
        bw_set_metric_info_len(metric_info, atomic_sizes[i]);

}
