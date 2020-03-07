#include <string.h>
#include <stdlib.h>

#include "ssd_dedupRAID.h"


// #region log

logdata *logData;

void log_initial_raid_data(logdata *rd){
    rd->readAfterArch = 0;
    rd->writeAfterArch = 0;
    rd->totalRead = 0;
    rd->totalWrite = 0;
    rd->traceRead = 0;
    rd->traceWrite = 0;
    rd->traceNum = 0;
    rd->logicalGC = 0;

    rd->readAfterArchAfterWarmUp = 0;
    rd->writeAfterArchAfterWarmUp = 0;
    rd->totalReadAfterWarmUp = 0;
    rd->totalWriteAfterWarmUp = 0;
    rd->traceReadAfterWarmUp = 0;
    rd->traceWriteAfterWarmUp = 0;
    rd->traceNumAfterWarmUp = 0;
    rd->logicalGCAfterWarmUp = 0;

    rd->warmupGC = 0;
}

void log_set_warmupGC(logdata *rd, int gctimes){
    rd->warmupGC = gctimes;
    printf("now set warm up gc times as %d\n", gctimes);
}

void log_trace_add(logdata *rd, int type, int warmup){
    if (type == TRACE_WRITE)
    {
        rd->traceWrite++;
    }
    if (type == TRACE_READ)
    {
        rd->traceRead++;
    }

    if(warmup == 1){
        if (type == TRACE_WRITE)
        {
            rd->traceWriteAfterWarmUp++;
        }
        if (type == TRACE_READ)
        {
            rd->traceReadAfterWarmUp++;
        }
    }
}

void log_total_add(logdata *rd, int type, int warmup){
    if (type == TRACE_WRITE)
    {
        rd->totalWrite++;
    }
    if (type == TRACE_READ)
    {
        rd->totalRead++;
    }
    if(warmup == 1){
        if (type == TRACE_WRITE)
        {
            rd->totalWriteAfterWarmUp++;
        }
        if (type == TRACE_READ)
        {
            rd->totalReadAfterWarmUp++;
        }
    }
}

void log_request_add(logdata *rd, int type, int warmup){
    if (type == TRACE_WRITE)
    {
        rd->writeAfterArch++;
    }
    if (type == TRACE_READ)
    {
        rd->readAfterArch++;
    }
    if(warmup == 1){
        if (type == TRACE_WRITE)
        {
            rd->writeAfterArchAfterWarmUp++;
        }
        if (type == TRACE_READ)
        {
            rd->readAfterArchAfterWarmUp++;
        }
    }
}

void log_another_trace(logdata *rd, int warmup){
    rd->traceNum++;
    if(warmup == 1){
        rd->traceNumAfterWarmUp++;
    }
}

void log_logicalGC_add(logdata *rd, int warmup){
    rd->logicalGC++;
    if(warmup == 1){
        rd->logicalGCAfterWarmUp++;;
    }
}

void log_print(logdata *rd){
    if (rd->traceNum % PRINT_MARK  == 0 && rd->traceNum != 0)
    {
        printf("now have done %d traces\n", rd->traceNum);
        hash_table_statistics();
    }
    if (rd->totalWrite % PRINT_MARK == 0 && rd->totalWrite != 0)
    {
        printf("now sent %d writes from trace\n", rd->totalWrite);
    }
    if (rd->totalRead % PRINT_MARK == 0 && rd->totalRead != 0)
    {
        printf("now sent %d reads from trace\n", rd->totalRead);
    }
    if (rd->readAfterArch % PRINT_MARK == 0 && rd->readAfterArch != 0)
    {
        printf("now sent %d reads after architecture\n", rd->readAfterArch);
    }
    if (rd->writeAfterArch % PRINT_MARK == 0 && rd->writeAfterArch != 0)
    {
        printf("now sent %d writes after architecture\n", rd->writeAfterArch);
    }
    if (rd->logicalGC % (PRINT_MARK / 2) == 0 && rd->logicalGC != 0)
    {
        printf("now done %d logical gc pages\n", rd->logicalGC);
    }
}

