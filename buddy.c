#include "buddy.h"
#include <stdio.h>
#include <stdlib.h>
#define NULL ((void *)0)

typedef struct BPage BPage;
struct BPage {
    int size;
    int rank;
    void* start;
    int alloc;
    BPage* buddy;
    BPage* par;
    BPage* prec;
    BPage* succ;
};

BPage* new_page(int rank, void* start, int alloc) {
    BPage* pg = (BPage*)malloc(sizeof(BPage));
    pg -> size = (BASIC_PAGE << (rank - 1));
    pg -> rank = rank;
    pg -> start = start;
    pg -> alloc = alloc;
    pg -> par = pg -> buddy = NULL;
    pg -> prec = pg -> succ = NULL;
}

BPage *list[MAX_RANK + 1];
BPage **all;
int all_cnt, page_cnt[MAX_RANK + 1];
int max_rank;
void* start;
void* end;

void list_insert(int rank, BPage* pg) {
    pg -> succ = list[rank];
    if (list[rank] != NULL) {
        list[rank] -> prec = pg;
    }
    list[rank] = pg;
    ++page_cnt[rank];
}

BPage* list_extract(int rank) {
    BPage* ret = list[rank];
    if (ret -> succ) {
        ret -> succ -> prec = NULL;
    }
    list[rank] = ret -> succ;
    --page_cnt[rank];
    return ret;
}

void list_delete(int rank, BPage* pg) {
    if (pg == list[rank]) {
        if (pg -> succ != NULL) {
            pg -> succ -> prec = NULL;
        }
        list[rank] = pg -> succ;
    } else {
        pg -> prec -> succ = pg -> succ;
        if (pg -> succ != NULL) {
            pg -> succ -> prec = pg -> prec;
        }
    }
    --page_cnt[rank];
}

int init_page(void *p, int pgcount){
    int m = max_rank = 1;
    while (m < pgcount) {
        m <<= 1;
        ++max_rank;
    }
    start = p;
    end = p + pgcount * BASIC_PAGE;
    list_insert(max_rank, new_page(max_rank, p, 0));
    all_cnt = pgcount;
    all = (BPage**)malloc(pgcount * sizeof(BPage*));
    for (int i = 0; i < pgcount; ++i) {
        all[i] = NULL;
    }
    return OK;
}

void *alloc_pages(int rank){
    for (int r = rank; r <= max_rank; ++r) {
        if (page_cnt[r]) {
            BPage* pg = list_extract(r);
            pg -> alloc = 1;
            while (r > rank) {
                BPage* lef = new_page(r - 1, pg -> start, 1);
                BPage* rig = new_page(r - 1, pg -> start + lef -> size, 0);
                lef -> buddy = rig;
                rig -> buddy = lef;
                lef -> par = rig -> par = pg;
                list_insert(r - 1, rig);
                pg = lef;
                --r;
            }
            all[(pg -> start - start) / BASIC_PAGE] = pg;
            return pg -> start;
        }
    }
    return -ENOSPC;
}

int return_pages(void *p){
    if (p < start || p > end || (p - start) % BASIC_PAGE) {
        return -EINVAL;
    }
    BPage* pg = all[(p - start) / BASIC_PAGE];
    if (pg == NULL) {
        return -EINVAL;
    }
    all[(p - start) / BASIC_PAGE] = NULL;
    while (pg -> rank < max_rank) {
        if (pg -> buddy -> alloc) {
            break;
        }
        list_delete(pg -> rank, pg -> buddy);
        BPage* par = pg -> par;
        free(pg -> buddy);
        free(pg);
        pg = par;
    }
    pg -> alloc = 0;
    list_insert(pg -> rank, pg);
    return OK;
}

int query_ranks(void *p) {
    if (p < start || p > end || (p - start) % BASIC_PAGE) {
        return -EINVAL;
    }
    BPage* pg = all[(p - start) / BASIC_PAGE];
    if (pg != NULL) {
        return pg -> rank;
    }
    for (int r = max_rank; r >= 1; --r) {
        BPage* u = list[r];
        while (u != NULL) {
            if (u -> start <= p && p < u -> start + u -> size) {
                return r;
            }
            u = u -> succ;
        }
    }
    return -EINVAL;
}

int query_page_counts(int rank){
    if (rank <= 0 || rank > max_rank) {
        return -EINVAL;
    }
    return page_cnt[rank];
}

int free_all_pages() {
    for (int i = 0; i < all_cnt; ++i) {
        if (all[i] != NULL) {
            free(all[i]);
            all[i] = NULL;
        }
    }
    free(all);
    for (int r = 1; r <= max_rank; ++r) {
        BPage* u = list[r];
        while (u != NULL) {
            BPage* v = u -> succ;
            free(u);
            u = v;
        }
        list[r] = NULL;
    }
    return 1;
}