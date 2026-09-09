/*
 * Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test-only LD_PRELOAD probe: reports who owns SIGSEGV, and when it is blocked.
 *
 * RV intermittently dies of SIGSEGV during startup on Rocky 8 while Crashpad's
 * handler is installed and yet writes no dump and logs nothing. Two things
 * would produce that silence: something replaced the SIGSEGV disposition after
 * Crashpad set it, or SIGSEGV was blocked in the crashing thread, in which case
 * the kernel forces the default action and no handler runs at all.
 *
 * gdb cannot answer this: attaching makes the failure disappear. So instead of
 * tracing, interpose the four calls that can change either state and log every
 * transition. The cost is a write() on calls that touch SIGSEGV, so timing
 * stays close to native.
 *
 * Output goes to stderr, which the smoke test captures. Mask changes are logged
 * only when the blocked state actually flips, per thread, to keep the volume
 * low enough to survive the log tail.
 */
#define _GNU_SOURCE

#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/syscall.h>
#endif

typedef int (*sigaction_fn)(int, const struct sigaction*, struct sigaction*);
typedef void (*sighandler_t_)(int);
typedef sighandler_t_ (*signal_fn)(int, sighandler_t_);
typedef int (*sigprocmask_fn)(int, const sigset_t*, sigset_t*);
typedef int (*pthread_sigmask_fn)(int, const sigset_t*, sigset_t*);

static sigaction_fn real_sigaction;
static signal_fn real_signal;
static sigprocmask_fn real_sigprocmask;
static pthread_sigmask_fn real_pthread_sigmask;

typedef int (*pthread_create_fn)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
static pthread_create_fn real_pthread_create;

/* -1 means "not yet sampled on this thread". */
static __thread int last_blocked = -1;

static void resolve_real_symbols(void)
{
    if (!real_sigaction)
    {
        real_sigaction = (sigaction_fn)dlsym(RTLD_NEXT, "sigaction");
    }
    if (!real_signal)
    {
        real_signal = (signal_fn)dlsym(RTLD_NEXT, "signal");
    }
    if (!real_sigprocmask)
    {
        real_sigprocmask = (sigprocmask_fn)dlsym(RTLD_NEXT, "sigprocmask");
    }
    if (!real_pthread_sigmask)
    {
        real_pthread_sigmask = (pthread_sigmask_fn)dlsym(RTLD_NEXT, "pthread_sigmask");
    }
    if (!real_pthread_create)
    {
        real_pthread_create = (pthread_create_fn)dlsym(RTLD_NEXT, "pthread_create");
    }
}

static long probe_tid(void)
{
#if defined(__linux__) && defined(SYS_gettid)
    return (long)syscall(SYS_gettid);
#else
    return (long)(size_t)pthread_self();
#endif
}

static void probe_log(const char* fmt, ...)
{
    char buf[512];
    va_list ap;

    int n = snprintf(buf, sizeof(buf), "[sigprobe pid=%d tid=%ld] ", (int)getpid(), probe_tid());
    if (n < 0 || (size_t)n >= sizeof(buf) - 2)
    {
        return;
    }

    va_start(ap, fmt);
    int m = vsnprintf(buf + n, sizeof(buf) - (size_t)n - 2, fmt, ap);
    va_end(ap);
    if (m < 0)
    {
        return;
    }

    size_t len = (size_t)n + (size_t)m;
    if (len > sizeof(buf) - 2)
    {
        len = sizeof(buf) - 2;
    }
    buf[len++] = '\n';

    ssize_t written = write(STDERR_FILENO, buf, len);
    (void)written;
}

static const char* handler_name(void* handler)
{
    if (handler == (void*)SIG_DFL)
    {
        return " (SIG_DFL)";
    }
    if (handler == (void*)SIG_IGN)
    {
        return " (SIG_IGN)";
    }
    return "";
}

/*
 * Report the calling thread's current SIGSEGV blocked state, but only when it
 * changed. Queries the real mask rather than trusting the requested change, so
 * SIG_SETMASK and nested block/unblock pairs are all accounted for correctly.
 */
