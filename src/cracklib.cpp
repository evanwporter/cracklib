#include "../include/cracklib.hpp"

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

constexpr char CHARSET[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr size_t CHARSET_LEN = sizeof(CHARSET) - 1;
char CHARSET_MAP[128];

constexpr int SAVE_INTERVAL = 100;
constexpr int PRINT_INTERVAL = 10;

std::mutex mtx;
bool password_found = false;

void init_charset_map() {
    memset(CHARSET_MAP, -1, sizeof(CHARSET_MAP));
    for (size_t i = 0; i < CHARSET_LEN; ++i) {
        CHARSET_MAP[CHARSET[i]] = i;
    }
}

bool next_password(char* password, size_t& length) {
    size_t i = length - 1;
    while (true) {
        char& c = password[i];
        int index = CHARSET_MAP[(unsigned char)c];

        if (index + 1 < CHARSET_LEN) {
            c = CHARSET[index + 1];
            return true;
        }
        c = CHARSET[0];

        if (i == 0) {
            if (length < 63) {
                memmove(password + 1, password, length);
                password[0] = CHARSET[0];
                length++;
                password[length] = '\0';
                return true;
            }
            else {
                return false;
            }
        }
        i--;
    }
}

void save_progress(const std::string& progress_file, const char* password) {
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream file(progress_file, std::ios::trunc);
    if (file) {
        file << password;
    }
}

bool test_password(const std::string& rar_file, const char* password) {
    std::string command = "unrar t -p";
    command += password;
    command += " \"" + rar_file + "\" 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        if (strstr(buffer, "OK")) {
            pclose(pipe);
            return true;
        }
    }
    pclose(pipe);
    return false;
}

void brute_force_thread(const std::string& rar_file, size_t thread_id, size_t total_threads) {
    char password[64] = { CHARSET[thread_id % CHARSET_LEN], '\0' };
    size_t length = 1;
    int attempts = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto last_print_time = start_time;

    while (!password_found) {
        if (test_password(rar_file, password)) {
            std::lock_guard<std::mutex> lock(mtx);
            password_found = true;
            std::cout << "\n[THREAD " << thread_id << "] Password found: " << password << std::endl;
            save_progress(rar_file + ".txt", password);
            return;
        }

        attempts++;
        next_password(password, length);

        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_print_time).count();

        if (attempts % SAVE_INTERVAL == 0) {
            save_progress(rar_file + ".txt", password);
        }

        if (elapsed_time >= PRINT_INTERVAL) {
            double total_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            double cps = attempts / (total_time > 0 ? total_time : 1);

            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "[THREAD " << thread_id << "] Attempt: " << password
                << " | Attempts: " << attempts
                << " | CPS: " << cps << " checks/sec" << std::endl;

            last_print_time = current_time;
        }
    }
}

void crack(const std::string& rar_file, int thread_count) {
    std::cout << "Using " << thread_count << " threads.\n";

    init_charset_map();

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(brute_force_thread, rar_file, i, thread_count);
    }

    for (auto& th : threads) {
        th.join();
    }
}