void log_final_print(logdata *rd){
    printf("---------------------------------------------------\n");
    printf("total Statistics:\n");
    printf("Trace Read Number %d\n", rd->traceRead);
    printf("Trace Write Number %d\n", rd->traceWrite);
    printf("Total Read Number %d\n", rd->totalRead);
    printf("Total Write Number %d\n", rd->totalWrite);
    printf("Read after architecture %d\n", rd->readAfterArch);
    printf("Write after architecture %d\n", rd->writeAfterArch);

    printf("logical gc pages %d\n", rd->logicalGC);
    printf("file total trace %d\n", rd->traceNum);


    printf("---------------------------------------------------\n");
    printf("Statistics after warm up:\n");
    printf("total gc after warm up: %d\n", rd->warmupGC);
    printf("Trace Read Number %d\n", rd->traceReadAfterWarmUp);
    printf("Trace Write Number %d\n", rd->traceWriteAfterWarmUp);
    printf("Total Read Number %d\n", rd->totalReadAfterWarmUp);
    printf("Total Write Number %d\n", rd->totalWriteAfterWarmUp);
    printf("Read after architecture %d\n", rd->readAfterArchAfterWarmUp);
    printf("Write after architecture %d\n", rd->writeAfterArchAfterWarmUp);

    printf("logical gc pages %d\n", rd->logicalGCAfterWarmUp);
    printf("file total trace %d\n", rd->traceNumAfterWarmUp);

    printf("total gc times %d\n", gctimes);

    printf("---------------------------------------------------\n");
}

// #endregion



// #region request parse

// bcount is stored at string's location
int get_bcount(char *string, int location){
    char *s;
    char *tofree;
    char *bcount;
    int rtn;

    int i;
    tofree = s = strdup(string);
    ASSERT(s != NULL);

    for (i = 0; i < location; i++){
        bcount = strsep(&s, " ");
        ASSERT(bcount != NULL);
    }

    rtn = atoi(bcount);
    free(tofree);
    return rtn;
}

void get_ioreq(trace *tr, ioreq_event *new, int pageSize){
    int i;
    ASSERT(tr->bcount >= pageSize);
    new->bcount = pageSize;
    new->blkno = tr->blkno;
    new->time = tr->time;
    new->flags = tr->flags;
    new->hash = strndup(tr->hash, HASH_SIZE+1);

    (new->hash)[HASH_SIZE] = '\0';

    // move to next request
    tr->blkno += pageSize;
    tr->bcount -= pageSize;
    tr->time += TIME_TICK;
    // move hash to next hash
    ASSERT(HASH_SIZE <= strlen(tr->hash));
    for (i = 0; i < HASH_SIZE; i++)
    {
        tr->hash++;
    }
}

// #endregion



// #region deduplication

static cfuhash_table_t *deduptable;

// print hash table statistics
void hash_table_statistics(){
    printf("%d entries, %d buckets used out of %d\n",
           cfuhash_num_entries(deduptable), cfuhash_num_buckets_used(deduptable),
           cfuhash_num_buckets(deduptable));
}

// initialize hash table
static void ini_hash_table(){
    deduptable = cfuhash_new_with_initial_size(2);
    cfuhash_set_flag(deduptable, CFUHASH_FROZEN_UNTIL_GROWS);
}

// page data
static page *ssdPages;

// local function, used to initialize all pages
static void ini_pages(int pageNums){
    int i;

    ssdPages = (page *)malloc(sizeof(page) * pageNums);
    if(ssdPages == NULL){
        printf("cannot initialize pages!\n");
        printf("the size of page is %d, we want to alloc %d nums\n", sizeof(page), pageNums);
        exit(1);
    }
    for (i = 0; i < pageNums; i++){
        ssdPages[i].pageNumber = i;
        ssdPages[i].remapPage = -1;
        ssdPages[i].refTimes = 0;
        ssdPages[i].hash = NULL;
        ll_create(&(ssdPages[i].deRefPages));
    }
}

