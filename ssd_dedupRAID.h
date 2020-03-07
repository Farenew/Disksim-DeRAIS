#ifndef _DISKSIM_SSD_DEDUPRAID_H
#define _DISKSIM_SSD_DEDUPRAID_H

#include "disksim_global.h"
#include "cfuhash.h"
#include "ssd_utils.h"
#include "ssd.h"


#define HASH_SIZE 40

#define TRACE_WRITE 0
#define TRACE_READ 1
#define TRACE_DELETE 2

#define PAGE_SIZE 8
#define SINGLE_SSD_BLOCK 9216
#define PAGES_PER_BLOCK 64
#define SINGLE_SSD_PAGE SINGLE_SSD_BLOCK * PAGES_PER_BLOCK
#define SSD_NUM 3
#define TOTAL_SSD_PAGE SINGLE_SSD_PAGE * SSD_NUM

// #define HASH_SIZE 32
// //#define HASH_SIZE 256

// #define TRACE_WRITE 0
// #define TRACE_READ 1
// #define TRACE_DELETE 2

// #define PAGE_SIZE 8
// #define SINGLE_SSD_BLOCK 8192
// #define PAGES_PER_BLOCK 64
// #define SINGLE_SSD_PAGE SINGLE_SSD_BLOCK * PAGES_PER_BLOCK
// #define SSD_NUM 3
// #define TOTAL_SSD_PAGE SINGLE_SSD_PAGE * SSD_NUM

#define TIME_TICK 0.00002

#define PRINT_MARK 10000

#define FULL_WRITE 0
#define SEQ_WRITE 1

/* #region  log data */
typedef struct LOG_DATA
{
    int traceRead;
    int traceWrite;

    int totalRead;
    int totalWrite;

    int readAfterArch;
    int writeAfterArch;

    int traceNum;

    int logicalGC;

    int traceReadAfterWarmUp;
    int traceWriteAfterWarmUp;

    int totalReadAfterWarmUp;
    int totalWriteAfterWarmUp;

    int readAfterArchAfterWarmUp;
    int writeAfterArchAfterWarmUp;

    int traceNumAfterWarmUp;

    int logicalGCAfterWarmUp;

    int warmupGC;

} logdata;

void log_initial_raid_data(logdata *rd);
void log_trace_add(logdata *rd, int type, int warmup);
void log_total_add(logdata *rd, int type, int warmup);
void log_request_add(logdata *rd, int type, int warmup);
void log_another_trace(logdata *rd, int warmup);
void log_final_print(logdata *rd);
void log_logicalGC_add(logdata *rd, int warmup);
void log_print(logdata *rd);

void log_set_warmupGC(logdata *rd, int gctimes);

extern logdata *logData;

/* #endregion */

/* #region  parse line */
typedef struct TRACE
{
    double time;
    int blkno;
    int bcount;
    int flags;
    char *hash;
    // hashStart is used to record start location for hash, convenient for free
    char *hashStart;
} trace;

// parse string, get bcount at location
int get_bcount(char *string, int location);

// this function is not memory safe, it mallocs but not free here
void get_ioreq(trace *tr, ioreq_event *new, int pageSize);

/* #endregion */

typedef struct page
{
    int pageNumber;
    int remapPage;
    int refTimes;
    char *hash;
    listnode *deRefPages;
} page;

// used at the beginning of program, namely disksim_main.c
void initialize_architecture();

int isInHashTable(char *hash);
void set_page_map(int startPage, int endPage, char *hash);
int find_empty_page();
void write_into_new_page(int page, char *hash);
void write_into_hash_table(int page, char *hash);

#define GROUP_NUM 4

#define GC_STATE -2
#define EMPTY_STATE -1

#define EMPTY -1
#define FULL SSD_NUM
#define DEAD 0
#define OTHERS -2

#define NEXT 1
#define PREV 0

/* stripe class, to store stripe informations

    -> stripeNo, the no of stripe, from 0 to block no of ssd

    -> valid pages, the valid pages of stripe, a total num for writeable pages

    -> fullState, a array align with full write, EMPTY means initial blank, FULL
    means it is fully written, contains num of data just as ssd num, DEAD measn
    all data is deleted. other values from 1 to SSD_NUM indicates how many data
    are written
*/
typedef struct stripe{
    int stripeNo;
    int validPages;
    int fullState[PAGES_PER_BLOCK];
}stripe;

typedef struct group{
    int stripes[SINGLE_SSD_BLOCK];
}group;

extern int workingStripe[GROUP_NUM];
extern int fullStripe[GROUP_NUM];
extern int writingPage;
extern int FULL_WRITE_TAG;
extern int BLKNO;

int page_to_stripe(int page);
int page_to_stripe_align(int page);

int find_empty_page_from_stripe(int stripeNo, int method);
int find_stripe_to_write(int method);
int find_page_from_stripe(int stripeNo, int page, int method);
void update_stripe_state(int stripeNo, int page, int method);
void assemble_request(ioreq_event *new, int stripeNo, int page);
int stripeIsFull(int stripeNo);

int find_stripe_from_group(int groupNo);


// at which level should we do gc, if writeable stripe is less than this threshold, then gc
#define GC_THRESHOLD 0.05

#define IN_GC 1
#define NOT_GC 0

extern int GCpages;
extern int GC_TAG;
extern int GClevel;
extern long double currentTime;
extern int GCstripe;
extern int workingGCpage;

int enterGCstate();
int find_gc_stripe();
int get_page_from_GC(int stripeNo);
void move_GC_page(int gcpage, int newPage);

extern int warmupset;
extern const int WARM_UP_TRACE;

#endif