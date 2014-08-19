/*
 *   Copyright 2010 Gerhard Gappmeier <gerhard.gappmeier@ascolab.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "timer.h"

void timer_init(struct timer *t)
{
#if defined(__linux__) || defined(__QNX__)
    t->start.tv_sec = 0;
    t->start.tv_usec = 0;
    t->end.tv_sec = 0;
    t->end.tv_usec = 0;
#endif /* defined(__linux__) || defined(__QNX__) */
#ifdef _WIN32
    t->start.QuadPart = 0;
    t->end.QuadPart = 0;
#endif /* __WIN32 */
    t->time = 0;
}

void timer_cleanup(struct timer *t)
{
    timer_init(t);
}

/** starts timer measurement. */
void timer_start(struct timer *t)
{
#if defined(__linux__) || defined(__QNX__)
    gettimeofday(&t->start, 0);
#endif /* defined(__linux__) || defined(__QNX__) */
#ifdef _WIN32
    QueryPerformanceCounter(&t->start);
#endif /* __WIN32 */
}

/** stops timer measurement. */
void timer_stop(struct timer *t)
{
#if defined(__linux__) || defined(__QNX__)
    gettimeofday(&t->end, 0);
#endif /* defined(__linux__) || defined(__QNX__) */
#ifdef _WIN32
    QueryPerformanceCounter(&t->end);
#endif /* __WIN32 */
}

/** Computes the elapsed time.
 * @return Elapsed time in usecs.
 */
uint64_t timer_compute_time(struct timer *t)
{
    uint64_t diff = 0;
#if defined(__linux__) || defined(__QNX__)
    uint64_t start = 0;
    uint64_t end = 0;
    start = t->start.tv_sec * 1000000;
    start += t->start.tv_usec;
    end = t->end.tv_sec * 1000000;
    end += t->end.tv_usec;
    diff = end - start;
    t->time = diff;
#endif /* defined(__linux__) || defined(__QNX__) */
#ifdef _WIN32
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);   /* 1/s = Hz */
    diff = t->end.QuadPart - t->start.QuadPart;
    /* normalize to usec */
    diff /= (freq.QuadPart / 1000000);
    t->time = diff;
#endif /* __WIN32 */
    return diff;
}

/** Returns the elapsed time that has been computed before
 * using timer_compute_time.
 * This way you can access the stored computed time without
 * computing it again.
 * @return Elapsed time in usecs.
 */
uint64_t timer_get_time(struct timer *t)
{
    return t->time;
}