static void report_mask_state(const char* who)
{
    sigset_t current;

    if (!real_pthread_sigmask)
    {
        real_pthread_sigmask = (pthread_sigmask_fn)dlsym(RTLD_NEXT, "pthread_sigmask");
    }
    if (!real_pthread_sigmask || real_pthread_sigmask(SIG_BLOCK, NULL, &current) != 0)
    {
        return;
    }

    int blocked = (sigismember(&current, SIGSEGV) == 1);
    if (blocked == last_blocked)
    {
        return;
    }

    int first_sample = (last_blocked == -1);
    last_blocked = blocked;
    if (first_sample && !blocked)
    {
        return;
    }
    probe_log("%s -> SIGSEGV %s", who, blocked ? "BLOCKED" : "unblocked");
}

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact)
{
    if (!real_sigaction)
    {
        real_sigaction = (sigaction_fn)dlsym(RTLD_NEXT, "sigaction");
    }

    int rc = real_sigaction(signum, act, oldact);

    if (signum == SIGSEGV && act != NULL)
    {
        void* handler = (act->sa_flags & SA_SIGINFO) ? (void*)act->sa_sigaction : (void*)act->sa_handler;
        probe_log("sigaction(SIGSEGV) handler=%p%s flags=0x%x rc=%d", handler, handler_name(handler), act->sa_flags, rc);
    }
    return rc;
}

sighandler_t_ signal(int signum, sighandler_t_ handler)
{
    if (!real_signal)
    {
        real_signal = (signal_fn)dlsym(RTLD_NEXT, "signal");
    }

    sighandler_t_ previous = real_signal(signum, handler);

    if (signum == SIGSEGV)
    {
        probe_log("signal(SIGSEGV) handler=%p%s previous=%p", (void*)handler, handler_name((void*)handler), (void*)previous);
    }
    return previous;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oldset)
{
    if (!real_sigprocmask)
    {
        real_sigprocmask = (sigprocmask_fn)dlsym(RTLD_NEXT, "sigprocmask");
    }

    int rc = real_sigprocmask(how, set, oldset);
    if (set != NULL)
    {
        report_mask_state("sigprocmask");
    }
    return rc;
}

int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset)
{
    if (!real_pthread_sigmask)
    {
        real_pthread_sigmask = (pthread_sigmask_fn)dlsym(RTLD_NEXT, "pthread_sigmask");
    }

    int rc = real_pthread_sigmask(how, set, oldset);
    if (set != NULL)
    {
        report_mask_state("pthread_sigmask");
    }
    return rc;
}

struct thread_start_context
{
    void* (*entry)(void*);
    void* arg;
};

static void* thread_start_wrapper(void* raw)
{
    struct thread_start_context context = *(struct thread_start_context*)raw;
    sigset_t current;

    free(raw);

    if (real_pthread_sigmask != NULL && real_pthread_sigmask(SIG_BLOCK, NULL, &current) == 0)
    {
        last_blocked = (sigismember(&current, SIGSEGV) == 1);
        if (last_blocked)
        {
            probe_log("thread start -> SIGSEGV BLOCKED (inherited)");
        }
    }
    return context.entry(context.arg);
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*entry)(void*), void* arg)
{
    struct thread_start_context* context;

    if (!real_pthread_create)
    {
        resolve_real_symbols();
    }

    context = (struct thread_start_context*)malloc(sizeof(*context));
    if (context == NULL)
    {
        return real_pthread_create(thread, attr, entry, arg);
    }
    context->entry = entry;
    context->arg = arg;

    int rc = real_pthread_create(thread, attr, thread_start_wrapper, context);
    if (rc != 0)
    {
        free(context);
    }
    return rc;
}

/*
 * Confirm the probe actually loaded, but only for the process under test: the
 * tcsh wrapper also runs readlink, python, openssl, grep and cut, and every one
 * of them would otherwise announce itself.
 */
__attribute__((constructor)) static void sigprobe_init(void)
{
    char comm[64] = {0};

    resolve_real_symbols();

    FILE* fh = fopen("/proc/self/comm", "r");

    if (fh == NULL)
    {
        return;
    }
    if (fgets(comm, sizeof(comm), fh) != NULL && strstr(comm, "rv.bin") != NULL)
    {
        probe_log("loaded");
    }
    fclose(fh);
}
