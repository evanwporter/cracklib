#ifndef CRACKLIB_HPP
#define CRACKLIB_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstring>
#include <mutex>

void crack(const std::string& rar_file, int thread_count);

#endif // CRACKLIB_HPP