// if hash exists in hashtable
int isInHashTable(char *hash){
    int page = -1;
    if (cfuhash_exists(deduptable, hash))
    {
        page = *(int *)cfuhash_get(deduptable, hash);
        ASSERT(page != -1 && page < TOTAL_SSD_PAGE);
    }
    return page;
}

// delete a page
static void delete_page(int page){
    char *hash = ssdPages[page].hash;
    ASSERT(ssdPages[page].refTimes == 0);
    ASSERT(hash != NULL);
    // make sure this page exits in hash table
    ASSERT(cfuhash_exists(deduptable, hash));
    // make sure this page has no dereference
    ASSERT(ll_get_size(ssdPages[page].deRefPages) == 0);

    // delete from hash table
    cfuhash_delete(deduptable, hash);
    // free this hash, which is created in get_ioreq, at which we called strndup()
    free(hash);
    ssdPages[page].hash = NULL;

    int stripeNo = page_to_stripe(page);
    update_stripe_state(stripeNo, page, TRACE_DELETE);
}

// the naked setting remap function with no margins check
static void set_remap(int startPage, int endPage){
    ssdPages[startPage].remapPage = endPage;
    ssdPages[endPage].refTimes++;
    ll_insert_at_head(ssdPages[endPage].deRefPages,
                      (listnode *)&(ssdPages[startPage].pageNumber));
}

// cancel remap, if cancel made remap page has no reference, then delete it
static void cancel_remap(int startpage, int remapPage){
    int size = ll_get_size(ssdPages[remapPage].deRefPages);
    int i;
    ASSERT(size >= 1 && size == ssdPages[remapPage].refTimes);
    ASSERT(ssdPages[startpage].remapPage == remapPage);

    ssdPages[remapPage].refTimes--;

    // iterate dereference, delete.
    for (i = 0; i < size; i++)
    {
        listnode *node = ll_get_nth_node(ssdPages[remapPage].deRefPages, i);
        int refPage = *(int *)(node->data);
        if (refPage == startpage)
        {
            ll_release_node_keep(ssdPages[remapPage].deRefPages, node);
            break;
        }
    }

    ASSERT(i != size);

    if (ssdPages[remapPage].refTimes == 0)
    {
        delete_page(remapPage);
    }
}

/* this is a very important function, indicates how we map our request
   there are two cases:
   -> startPage has no remap page, just set remap
   -> startPage already has a remap page, cancel previous remap, then set new remap
   -> set remap to same place, just skip this
*/
void set_page_map(int startPage, int endPage, char *hash){

    int remapPage = ssdPages[startPage].remapPage;

    if (remapPage == endPage)
    {
        // a rare case, it happeds when write same content at same address
        return;
    }

    ASSERT(remapPage < TOTAL_SSD_PAGE &&
           startPage < TOTAL_SSD_PAGE &&
           endPage < TOTAL_SSD_PAGE);
    ASSERT(ssdPages[endPage].hash != NULL);
    ASSERT(strcmp(hash, ssdPages[endPage].hash) == 0);

    // if remap page is -1, means this page is fresh, so just set remap
    if (remapPage == -1)
    {
        set_remap(startPage, endPage);
    }
    // if remap already exists, cancel it first
    else
    {
        ASSERT(ssdPages[remapPage].refTimes > 0);
        // when cancel remap, if refTimes drops to 0, make delete inside this function
        cancel_remap(startPage, remapPage);
        set_remap(startPage, endPage);
    }
}

static int is_empty_page(int page){
    ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);
    if (ssdPages[page].hash == NULL &&
            ssdPages[page].refTimes == 0 &&
            ll_get_size(ssdPages[page].deRefPages) == 0){
                return 1;
            }
    else{
        return 0;
    }
}

