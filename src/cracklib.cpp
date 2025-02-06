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

constexpr int SAVE_INTERVAL = 100; // attempts
constexpr int PRINT_INTERVAL = 5; // seconds

std::atomic<bool> password_found{ false };

void save_progress(const std::string& progress_file, const char* password) {
    std::ofstream file(progress_file, std::ios::trunc);
    if (file) {
        file << password;
    }
}

bool load_progress(const std::string& progress_file, char* password, size_t& length) {
    std::ifstream file(progress_file);
    if (file) {
        file.getline(password, 64);
        length = std::strlen(password);
        return length > 0;
    }
    return false;
}

bool test_password(const std::string& rar_file, const char* password) {
    std::string command = "unrar t -p" + std::string(password) + " \"" + rar_file + "\" 2>&1";
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

bool next_password(char* password, size_t& length) {
    size_t i = length - 1;
    while (true) {
        char& c = password[i];
        auto index = strchr(CHARSET, c) - CHARSET;

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

void brute_force_thread(const std::string& rar_file, size_t thread_id, size_t total_threads) {
    char password[64] = { CHARSET[thread_id % CHARSET_LEN], '\0' };
    size_t length = 1;
    int attempts = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto last_print_time = start_time;

    std::string progress_file = rar_file + "_progress_" + std::to_string(thread_id) + ".txt";

    // Load progress if available
    if (load_progress(progress_file, password, length)) {
        std::cout << "[THREAD " << thread_id << "] Resuming from: " << password << "\n";
    }

    std::string log_buffer;
    log_buffer.reserve(512);

    while (!password_found) {
        if (test_password(rar_file, password)) {
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
            save_progress(progress_file, password);
        }

        if (elapsed_time >= PRINT_INTERVAL) {
            double total_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            double cps = attempts / (total_time > 0 ? total_time : 1);

            log_buffer += "[THREAD " + std::to_string(thread_id) + "] Attempt: ";
            log_buffer += password;
            log_buffer += " | Attempts: " + std::to_string(attempts);
            log_buffer += " | CPS: " + std::to_string(cps) + " checks/sec\n";

            std::cout << log_buffer;
            log_buffer.clear();

            last_print_time = current_time;
        }
    }
}

void crack(const std::string& rar_file, int thread_count) {
    std::cout << "Using " << static_cast<int>(thread_count) << " threads.\n";

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(brute_force_thread, rar_file, i, thread_count);
    }

    for (auto& th : threads) {
        th.join();
    }
}
