
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
   int start_len, max_len;
   int inc, trials;
   int warmup;
   int validate;
   int my_node, npes;
   long * target;
   char * buf;
} perf_metrics_t;

void data_init(perf_metrics_t * data) {
   data->start_len = START_LEN;
   data->max_len = MAX_MSG_SIZE;
   data->inc = INC;
   data->trials = TRIALS;
   data->warmup = WARMUP; /*number of initial iterations to skip*/
   data->validate = FALSE;
   data->my_node = shmem_my_pe();
   data->npes = shmem_n_pes();
   data->target = NULL;
   data->buf = NULL;
}

void static inline print_results_header() {
   printf("\nLength                  Latency                       \n");
   printf("in bytes            in micro seconds              \n");
}

/*not storing results, only outputing it*/
void static inline calc_and_print_results(double start, double end, int len,
                                         perf_metrics_t data) {
    double latency = 0.0;
    latency = (end - start) / data.trials;

    printf("%9d           %8.2f             \n", len, latency * 1000000.0);
}

void static inline command_line_arg_check(int argc, char *argv[],
                            perf_metrics_t *metric_info) {
    int ch, error = FALSE;
    extern char *optarg;

    /* check command line args */
    while ((ch = getopt(argc, argv, "e:s:n:v")) != EOF) {
        switch (ch) {
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
            fprintf(stderr, "Usage: %s [-s start_length] [-e end_length] "\
                    ": lengths must be a power of two \n " \
                    "[-n trials (must be greater than 20)] "\
                    "[-v (validate results)]\n", argv[0]);
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

/**************************************************************/
/*                   Latency data gathering                   */
/**************************************************************/

/*have single symmetric long element "target" from perf_metrics_t
 *  that needs to be initialized in function*/
extern void long_element_round_trip_latency(perf_metrics_t data);

/*have symmetric buffer "buf" from perf_metrics_t
 *  that has been initialized to my_node number */
extern void streaming_latency(int len, perf_metrics_t *data);

void static inline  multi_size_latency(perf_metrics_t data, char *argv[]) {
    int len;
    int partner_pe = partner_node(data.my_node);
    char type[4];

    /*assumes the exec name is standardized, hacky way to get type from exec name*/
    type [3] = '\0';
    strncpy(type, &argv[0][77], 3); /*put and get are both 3 letters*/
    /*you'll likely need to modify this code if this assert isn't true */
    assert((0 == strncmp(type, "put", 3)) || (0 == strncmp(type, "get", 3)));

    if (data.my_node == 0) {
       printf("\nResults for %d trials each of length %d through %d in"\
              " powers of %d\n", data.trials, data.start_len,
              data.max_len, data.inc);
       printf("\nshmem_%smem results:\n", type);
       print_results_header();
    }

    for (len = data.start_len; len <= data.max_len; len *= data.inc) {

        shmem_barrier_all();

        streaming_latency(len, &data);

        shmem_barrier_all();
    }

    shmem_barrier_all();

    if((data.my_node == 0) && data.validate)
        validate_recv(data.buf, data.max_len, partner_pe);
}



/**************************************************************/
/*                   INIT and teardown of resources           */
/**************************************************************/

void static inline latency_init_resources(int argc, char *argv[],
                                          perf_metrics_t *data) {
    shmem_init();

    data_init(data);

    only_two_PEs_check(data->my_node, data->npes);

    command_line_arg_check(argc, argv, &data);
    data->buf = aligned_buffer_alloc(data->max_len);
    init_array(data->buf, data->max_len, data->my_node);
    data->target = shmem_malloc(sizeof(long));
}

void static inline latency_free_resources(perf_metrics_t *data) {
    shmem_barrier_all();

    shmem_free(data->target);
    shmem_free(data->buf);
    shmem_finalize();
}

void static inline latency_main(int argc, char *argv[]) {
    perf_metrics_t data;

    latency_init_resources(argc, argv, &data);

    long_element_round_trip_latency(data);

    multi_size_latency(data, argv);

    latency_free_resources(&data);
}