// TODO: this methods needs to be changed, here just iterates pages, the better way for
// deduplication system is to use a set.
// TODO: new api is int find_empty_page(int stripe);
int find_empty_page(){
    int i;
    for (i = 0; i < TOTAL_SSD_PAGE; i++)
    {

        if (is_empty_page(i))
        {
            return i;
        }
    }
    printf("there is no empty page to write!!!!!\n");
    ASSERT(0);
}

// write hash into a page
void write_into_new_page(int page, char *hash){
    ASSERT(page >= 0 && page <= TOTAL_SSD_PAGE);
    ASSERT(is_empty_page(page));

    ssdPages[page].hash = hash;
}

// write hash into hash table
void write_into_hash_table(int page, char *hash){
    ASSERT(!cfuhash_exists(deduptable, hash));

    // cfuhash table's value is void*, so here we have to chagne the type
    cfuhash_put(deduptable, hash, (void *)&ssdPages[page].pageNumber);

    ASSERT(cfuhash_exists(deduptable, hash));

    ASSERT(*(int *)cfuhash_get(deduptable, hash) == page);
}

// #endregion


// # region stripe full write


static stripe* stripes;

static group* groups;

// the stripe we are currently writing
int workingStripe[GROUP_NUM];
// the pages accumulated for full stripe, if it equals SSD_NUM means now it's a
// full write
int fullStripe[GROUP_NUM];
// the working page we are writing to
int writingPage = -1;
// a tag to indicate now doing a full stripe write, used in disksim_redun.c
int FULL_WRITE_TAG = 0;
// used to indicate first write blkno, to calculate how to do full stripe, used
// in disksim_redun.c, defined in disksim_iodriver.c
int BLKNO;

int r_star = 10;


static const int STRIPEPAGES = SSD_NUM * PAGES_PER_BLOCK;

// initialize stripes
static void ini_stripes(int stripeNums){
    int i,j;
    stripes = (stripe*)malloc(sizeof(stripe) * stripeNums);
    if(stripes == NULL){
        printf("cannot alloc memorys for stripes!\n");
        exit(1);
    }
    for(i=0; i<stripeNums; i++){
        stripes[i].stripeNo = i;
        stripes[i].validPages = PAGES_PER_BLOCK * SSD_NUM;
        for(j=0; j<PAGES_PER_BLOCK; j++){
            stripes[i].fullState[j] = EMPTY;
        }
    }
}

/* here we used Buddy algorithm idea, divide groups as half, and then half.
   the idea is that for low ref count it tends to write more. With ref rising,
   the apperance tends to lower. the last to group would share same amount of 
   stripe num
*/
static void ini_groups(){
    int i;
    int granularity = SINGLE_SSD_BLOCK / 2;
    int thres = granularity - 1;
    int iniValue = 0;
    groups = (group*)malloc(sizeof(group));
    for(i=0; i<SINGLE_SSD_BLOCK; i++){
        groups->stripes[i] = iniValue;
        if(i == thres){
            iniValue++;
            if(iniValue == GROUP_NUM - 1)
                continue;
            granularity /= 2;
            thres += granularity;
        }
    }

    // initialize the working stripes and stripe state
    for(i=0; i<GROUP_NUM; i++){
        workingStripe[i] = EMPTY_STATE;
        fullStripe[i] = EMPTY_STATE;
    }

}

// supports EMPTY, FULL, DEAD as main methods, as well as others as extension
static int find_full_slots(int stripeNo, int goal, int value){
    int i;
    int sum = 0;
    for(i=0; i<PAGES_PER_BLOCK; i++){
        if(goal == EMPTY && stripes[stripeNo].fullState[i] == EMPTY){
            sum += 1;
        }
        else if(goal == FULL && stripes[stripeNo].fullState[i] == FULL){
            sum += 1;
        }
        else if(goal == DEAD && stripes[stripeNo].fullState[i] == DEAD){
            sum += 1;
        }
        else if(goal == OTHERS && stripes[stripeNo].fullState[i] == value){
            sum += 1;
        }
    }

    return sum;
}

