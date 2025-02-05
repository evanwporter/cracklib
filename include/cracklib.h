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

#define _GNU_SOURCE

#include <stdio.h> //Standard headers
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> //POSIX threads

#include <stdint.h>

char default_ABC[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char CMD_DETECT[] = "file -i -b -L %s"; //this command return what is the file mime type

const char* TYPE[] = { "rar", "7z", "zip", "" }; //the last "" signing this is end of the list
const char* MIME[] = { "application/x-rar", "application/octet-stream", "application/x-zip", "" };
const char* CMD[] = { "unrar t -y -p%s %s 2>&1", "7z t -y -p%s %s 2>&1", "unzip -P%s -t %s 2>&1", "" };

#define PWD_LEN 100
#define SAVE_INTERVAL 10000  // Save after every 10,000 passwords

typedef struct {
    char ABC[128]; // Alphabet used for password generation
    char password[PWD_LEN + 1];  // Current password attempt
    char password_good[PWD_LEN + 1]; // Correct password if found
    unsigned int curr_len; // Length of current password
    long counter; // Number of tested passwords
    int finished; // 1 if password found, 0 otherwise
} CrackStatus;