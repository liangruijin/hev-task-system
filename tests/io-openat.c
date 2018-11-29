/*
 ============================================================================
 Name        : io-openat.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : IO OpenAT Test
 ============================================================================
 */

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hev-task.h>
#include <hev-task-system.h>
#include <hev-task-io.h>

static void
task_entry (void *data)
{
    int fd;
    char cwd[1024];
    const char *path = ".task-system";

    assert (getcwd (cwd, 1024) == cwd);

    assert (chdir ("/dev") == 0);
    fd = hev_task_io_openat (AT_FDCWD, "zero", O_RDWR);
    assert (fd >= 0);
    assert ((fcntl (fd, F_GETFL) & O_NONBLOCK) == O_NONBLOCK);
    close (fd);

    assert (chdir ("/tmp") == 0);
    unlink (path);
    fd = hev_task_io_openat (AT_FDCWD, path, O_RDWR | O_CREAT, 0644);
    assert (fd >= 0);
    assert ((fcntl (fd, F_GETFL) & O_NONBLOCK) == O_NONBLOCK);
    close (fd);
    assert (unlink (path) == 0);

    assert (chdir (cwd) == 0);
}

int
main (int argc, char *argv[])
{
    HevTask *task;

    assert (hev_task_system_init () == 0);

    task = hev_task_new (-1);
    assert (task);
    hev_task_run (task, task_entry, NULL);

    hev_task_system_run ();

    hev_task_system_fini ();

    return 0;
}
