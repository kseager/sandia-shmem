
#include <common.h>

#define MAX_MSG_SIZE (1<<23)
#define START_LEN 1

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
    int validate;
    int my_node, num_pes;
    bw_units unit;
    char *buf, *buf2;
} perf_metrics_t;

long red_psync[_SHMEM_REDUCE_SYNC_SIZE];

/*default settings if no input is provided */
void data_init(perf_metrics_t * data) {
    data->start_len = START_LEN;
    data->max_len = MAX_MSG_SIZE;
    data->size_inc = INC;
    data->trials = TRIALS;
    data->window_size = WINDOW_SIZE; /*back-to-back msg stream*/
    data->warmup = WARMUP; /*number of initial iterations to skip*/
    data->unit = MB;
    data->validate = FALSE;
    data->my_node = shmem_my_pe();
    data->num_pes = shmem_n_pes();
    data->buf = NULL;
    data->buf2 = NULL;
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

void print_results_header(output_types type, perf_metrics_t metric_info) {
    if(type == BI_DIR) {
        printf("\nResults for %d PEs %d trials with window size %d "\
            "max message size %d with multiple of %d increments\n",
            metric_info.num_pes, metric_info.trials, metric_info.window_size,
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
                            perf_metrics_t *metric_info) {
    int ch, error = FALSE;
    extern char *optarg;

    /* check command line args */
    while ((ch = getopt(argc, argv, "i:e:s:n:kbv")) != EOF) {
        switch (ch) {
        case 'i':
            metric_info->size_inc = strtol(optarg, (char **)NULL, 0);
            break;
        case 'e':
            metric_info->max_len = strtol(optarg, (char **)NULL, 0);
            if(!is_divisible_by_4(metric_info->max_len)) error = TRUE;
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
        case 'v':
            metric_info->validate = TRUE;
            break;
        default:
            error = TRUE;
            break;
        }
    }

    if (error) {
        if (metric_info->my_node == 0) {
            fprintf(stderr, "Usage: [-s start_length] [-e end_length] "\
                    ": lengths should be divisible by four \n " \
                    "[-i inc] [-n trials (must be greater than 20)] "\
                    "[-k (kilobytes/second)] [-b (bytes/second)] "\
                    "[-v (validate data stream)] \n");
        }
        shmem_finalize();
        exit (-1);
    }
}

void static inline only_even_PEs_check(int my_node, int num_pes) {
    if (num_pes % 2 != 0) {
        if (my_node == 0) {
            fprintf(stderr, "can only use an even number of nodes\n");
        }
        shmem_finalize();
        exit(77);
    }
}

void static inline bw_init_two_buff_data_stream(perf_metrics_t *metric_info,
                                    int argc, char *argv[]) {
    int i = 0;

    /*must be before data_init*/
    shmem_init();

    data_init(metric_info);

    only_even_PEs_check(metric_info->my_node, metric_info->num_pes);

    for(i = 0; i < _SHMEM_REDUCE_MIN_WRKDATA_SIZE; i++)
        red_psync[i] = _SHMEM_SYNC_VALUE;

    command_line_arg_check(argc, argv, metric_info);

    metric_info->buf = aligned_buffer_alloc(metric_info->max_len);
    init_array(metric_info->buf, metric_info->max_len, metric_info->my_node);
    metric_info->buf2 = aligned_buffer_alloc(metric_info->max_len);
    init_array(metric_info->buf2, metric_info->max_len, metric_info->my_node);

}

void static inline bw_free_two_buff_data_stream(perf_metrics_t *metric_info) {
    shmem_barrier_all();

    shmem_free(metric_info->buf);
    shmem_free(metric_info->buf2);
    shmem_finalize();
}

/**************************************************************/
/*                   Bi-Directional BW                        */
/**************************************************************/

extern void bi_dir_bw(int len, perf_metrics_t metric_info);

void static inline bi_dir_bw_test_and_output(perf_metrics_t metric_info) {


    int len = 0, partner_pe = partner_node(metric_info.my_node);

    if (metric_info.my_node == 0)
        print_results_header(BI_DIR, metric_info);

    for (len = metric_info.start_len; len <= metric_info.max_len;
        len *= metric_info.size_inc) {

        bi_dir_bw(len, metric_info);
    }

    if(metric_info.validate) {
        if(metric_info.my_node % 2 == 0)
            validate_recv(metric_info.buf2, metric_info.max_len, partner_pe);
        else
            validate_recv(metric_info.buf, metric_info.max_len, partner_pe);
    }
}

/**************************************************************/
/*                   UNI-Directional BW                       */
/**************************************************************/

extern void uni_dir_bw(int len, perf_metrics_t metric_info);

void static inline uni_dir_bw_test_and_output(perf_metrics_t metric_info) {
    int len = 0, partner_pe = partner_node(metric_info.my_node);

    if (metric_info.my_node == 0)
        print_results_header(UNI_DIR, metric_info);

    /*reset array*/
    init_array(metric_info.buf, metric_info.max_len, metric_info.my_node);

    for (len = metric_info.start_len; len <= metric_info.max_len;
        len *= metric_info.size_inc) {

        uni_dir_bw(len, metric_info);
    }

    if((metric_info.my_node % 2 != 0) && metric_info.validate)
        validate_recv(metric_info.buf, metric_info.max_len, partner_pe);

}


/* reduction to collect performance results from even PE set
            then start_pe will print results    */
void static inline calc_and_print_results(double total_t, double bw,
                        int len, perf_metrics_t metric_info)
{
    int num_even_PEs = metric_info.num_pes/2, start_pe = 0;
    int stride_only_even_pes = 1;
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

    /* base case: will be overwritten by collective if num_pes > 2 */
    pe_bw_sum = bw;

    if(metric_info.num_pes >= 2)
        shmem_double_sum_to_all(&pe_bw_sum, &bw, nred_elements, start_pe,
                                stride_only_even_pes, num_even_PEs, pwrk, red_psync);

    if(metric_info.my_node == start_pe) {
        pe_bw_avg = pe_bw_sum / metric_info.num_pes;
        pe_mr_avg = pe_bw_avg / (len / 1.048576e6);
        print_data_results(pe_bw_avg, pe_mr_avg, metric_info, len);
    }
}
