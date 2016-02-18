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
**  This is a bandwidth centric test for put: back-to-back message rate
**
**  Notice: micro benchmark ~ two nodes only
**
**  Features of Test:
**  1) bidirection bandwidth
**  2) unidirectional bandwidth
**
**  -by default megabytes/second results
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <shmem.h>
#include <shmemx.h>
#include <string.h>

#define MAX_MSG_SIZE (1<<23)
#define START_LEN 1

#define CEIL(num) (((num) % 2 == 0) ? (num) : ((num) + 1))

#define INC 2
#define TRIALS 100
#define WINDOW_SIZE 64
#define WARMUP 10

#define TRIALS_LARGE  20
#define WINDOW_SIZE_LARGE 64
#define WARMUP_LARGE  2
#define LARGE_MESSAGE_SIZE  8192

#define TRUE  (1)
#define FALSE (0)

typedef enum {
    BI_DIR,
    UNI_DIR
} output_types;

typedef enum {
    B,
    KB,
    MB
} bw_units;

typedef struct perf_metrics {
    int start_len, max_len;
    int size_inc, trials;
    int window_size, warmup;
    bw_units unit;
} perf_metrics_t;

long red_psync[_SHMEM_REDUCE_SYNC_SIZE];

void bi_dir_bw(char *buf, char *buf2, int len, perf_metrics_t metric_info,
            int my_node, int npes);

void uni_dir_bw(char * buf, int len, perf_metrics_t metric_info,
                int my_node, int npes);

void data_init(perf_metrics_t * data) {
    data->start_len = START_LEN;
    data->max_len = MAX_MSG_SIZE;
    data->size_inc = INC;
    data->trials = TRIALS;
    data->window_size = WINDOW_SIZE; /*back-to-back msg stream*/
    data->warmup = WARMUP; /*number of initial iterations to skip*/
    data->unit = MB;
}

void print_data_results(double bw, double mr, perf_metrics_t data, int len) {
    printf("%9d       ", len);

    if(data.unit == KB) {
        bw = bw * 1.024e3;
    } else if(data.unit == B){
        bw = bw * 1.048576e6;
    }

    printf("%10.2f                          %10.2f\n", bw, mr);
}

void print_results_header(output_types type, perf_metrics_t metric_info,
                        int num_pes) {
    if(type == BI_DIR) {
        printf("\nResults for %d PEs %d trials with window size %d "\
            "max message size %d with multiple of %d increments\n",
            num_pes, metric_info.trials, metric_info.window_size,
            metric_info.max_len, metric_info.size_inc);

        printf("\nLength           Bi-directional Bandwidth           "\
            "Message Rate\n");
    } else {
        printf("\nLength           Uni-directional Bandwidth           "\
            "Message Rate\n");
    }

    printf("in bytes         ");
    if (metric_info.unit == MB) {
        printf("in megabytes per second");
    } else if (metric_info.unit == KB) {
        printf("in kilobytes per second");
    } else {
        printf("in bytes per second");
    }

    printf("         in messages/seconds\n");
}

void command_line_arg_check(int argc, char *argv[],
                            perf_metrics_t *metric_info, int my_node) {
    int ch, error = FALSE;
    extern char *optarg;

    /* check command line args */
    while ((ch = getopt(argc, argv, "i:e:s:n:kb")) != EOF) {
        switch (ch) {
        case 'i':
            metric_info->size_inc = strtol(optarg, (char **)NULL, 0);
            break;
        case 'e':
            metric_info->max_len = strtol(optarg, (char **)NULL, 0);
            break;
        case 's':
            metric_info->start_len = strtol(optarg, (char **)NULL, 0);
            if ( metric_info->start_len < 1 ) metric_info->start_len = 1;
            break;
        case 'n':
            metric_info->trials = strtol(optarg, (char **)NULL, 0);
            if(metric_info->trials <= (metric_info->warmup*2)) error = TRUE;
            break;
        case 'k':
            metric_info->unit = KB;
            break;
        case 'b':
            metric_info->unit = B;
            break;
        default:
            error = TRUE;
            break;
        }
    }

    if (error) {
        if (my_node == 0) {
            fprintf(stderr, "Usage: [-s start_length] [-e end_length] "\
                    ": lengths should be a power of two \n " \
                    "[-i inc] [-n trials (must be greater than 20)] "\
                    "[-k (kilobytes/second)] [-b (bytes/second)] \n");
        }
        shmem_finalize();
        exit (-1);
    }
}

char * buffer_alloc_and_init(int len) {
    unsigned long page_align;
    char *buf;
    int i;

    page_align = getpagesize();
    buf = shmem_malloc(len + page_align);
    buf = (char *) (((unsigned long) buf + (page_align - 1)) /
            page_align * page_align);

    for(i = 0; i < len; i++)
        buf[i] = 'z';

    return buf;
}


