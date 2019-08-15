/*
 ============================================================================
 Name        : hev-task-fd-event.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description :
 ============================================================================
 */

#ifndef __HEV_TASK_FD_EVENT_H__
#define __HEV_TASK_FD_EVENT_H__

#if defined(__linux__)
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HevTaskFDEvent HevTaskFDEvent;

struct _HevTaskFDEvent
{
#if defined(__linux__)
    struct epoll_event event;
#else
    struct kevent event;
#endif
};

/**
 * hev_task_fd_event_events:
 * @self: a #HevTaskFDEvent
 *
 * Get events of @self.
 *
 * Returns: a poll events. (e.g. POLLIN, POLLOUT)
 *
 * Since: 4.8
 */
unsigned int hev_task_fd_event_events (HevTaskFDEvent *self);

/**
 * hev_task_fd_event_data:
 * @self: a #HevTaskFDEvent
 *
 * Get data of @self.
 *
 * Returns: a data.
 *
 * Since: 4.8
 */
void *hev_task_fd_event_data (HevTaskFDEvent *self);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_TASK_FD_EVENT_H__ */
