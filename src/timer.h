
#ifndef TIMER_H
#define TIMER_H

struct timer_t {
    timespec start;
    timespec end;
    u64 nanos_elapsed;
};


static void
init_timer(timer_t *timer)
{
    timer->nanos_elapsed = 0;
}

static u32
start_timer(timer_t *t)
{
    if(timespec_get(&t->start, TIME_UTC) == 0)
        return 1;
    return 0;
}

static u32
end_timer(timer_t *t)
{
    if(timespec_get(&t->end, TIME_UTC) == 0)
        return 1;
    
    t->nanos_elapsed += (t->end.tv_sec-t->start.tv_sec)*1E9 + (t->end.tv_nsec-t->start.tv_nsec);
    
    return 0;
}

static u64
check_timer(timer_t *t)
{
    end_timer(t);
    start_timer(t);
    
    return t->nanos_elapsed;
}

static void
reset_timer(timer_t *t)
{
    t->nanos_elapsed = 0;
}

#endif //TIMER_H
