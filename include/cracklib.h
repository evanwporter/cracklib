/*
 *   Copyright (C) 2007 by David Zoltan Kedves
 *   kedazo@gmail.com
 *
 *   Modified by Evan William Porter in 2025
 *   evanwporter@gmail.com
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef CRACKLIB_H
#define CRACKLIB_H

#include <pthread.h>
#define _GNU_SOURCE

#define PWD_LEN 100
#define SAVE_INTERVAL 10000

typedef struct {
    char ABC[128];
    char password[PWD_LEN + 1];
    char password_good[PWD_LEN + 1];
    unsigned int curr_len;
    long counter;
    int finished;
} CrackStatus;

extern char default_ABC[];
extern char filename[255];
extern char statname[259];
extern int num_threads;
extern pthread_mutex_t pwdMutex;
extern pthread_mutex_t finishedMutex;

void crack(const char* file, int threads);
void save_status();
int load_status();
char* nextpass();
void* crack_thread();
void* status_thread();
void status_to_json(const char* file);

#endif