// two methods suppored currently, SEQ_WRITE and FULL_WRITE, if there is no page
// to write, program would stop
int find_empty_page_from_stripe(int stripeNo, int method){
    if(method == SEQ_WRITE){
        int startPage = stripeNo * STRIPEPAGES;
        int endPage = (stripeNo + 1) * STRIPEPAGES;
        int i;
        for(i=startPage; i<endPage; i++){
            if(is_empty_page(i))
            {
                return i;
            }
        }
        printf("there is no empty page in this STRIPE!!!!!\n");
        ASSERT(0);
    }
    if(method == FULL_WRITE){
        int i;
        for(i=0; i<PAGES_PER_BLOCK; i++){
            if(stripes[stripeNo].fullState[i] == EMPTY || stripes[stripeNo].fullState[i] == DEAD){
                int page = stripeNo * STRIPEPAGES + i;
                if(is_empty_page(page))
                {
                    return page;
                }
            }
        }
        printf("there is no empty page in this STRIPE for FULL WRITE!!!!!\n");
        ASSERT(0);
    }
}

int get_ref_from_page(int page){
    ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);
    return ssdPages[page].refTimes;
}

// find group No based on ref, be aware of the int and float transations.
int find_group(int ref){
    if(ref > r_star){
        r_star = ref;
        return GROUP_NUM - 1;
    }
    float granularity = (float)r_star / GROUP_NUM;
    if(granularity == 0)
        granularity = 1;
    return (int)(ref / granularity);
}

// find a writeable stripe based on groupNo
static int choose_writeable_stripe_from_group(int groupNo){
    int i;
    int maxValid = 0;
    int choose = -1;

    for(i=0; i<SINGLE_SSD_BLOCK; i++){
        if(groups->stripes[i] == groupNo){
            if(find_full_slots(i, EMPTY, EMPTY) > maxValid){
                maxValid = find_full_slots(i, EMPTY, EMPTY);
                choose = i;
            }
        }
    }

    // if there is no empty slot, still it might got some dead slots
    if(choose == -1){
        for(i=0; i<SINGLE_SSD_BLOCK; i++){
            if(groups->stripes[i] == groupNo){
                if(find_full_slots(i, DEAD, DEAD) > maxValid){
                    maxValid = find_full_slots(i, DEAD, DEAD);
                    choose = i;
                }
            }
        }
    }
    return choose;
}

// find a stripe no based on groupNo, if cannot find, then find from next group
int find_stripe_from_group(int groupNo){
    int choose = -1;
    int workingGroup = groupNo;
    int times = 0;
    while(choose == -1){
        choose = choose_writeable_stripe_from_group(workingGroup);
        workingGroup = (workingGroup + 1) % GROUP_NUM;
        if(times++ == GROUP_NUM)
            break;
    }

    groups->stripes[choose] = groupNo;

    if(choose == -1){
        printf("all stripes are full!!!! cannot find!!!\n");
        ASSERT(0);
    }
    return choose;
}

// two methods suppored currently, SEQ_WRITE find the stripe contains maximum
// empty pages and FULL_WRITE, find the stripe contains maximum full writes
int find_stripe_to_write(int method){
    int i;
    int maxValid = 0;
    int choose = -1;
    
    if(method == SEQ_WRITE){
        for(i=0; i<SINGLE_SSD_BLOCK; i++){
            if(stripes[i].validPages > maxValid){
                maxValid = stripes[i].validPages;
                choose = i;
            }
        }
        ASSERT(choose != -1);
        ASSERT(maxValid > 0);
    }

    if(method == FULL_WRITE){
        for(i=0; i<SINGLE_SSD_BLOCK; i++){
            if(find_full_slots(i, EMPTY, EMPTY) > maxValid){
                maxValid = find_full_slots(i, EMPTY, EMPTY);
                choose = i;
            }
        }

        // if there is no empty slot, still it might got some dead slots
        if(choose == -1){
            for(i=0; i<SINGLE_SSD_BLOCK; i++){
                if(find_full_slots(i, DEAD, DEAD) > maxValid){
                    maxValid = find_full_slots(i, DEAD, DEAD);
                    choose = i;
                }
            }
        }

        ASSERT(choose != -1);
        ASSERT(maxValid > 0);
    }

    return choose;
}

