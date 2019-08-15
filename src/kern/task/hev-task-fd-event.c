/*
 ============================================================================
 Name        : hev-task-fd-event.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description :
 ============================================================================
 */

#include "lib/utils/hev-compiler.h"
#include "kern/io/hev-task-io-reactor.h"

#include "hev-task-fd-event.h"

EXPORT_SYMBOL unsigned int
hev_task_fd_event_events (HevTaskFDEvent *self)
{
    return hev_task_io_reactor_wait_event_get_events (&self->event);
}

EXPORT_SYMBOL void *
hev_task_fd_event_data (HevTaskFDEvent *self)
{
    return hev_task_io_reactor_wait_event_get_data (&self->event);
}
