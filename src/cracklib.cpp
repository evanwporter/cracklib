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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>

#include "../include/cracklib.hpp"

char default_ABC[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char filename[255];
char statname[259];
int num_threads = 2;

std::mutex pwdMutex;
std::mutex finishedMutex;
std::atomic<long> counter(0);  // Atomic counter for performance

CrackStatus crack_status;

void save_status() {
    FILE* file = fopen(statname, "wb");
    if (file) {
        std::lock_guard<std::mutex> lock(finishedMutex);
        fwrite(&crack_status, sizeof(CrackStatus), 1, file);
        fclose(file);
    }
}

int load_status() {
    FILE* file = fopen(statname, "rb");
    if (!file) return 0;

    fread(&crack_status, sizeof(CrackStatus), 1, file);
    fclose(file);

    if (crack_status.finished) {
        printf("GOOD: Archive successfully cracked\n");
        printf("      Password: '%s'\n", crack_status.password_good);
        return 1;
    }

    printf("INFO: Resuming cracking from password: '%s'\n", crack_status.password);
    return 0;
}

void nextpass2(char* p, unsigned int n) {
    size_t len = strlen(crack_status.ABC);
    char first_char = crack_status.ABC[0];

    while (n >= 0) {
        char* pos = strchr(crack_status.ABC, p[n]);
        if (pos && pos[1] != '\0') {
            p[n] = pos[1];
            return;
        }
        p[n] = first_char;
        if (n == 0) {
            memmove(p + 1, p, crack_status.curr_len);
            p[0] = first_char;
            crack_status.curr_len++;
            p[crack_status.curr_len] = '\0';
            return;
        }
        n--;
    }
}

char* nextpass() {
    std::lock_guard<std::mutex> lock(pwdMutex);

    char* pass = (char*)malloc(PWD_LEN + 1);
    if (!pass) return nullptr;

    strcpy(pass, crack_status.password);
    nextpass2(crack_status.password, crack_status.curr_len - 1);

    return pass;
}

void status_thread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        long pwds_per_sec = counter.exchange(0) / 3;  // Reset and get value atomically

        {
            std::lock_guard<std::mutex> lock(finishedMutex);
            if (crack_status.finished) break;
        }

        {
            std::lock_guard<std::mutex> lock(pwdMutex);
            printf("Probing: '%s' [%ld pwds/sec]\n", crack_status.password, pwds_per_sec);
        }

        save_status();
    }
}

void crack_thread() {
    char* current;
    char ret[200];
    char cmd[400];

    while (true) {
        current = nextpass();
        if (!current) break;

        snprintf(cmd, sizeof(cmd), "unrar %s -y -p%s \"%s\" 2>&1",
#ifdef _WIN32
            "exe",
#else
            "t",
#endif
            current, filename);

        FILE* Pipe = popen(cmd, "r");
        if (!Pipe) {
            free(current);
            continue;
        }

        while (fgets(ret, sizeof(ret), Pipe)) {
            if (strstr(ret, "ok")) {
                {
                    std::lock_guard<std::mutex> lock(finishedMutex);
                    strcpy(crack_status.password_good, current);
                    crack_status.finished = 1;
                    printf("GOOD: Password cracked: '%s'\n", current);
                }

                save_status();
                pclose(Pipe);
                free(current);
                return;
            }
            }

        pclose(Pipe);
        counter++;

        {
            std::lock_guard<std::mutex> lock(finishedMutex);
            if (crack_status.finished) break;
        }

        free(current);
        }
    }

void crack(const char* file, int threads) {
    strncpy(filename, file, sizeof(filename) - 1);
    snprintf(statname, sizeof(statname), "%s.bin", filename);
    num_threads = threads;

    if (load_status()) return;

    strcpy(crack_status.ABC, default_ABC);
    crack_status.curr_len = 1;
    crack_status.finished = 0;
    strcpy(crack_status.password, default_ABC);

    std::vector<std::thread> thread_pool;

    for (int i = 0; i < num_threads; i++) {
        thread_pool.emplace_back(crack_thread);
    }

    std::thread status_thread_obj(status_thread);

    for (auto& th : thread_pool) {
        th.join();
    }

    status_thread_obj.join();
}

void status_to_json(const char* file) {
    char status_file[259];
    snprintf(status_file, sizeof(status_file), "%s.bin", file);

    FILE* fp = fopen(status_file, "rb");
    if (!fp) {
        printf("{\"error\": \"Status file not found.\"}\n");
        return;
    }

    CrackStatus status;
    if (fread(&status, sizeof(CrackStatus), 1, fp) != 1) {
        printf("{\"error\": \"Failed to read status file.\"}\n");
        fclose(fp);
        return;
    }

    fclose(fp);

    char json_file[259];
    snprintf(json_file, sizeof(json_file), "%s.json", file);

    FILE* json_fp = fopen(json_file, "w");
    if (!json_fp) {
        printf("{\"error\": \"Failed to write JSON file.\"}\n");
        return;
    }

    fprintf(json_fp, "{\n");
    fprintf(json_fp, "  \"ABC\": \"%s\",\n", status.ABC);
    fprintf(json_fp, "  \"current_password\": \"%s\",\n", status.password);
    fprintf(json_fp, "  \"password_good\": \"%s\",\n", status.password_good);
    fprintf(json_fp, "  \"current_length\": %u,\n", status.curr_len);
    fprintf(json_fp, "  \"tested_count\": %ld,\n", status.counter.load());
    fprintf(json_fp, "  \"finished\": %d\n", status.finished);
    fprintf(json_fp, "}\n");

    fclose(json_fp);

    printf("JSON status saved to: %s\n", json_file);
}

