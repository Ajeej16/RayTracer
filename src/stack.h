
#ifndef STACK_H
#define STACK_H

typedef struct stack_header_t {
    u32 cap;
    u32 count;
} stack_header_t;

#define STACK_INITIAL_CAP 16

#define get_stack_header(b) ((stack_header_t *)((u8 *)b - sizeof(stack_header_t)))

#define get_stack_cap(b) ((b) ? get_stack_header(b)->cap : 0)
#define _get_stack_size(b, s) ((b) ? (get_stack_header(b)->count*s) : 0)
#define get_stack_size(b) _get_stack_size(b, sizeof(*b))
#define get_stack_count(b) ((b) ? get_stack_header(b)->count : 0)
#define get_stack_last(b) ((b && get_stack_header(b)->count != 0) ? b+(get_stack_header(b)->count-1) : 0)
#define stack_clear(b) ((b) ? (get_stack_header(b)->count = 0) : 0)
#define stack_free(b) free(get_stack_header(b))

// NOTE(ajeej): the stack only contains pointers
static i32
_stack_resize(void **base_ptr, u32 new_cap, u32 item_size)
{
    void *base = *base_ptr;
    if(new_cap == 0) new_cap = STACK_INITIAL_CAP;
    u32 cap = get_stack_cap(base);
    if(new_cap == cap) return 1;
    
    u32 size = _get_stack_size(base, item_size)+sizeof(stack_header_t);
    u32 new_size = item_size*new_cap+sizeof(stack_header_t);
    
    u32 was_uninitialized = 0;
    if(base == NULL) was_uninitialized = 1;
    
    stack_header_t *head = (stack_header_t *)realloc(was_uninitialized ? NULL : get_stack_header(base), new_size);
    if(head == NULL) return 0;
    
    head->cap = new_cap;
    if(was_uninitialized) { head->count = 0; };
    
    *base_ptr = (void *)((u8 *)head + sizeof(*head));
    
    return 1;
}

#define stack_fit(b, c) _fit_stack((void **)b, c, sizeof(**b))
static void *
_stack_fit(void **base_ptr, u32 item_count, u32 item_size)
{
    void *base = *base_ptr;
    u32 new_count = item_count;
    
    if(base == NULL) goto resize;
    
    stack_header_t *head = get_stack_header(base);
    new_count += head->count;
    
    if(new_count > head->cap)
    {
        resize: {}
        
        u32 new_cap = (base != NULL && head->cap) ? head->cap * 2 : STACK_INITIAL_CAP;
        
        if(new_count > new_cap)
            new_cap = new_count;
        
        if(!_stack_resize(&base, new_cap, item_size)) return NULL;
    }
    
    *base_ptr = base;
    return base;
}

// TODO(ajeej): fix stack push system (just ass in what u want to push)
#define stack_push(b) _stack_push((void **)b, 1, sizeof(**b))
#define stack_push_array(b, c) _stack_push((void **)b, c, sizeof(**b))
static void *
_stack_push(void **base_ptr, u32 item_count, u32 item_size)
{
    void *base = _stack_fit(base_ptr, item_count, item_size);
    if(base == NULL) return NULL;
    stack_header_t *head = get_stack_header(base);
    
    void *element = (void *)((u8 *)base + item_size*head->count);
    memset(element, 0, item_count*item_size);
    head->count += item_count;
    return element;
}

#define stack_pop(b) get_stack_header(b)->count -= 1
#define stack_pop_array(b, c) get_stack_header(b)->count -= c

#define STACK(s) s

#endif //STACK_H