int page_to_ssd(int page){
    ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);
    return (page/PAGES_PER_BLOCK) % SSD_NUM;
}

// from a page find its stripe no
int page_to_stripe(int page){
    ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);
    return page / (PAGES_PER_BLOCK * SSD_NUM);
}
// from a page find its stripe align, means the number of page in this stripe
int page_to_stripe_align(int page){
    ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);
    return page - page_to_stripe(page) * STRIPEPAGES;
}


// according to method, find a page in stripe, if cannot find, program would
// return EMPTY. methods including find next align page or previous align page
int find_page_from_stripe(int stripeNo, int page, int method){
    
    if(method == NEXT){
        int pageFind = -1;

        ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);

        pageFind = page + PAGES_PER_BLOCK;
        if(pageFind >= 0 && pageFind < TOTAL_SSD_PAGE){
            int pageFindInStripe = page_to_stripe(pageFind);
            if(pageFindInStripe == stripeNo){
                return pageFind;
            }
        }
        return EMPTY;
    }
    else if(method == PREV){
        int pageFind = -1;

        ASSERT(page >= 0 && page < TOTAL_SSD_PAGE);

        pageFind = page - PAGES_PER_BLOCK;
        if(pageFind >= 0 && pageFind < TOTAL_SSD_PAGE){
            int pageFindInStripe = page_to_stripe(pageFind);
            if(pageFindInStripe == stripeNo){
                return pageFind;
            }
        }
        return EMPTY;
    }
    else{
        printf("the wrong method!!!!\n");
        ASSERT(0);
    }
}


// update stripe state according to method, method supports TRACE_WRITE and
// TRACE_DELETE
void update_stripe_state(int stripeNo, int page, int method){
    int i;
    if(method == TRACE_WRITE){
        int align = page_to_stripe_align(page) % PAGES_PER_BLOCK;
        ASSERT(align >= 0 && align < PAGES_PER_BLOCK);
        ASSERT(stripes[stripeNo].fullState[align] != FULL);
        if(stripes[stripeNo].fullState[align] == EMPTY){
            stripes[stripeNo].fullState[align] = 1;
        }
        else{
            stripes[stripeNo].fullState[align] ++;
        }
        stripes[stripeNo].validPages --;
    }
    else if(method == TRACE_DELETE){
        int align = page_to_stripe_align(page) % PAGES_PER_BLOCK;
        ASSERT(align >= 0 && align < PAGES_PER_BLOCK);
        ASSERT(stripes[stripeNo].fullState[align] != EMPTY && stripes[stripeNo].fullState[align] != DEAD);
        stripes[stripeNo].fullState[align] --;
        stripes[stripeNo].validPages ++;
    }
    else{
        printf("unknown method !!!! \n");
        ASSERT(0);
    }
}

// pass parameters to new, or disksim would cause an error
void assemble_request(ioreq_event *new, int stripeNo, int page){
    int firstStripePage;
    int find = find_page_from_stripe(stripeNo, page, PREV);
    if(find == EMPTY){
        printf("cannot assemble request!\n");
        ASSERT(0);
    }
    while(find != EMPTY){
        firstStripePage = find;

        find = find_page_from_stripe(stripeNo, find, PREV);
    }
    new->blkno = firstStripePage * PAGE_SIZE;
    new->bcount = PAGE_SIZE;
    new->time = currentTime;
    new->flags = TRACE_WRITE;
}

// if stripe is full and cannot hold more full write, if so, return true
int stripeIsFull(int stripeNo){
    if(find_full_slots(stripeNo, EMPTY, EMPTY) == 0){
        if(find_full_slots(stripeNo, DEAD, DEAD) == 0){
            return 1;
        }
    }
    return 0;
}


// # endregion

// # GC region

