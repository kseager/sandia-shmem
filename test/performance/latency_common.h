
#include <common.h>

#define TRUE (1)
#define FALSE (0)
#define INIT_VALUE 1

#define MAX_MSG_SIZE (1<<23)
#define START_LEN 1

#define INC 2
#define TRIALS 100
#define WARMUP 10

typedef struct perf_metrics {
   double latency;
   int start_len, max_len;
   int inc, trials;
   int mega, warmup;
   int validate;
} perf_metrics_t;

void data_init(perf_metrics_t * data) {
   data->latency = 0.0;
   data->start_len = START_LEN;
   data->max_len = MAX_MSG_SIZE;
   data->inc = INC;
   data->trials = TRIALS;
   data->mega = TRUE;
   data->warmup = WARMUP; /*number of initial iterations to skip*/
   data->validate = FALSE;
}

void static inline print_results_header(int mega) {
   printf("\nLength                  Latency                       \n");
   printf("in bytes            in micro seconds              \n");
}

void static inline calc_and_print_results(double start, double end,
                                        perf_metrics_t data, int len) {
    data.latency = (end - start) / data.trials;

    printf("%9d           %8.2f             \n",
          len, data.latency * 1000000.0);
}

void static inline command_line_arg_check(int argc, char *argv[],
                            perf_metrics_t *metric_info, int my_node) {
    int ch, error = FALSE;
    extern char *optarg;

    /* check command line args */
    while ((ch = getopt(argc, argv, "i:e:s:n:mv")) != EOF) {
        switch (ch) {
        case 'i':
            metric_info->inc = strtol(optarg, (char **)NULL, 0);
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
        case 'm':
            metric_info->mega = FALSE;
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
        if (my_node == 0) {
            fprintf(stderr, "Usage: %s [-s start_length] [-e end_length] "\
                    ": lengths must be a power of two \n " \
                    "[-i inc] [-n trials (must be greater than 20)] "\
                    "[-m (millions)] [-v (validate results)]\n", argv[0]);
        }
        shmem_finalize();
        exit (-1);
    }
}

void static inline only_two_PEs_check(int my_node, int num_pes) {
    if (num_pes != 2) {
        if (my_node == 0) {
            fprintf(stderr, "2-nodes only test\n");
        }
        shmem_finalize();
        exit(77);
    }
}
