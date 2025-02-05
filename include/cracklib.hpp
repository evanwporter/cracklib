/*
 *   Copyright (C) 2007 by David Zoltan Kedves
 *   kedazo@gmail.com
 *
 *   Modified by Evan Porter in 2025
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

#ifndef CRACKLIB_HPP
#define CRACKLIB_HPP

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

#include <mutex>
#include <atomic>

#define PWD_LEN 100

typedef struct {
  char ABC[128];                    // Character set used for cracking
  char password[PWD_LEN + 1];        // Current password attempt
  char password_good[PWD_LEN + 1];   // Successfully cracked password (if found)
  unsigned int curr_len;             // Current password length being tested
  std::atomic<long> counter;         // Number of passwords tested
  int finished;                      // Flag indicating if cracking is complete
} CrackStatus;

extern char default_ABC[];          // Default character set for password attempts
extern char filename[255];          // Target file name
extern char statname[259];          // Status file name
extern int num_threads;             // Number of threads for brute force

extern std::mutex pwdMutex;         // Mutex for password generation
extern std::mutex finishedMutex;    // Mutex for finished flag

void crack(const char* file, int threads);  // Main function to start cracking
void save_status();                         // Saves the cracking progress to a file
int load_status();                          // Loads the cracking progress from a file
char* nextpass();                           // Generates the next password attempt
void crack_thread();                        // Brute force thread function
void status_thread();                       // Periodically prints status and saves progress
void status_to_json(const char* file);      // Outputs cracking status in JSON format

#endif // CRACKLIB_HPP
