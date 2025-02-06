#include "../include/cracklib.hpp"
#include <string>
#include <iostream>
#include <thread>

int parse_arguments(int argc, char* argv[], std::string& rar_file, int& thread_count) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <RARFILE>.rar [--threads #]\n";
        return 1;
    }

    rar_file = argv[1];
    thread_count = std::thread::hardware_concurrency();  // Default

    if (argc == 4) {
        std::string arg = argv[2];
        if (arg == "--threads") {
            try {
                thread_count = std::stoi(argv[3]);
                if (thread_count < 1) {
                    throw std::invalid_argument("Thread count must be >= 1");
                }
            }
            catch (...) {
                std::cerr << "Invalid thread count value.\n";
                return 1;
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    std::string rar_file;
    int thread_count;

    if (parse_arguments(argc, argv, rar_file, thread_count)) {
        return 1;
    }

    crack(rar_file, thread_count);
    return 0;
}
