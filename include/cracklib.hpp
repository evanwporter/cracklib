#ifndef CRACKLIB_HPP
#define CRACKLIB_HPP

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <string>

void crack(const std::string& rar_file, int thread_count);

#endif // CRACKLIB_HPP