int main(int argc, char *argv[])
{
    char *buf, *buf2;
    int len = 0, my_node, num_pes, i = 0;
    perf_metrics_t metric_info;

    shmem_init();
    my_node = shmem_my_pe();
    num_pes = shmem_n_pes();

    /* initialize all data */
    data_init(&metric_info);

    for(i = 0; i < _SHMEM_REDUCE_MIN_WRKDATA_SIZE; i++)
        red_psync[i] = _SHMEM_SYNC_VALUE;

    command_line_arg_check(argc, argv, &metric_info, my_node);

    buf  = buffer_alloc_and_init(metric_info.max_len);
    buf2 = buffer_alloc_and_init(metric_info.max_len);

/**************************************************************/
/*                   Bi-Directional BW                        */
/**************************************************************/

    if (my_node == 0)
        print_results_header(BI_DIR, metric_info, num_pes);

    for (len = metric_info.start_len; len <= metric_info.max_len;
        len *= metric_info.size_inc) {

        bi_dir_bw(buf, buf2, len, metric_info, my_node, num_pes);
    }

/**************************************************************/
/*                   UNI-Directional BW                       */
/**************************************************************/

    if (my_node == 0)
        print_results_header(UNI_DIR, metric_info, num_pes);

    for (len = metric_info.start_len; len <= metric_info.max_len;
        len *= metric_info.size_inc) {

        uni_dir_bw(buf, len, metric_info, my_node, num_pes);
    }

    shmem_barrier_all();

    shmem_free(buf);
    shmem_free(buf2);
    shmem_finalize();
    return 0;
}  /* end of main() */

/* reduction to collect performance results from even PE set
            then start_pe will print results    */
void static inline calc_and_print_results(double total_t, double bw,
                        int len, perf_metrics_t metric_info, int my_node,
                        int npes)
{
    int num_even_PEs = CEIL(npes)/2, start_pe = 0, stride_only_even_pes = 1;
    static double pe_bw_sum;
    double pe_bw_avg = 0.0, pe_mr_avg = 0.0;
    int nred_elements = 1;
    static double pwrk[_SHMEM_REDUCE_MIN_WRKDATA_SIZE];

    if (total_t > 0 ) {
        bw = (len / 1.048576e6 * metric_info.window_size * metric_info.trials) /
                (total_t);
    } else {
        bw = 0.0;
    }

    /* base case: will be overwritten by collective if npes > 2 */
    pe_bw_sum = bw;

    if(npes >= 2)
        shmem_double_sum_to_all(&pe_bw_sum, &bw, nred_elements, start_pe,
                                stride_only_even_pes, num_even_PEs, pwrk, red_psync);

    if(my_node == start_pe) {
        pe_bw_avg = pe_bw_sum / npes;
        pe_mr_avg = pe_bw_avg / (len / 1.048576e6);
        print_data_results(pe_bw_avg, pe_mr_avg, metric_info, len);
    }


}

void
bi_dir_bw(char *buf, char *buf2, int len, perf_metrics_t metric_info, int my_node,
          int npes)
{
    double start = 0.0, end = 0.0;
    int dest = (my_node + 1) % npes;
    int i = 0, j = 0;
    static double bw = 0.0; /*must be symmetric for reduction*/

    shmem_barrier_all();

    if (my_node % 2 == 0) {
        for (i = 0; i < metric_info.trials + metric_info.warmup; i++) {
            if(i == metric_info.warmup)
                start = shmemx_wtime();

            for(j = 0; j < metric_info.window_size; j++)
                shmem_putmem(buf, buf, len, dest);

            shmem_quiet();
        }
        end = shmemx_wtime();

        calc_and_print_results((end - start), bw, len, metric_info, my_node,
                            npes);

    } else {
        for (i = 0; i < metric_info.trials + metric_info.warmup; i++) {
            for(j = 0; j < metric_info.window_size; j++)
                shmem_putmem(buf2, buf2, len, dest);

            shmem_quiet();
        }
    }

}


void
uni_dir_bw(char * buf, int len, perf_metrics_t metric_info, int my_node,
         int npes)
{
    double start = 0.0, end = 0.0;
    int i = 0, j = 0;
    int dest = (my_node + 1) % npes;
    static double bw = 0.0; /*must be symmetric for reduction*/

    shmem_barrier_all();

    if (my_node % 2 == 0) {
        for (i = 0; i < metric_info.trials + metric_info.warmup; i++) {
            if(i == metric_info.warmup)
                start = shmemx_wtime();

            for(j = 0; j < metric_info.window_size; j++)
                shmem_putmem(buf, buf, len, dest);

            shmem_quiet();

        }
        end = shmemx_wtime();

        calc_and_print_results((end - start), bw, len, metric_info, my_node,
                            npes);
    }
}
