#pragma once
#include <omp.h>
#include <iostream>
#include <string>
#include <chrono>

class OpenMPManager {
public:
    int maxThreads  = 1;
    int activeThreads = 1;
    double lastFrameParallelMs = 0.0;

    void init(int threads = 4) {
        maxThreads = omp_get_max_threads();
        activeThreads = std::min(threads, maxThreads);
        omp_set_num_threads(activeThreads);
        std::cout << "[OpenMP] " << maxThreads << " max threads, using " << activeThreads << "\n";
    }

    void setThreads(int n) {
        activeThreads = std::max(1, std::min(n, maxThreads));
        omp_set_num_threads(activeThreads);
    }

    // Parallel reduction over a range — helper
    template<typename Fn>
    void parallelFor(int n, Fn fn) {
        auto t0 = std::chrono::high_resolution_clock::now();
        #pragma omp parallel for schedule(dynamic, 8) num_threads(activeThreads)
        for (int i = 0; i < n; i++) fn(i);
        auto t1 = std::chrono::high_resolution_clock::now();
        lastFrameParallelMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    std::string getStats() const {
        return "OMP:" + std::to_string(activeThreads) + "/" + std::to_string(maxThreads)
             + " (" + std::to_string((int)lastFrameParallelMs) + "ms)";
    }
};