// how many pages to do gc
int GCpages = 0;
// global indicator to tell if we are at GC or not
int GC_TAG = NOT_GC;
// level starts from 1, doing gc to failed full-write with only 1 invalid pages
// left
int GClevel = EMPTY;
// time used to assemble request
long double currentTime;
// the stripe to do gc
int GCstripe = EMPTY;
// current gc page
int workingGCpage = EMPTY;

// if we should enter gc state or not, so long as there are dead or empty slots,
// we are not entering gc. 
int enterGCstate(){
    int useableStripe = 0;
    int i;
    for(i=0 ;i<SINGLE_SSD_BLOCK; i++){
        if(find_full_slots(i, EMPTY, EMPTY) > 0 || find_full_slots(i, DEAD, DEAD) > 0){
            useableStripe++;
        }
    }
    float ratio = (float)useableStripe / SINGLE_SSD_BLOCK;
    if(ratio < GC_THRESHOLD){
        return 1;
    }
    return 0;
}



// iterates all stripes, find the one that has the most invalid gc level pages.
// if gc level is not supported, gc level would rise automaticlly, untill cannot
// find a slot to do gc. In this function we set GClevel as well
int find_gc_stripe(){
    int i;
    int state = 1;
    int maxLeastUsage = 0;
    int choose = -1;
    while(state < SSD_NUM){
        for(i=0; i<SINGLE_SSD_BLOCK; i++){
            if(find_full_slots(i, OTHERS, state) > maxLeastUsage){
                choose = i;
                maxLeastUsage = find_full_slots(i, OTHERS, state);
            }
        }
        if(maxLeastUsage > 0 && choose != -1){
            GCpages = maxLeastUsage * state;
            GClevel = state;
            return choose;
        }
        state++;
    }
    printf("all pages are fully written! cannot collect anymore pages!\n");
    ASSERT(0);
}

// according to stripe no, find the pages should do gc
int get_page_from_GC(int stripeNo){
    int i;
    int page = -1;
    for(i=0; i<PAGES_PER_BLOCK; i++){
        if(stripes[stripeNo].fullState[i] == GClevel){
            int find = 0;
            while(find < SSD_NUM){
                page = stripeNo * STRIPEPAGES + find * PAGES_PER_BLOCK + i;
                if(!is_empty_page(page)){
                    break;
                }
                find ++;
            }
            ASSERT(page != EMPTY);
            break;
        }
    }

    ASSERT(page != EMPTY);
    return(page);
}

// a rather complicated function, wraps lots of operations. including:
// - copy the hash
// - write hash into new page
// - cancel pages mapping to gc page one by one
// - set pages mapping to new page one by one
// - write into hash table
// this function does 2 vital margin check for two pages validation
void move_GC_page(int gcpage, int newPage){
    ASSERT(!is_empty_page(gcpage));
    ASSERT(is_empty_page(newPage));
    int i;

    char* hash = strdup(ssdPages[gcpage].hash);
    write_into_new_page(newPage, hash);

    listnode* deref = ssdPages[gcpage].deRefPages;
    int size = ll_get_size(deref);
    ASSERT(size > 0);
    ASSERT(size == ssdPages[gcpage].refTimes);

    for(i=0; i<size; i++){
        listnode* node = ll_get_nth_node(deref, 0);
        int refPage = *(int*)(node->data);

        cancel_remap(refPage, gcpage);
        set_remap(refPage, newPage);
    }

    ASSERT(is_empty_page(gcpage));
    ASSERT(!is_empty_page(newPage));

    // make sure the page is moved correctly
    ASSERT(ll_get_size(ssdPages[newPage].deRefPages) == ssdPages[newPage].refTimes);

    write_into_hash_table(newPage, hash);
}

// # endregion

int warmupset = 0;
const int WARM_UP_TRACE = 600000;

// global initialization function
void initialize_architecture(){
    ini_hash_table();
    ini_pages(TOTAL_SSD_PAGE);
    ini_stripes(SINGLE_SSD_BLOCK);
    ini_groups();
}