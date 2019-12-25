/*
 ============================================================================
 Name        : hev-task.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

#include "kern/core/hev-task-system-private.h"
#include "lib/utils/hev-compiler.h"
#include "mm/api/hev-memory-allocator-api.h"

#include "hev-task-private.h"

#include "hev-task.h"

#define STACK_OVERFLOW_DETECTION_TAG (0xdeadbeefu)

#define HEV_TASK_STACK_SIZE (64 * 1024)

#define ALIGN_DOWN(addr, align) ((addr) & ~((typeof(addr))align - 1))

EXPORT_SYMBOL HevTask *
hev_task_new (int stack_size)
{
    HevTask *self;
    uintptr_t stack_addr;

    self = hev_malloc0 (sizeof (HevTask));
    if (!self)
        return NULL;

    self->ref_count = 1;
    self->next_priority = HEV_TASK_PRIORITY_DEFAULT;

    if (stack_size == -1)
        stack_size = HEV_TASK_STACK_SIZE;

    self->stack = hev_malloc (stack_size);
    if (!self->stack) {
        hev_free (self);
        return NULL;
    }
#ifdef ENABLE_STACK_OVERFLOW_DETECTION
    *(unsigned int *)self->stack = STACK_OVERFLOW_DETECTION_TAG;
#endif

    stack_addr = (uintptr_t) (self->stack + stack_size);
    self->stack_top = (void *)ALIGN_DOWN (stack_addr, 16);
    self->stack_size = stack_size;
    self->sched_entity.task = self;

    return self;
}

EXPORT_SYMBOL HevTask *
hev_task_ref (HevTask *self)
{
    self->ref_count++;

    return self;
}

EXPORT_SYMBOL void
hev_task_unref (HevTask *self)
{
    self->ref_count--;
    if (self->ref_count)
        return;

#ifdef ENABLE_STACK_OVERFLOW_DETECTION
    assert (*(unsigned int *)self->stack == STACK_OVERFLOW_DETECTION_TAG);
#endif
    if (self->reactor)
        hev_task_io_reactor_destroy (self->reactor);
    hev_free (self->stack);
    hev_free (self);
}

EXPORT_SYMBOL HevTask *
hev_task_self (void)
{
    return hev_task_system_get_context ()->current_task;
}

EXPORT_SYMBOL HevTaskState
hev_task_get_state (HevTask *self)
{
    return self->state;
}

EXPORT_SYMBOL void
hev_task_set_priority (HevTask *self, int priority)
{
    if (priority < HEV_TASK_PRIORITY_MIN)
        priority = HEV_TASK_PRIORITY_MIN;
    else if (priority > HEV_TASK_PRIORITY_MAX)
        priority = HEV_TASK_PRIORITY_MAX;

    self->next_priority = priority;
}

EXPORT_SYMBOL int
hev_task_get_priority (HevTask *self)
{
    return self->next_priority;
}

static int
hev_task_io_reactor_init (HevTask *self)
{
    HevTaskIOReactor *reactor;
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int fd, count;

    self->reactor = hev_task_io_reactor_new ();
    if (!self->reactor)
        return -1;

    fd = hev_task_io_reactor_get_fd (self->reactor);
    count = hev_task_io_reactor_setup_event_gen (revents, fd,
                                                 HEV_TASK_IO_REACTOR_OP_ADD, 1,
                                                 HEV_TASK_IO_REACTOR_EV_RO,
                                                 &self->sched_entity);

    reactor = hev_task_system_get_context ()->reactor;
    return hev_task_io_reactor_setup (reactor, revents, count);
}

EXPORT_SYMBOL int
hev_task_add_fd (HevTask *self, int fd, unsigned int events, ...)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    va_list ap;
    int count;

    if (!self->reactor) {
        int res = hev_task_io_reactor_init (self);
        if (res < 0)
            return res;
    }

    va_start (ap, events);
    count = hev_task_io_reactor_setup_event_gen (revents, fd,
                                                 HEV_TASK_IO_REACTOR_OP_ADD, 1,
                                                 events, va_arg (ap, void *));
    va_end (ap);

    return hev_task_io_reactor_setup (self->reactor, revents, count);
}

EXPORT_SYMBOL int
hev_task_mod_fd (HevTask *self, int fd, unsigned int events, ...)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    va_list ap;
    int count;

    if (!self->reactor)
        return -1;

    va_start (ap, events);
    count = hev_task_io_reactor_setup_event_gen (revents, fd,
                                                 HEV_TASK_IO_REACTOR_OP_MOD, 1,
                                                 events, va_arg (ap, void *));
    va_end (ap);

    return hev_task_io_reactor_setup (self->reactor, revents, count);
}

EXPORT_SYMBOL int
hev_task_del_fd (HevTask *self, int fd)
{
    HevTaskIOReactorSetupEvent revents[HEV_TASK_IO_REACTOR_EVENT_GEN_MAX];
    int count;

    if (!self->reactor)
        return -1;

    count = hev_task_io_reactor_setup_event_gen (
        revents, fd, HEV_TASK_IO_REACTOR_OP_DEL, 1, 0, NULL);
    return hev_task_io_reactor_setup (self->reactor, revents, count);
}

EXPORT_SYMBOL int
hev_task_get_fd_events (HevTask *self, HevTaskFDEvent *events, int count)
{
    HevTaskIOReactorWaitEvent *_events = (HevTaskIOReactorWaitEvent *)events;

    if (!self->reactor)
        return -1;

    return hev_task_io_reactor_wait (self->reactor, _events, count, 0);
}

EXPORT_SYMBOL void
hev_task_wakeup (HevTask *task)
{
    hev_task_system_wakeup_task (task);
}

EXPORT_SYMBOL void
hev_task_yield (HevTaskYieldType type)
{
    hev_task_system_schedule (type);
}

EXPORT_SYMBOL unsigned int
hev_task_sleep (unsigned int milliseconds)
{
    return hev_task_usleep (milliseconds * 1000) / 1000;
}

EXPORT_SYMBOL unsigned int
hev_task_usleep (unsigned int microseconds)
{
    HevTaskSystemContext *ctx;

    if (microseconds == 0)
        return 0;

    ctx = hev_task_system_get_context ();
    return hev_task_timer_wait (ctx->timer, microseconds, ctx->current_task);
}

EXPORT_SYMBOL void
hev_task_run (HevTask *self, HevTaskEntry entry, void *data)
{
    /* Skip to run task that already running */
    if (self->state != HEV_TASK_STOPPED)
        return;

    self->entry = entry;
    self->data = data;
    self->priority = self->next_priority;
    self->sched_key = self->next_priority;

    hev_task_system_run_new_task (self);
}

EXPORT_SYMBOL void
hev_task_exit (void)
{
    hev_task_system_kill_current_task ();
}

EXPORT_SYMBOL void *
hev_task_get_data (HevTask *self)
{
    return self->data;
}
