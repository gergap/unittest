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
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#if defined(__linux__) || defined(__QNX__)
# include <sys/time.h>
#endif /* defined(__linux__) || defined(__QNX__) */
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif /* _WIN32 */

#define TIMER_STATIC_INITIALIZER {{0, 0}, {0, 0}, 0}

struct timer {
#if defined(__linux__) || defined(__QNX__)
    struct timeval start;
    struct timeval end;
#endif /* defined(__linux__) || defined(__QNX__) */
#ifdef _WIN32
    LARGE_INTEGER start;
    LARGE_INTEGER end;
#endif /* _WIN32 */
    uint64_t time;
};

void timer_init(struct timer *t);
void timer_cleanup(struct timer *t);
void timer_start(struct timer *t);
void timer_stop(struct timer *t);
uint64_t timer_compute_time(struct timer *t);
uint64_t timer_get_time(struct timer *t);

#endif /* __TIMER_H__ */

