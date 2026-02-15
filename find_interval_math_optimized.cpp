#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <algorithm>
#include <set>

using namespace std;
using namespace chrono;

class OptimizedPrimeFinder {
private:
    static constexpr int LENGTH = 2004;
    static constexpr int TARGET = 12;
    static constexpr unsigned long long SEARCH_RADIUS = 1000000ULL; // ±1M around center
    static constexpr unsigned long long MAX_LIMIT = 100000000000000ULL; // 10^14

    vector<int> small_primes_all;     // primes up to 10^7
    vector<int> small_primes_100;     // first 100 primes for fast pre-sieve
    mutex io_mutex;
    
    atomic<bool> found{false};
    atomic<unsigned long long> solution_start{0};
    atomic<unsigned long long> total_fully_checked{0};
    high_resolution_clock::time_point program_start;

    // Generate small primes up to 10^7
    void generate_primes() {
        const int LIMIT = 10000000;
        vector<char> is_prime(LIMIT + 1, true);
        is_prime[0] = is_prime[1] = false;
        for (int i = 2; i <= LIMIT; ++i) {
            if (is_prime[i]) {
                small_primes_all.push_back(i);
                if (static_cast<long long>(i) * i <= LIMIT) {
                    for (long long j = static_cast<long long>(i) * i; j <= LIMIT; j += i)
                        is_prime[j] = false;
                }
            }
        }
        small_primes_100.assign(small_primes_all.begin(), 
                               small_primes_all.begin() + min(100, (int)small_primes_all.size()));
        
        lock_guard<mutex> lock(io_mutex);
        cout << "✓ Generated " << small_primes_all.size() << " small primes (up to 10^7)" << endl;
    }

    // Fast pre-sieve: skip windows with too many/few survivors
    bool fast_presieve(unsigned long long start) {
        static thread_local vector<char> marked(LENGTH, false);
        fill(marked.begin(), marked.end(), false);
        int covered = 0;
        
        for (int p : small_primes_100) {
            if (p >= LENGTH) break;
            // First multiple of p >= start
            unsigned long long first = ((start + p - 1ULL) / p) * p;
            for (unsigned long long x = first; x < start + LENGTH; x += p) {
                int idx = static_cast<int>(x - start);
                if (idx >= 0 && idx < LENGTH && !marked[idx]) {
                    marked[idx] = true;
                    if (++covered > LENGTH - 8) return false; // survivors < 8 → impossible
                }
            }
        }
        int survivors = LENGTH - covered;
        return (survivors >= 10 && survivors <= 30); // plausible range for 12 primes
    }

    // Full segmented sieve for one window
    int count_primes_in_window(unsigned long long start) {
        const unsigned long long end = start + LENGTH - 1;
        vector<char> seg(LENGTH, true);
        if (start == 1) seg[0] = false;

        unsigned long long sqrt_end = static_cast<unsigned long long>(sqrt(end)) + 1;
        for (int p : small_primes_all) {
            if (static_cast<unsigned long long>(p) > sqrt_end) break;
            unsigned long long p_sq = static_cast<unsigned long long>(p) * p;
            unsigned long long first = (start + p - 1ULL) / p * p;
            if (first < p_sq) first = p_sq;
            if (first > end) continue;
            
            for (unsigned long long x = first; x <= end; x += p) {
                seg[x - start] = false;
            }
        }

        int count = 0;
        for (int i = 0; i < LENGTH; ++i) {
            if (seg[i]) count++;
        }
        return count;
    }

    // Search in [center - R, center + R], bounded by [1, MAX_LIMIT]
    void search_around_center(unsigned long long center, int id) {
        unsigned long long low = (center > SEARCH_RADIUS) ? center - SEARCH_RADIUS : 1ULL;
        unsigned long long high = min(center + SEARCH_RADIUS, MAX_LIMIT);
        unsigned long long effective_end = (high >= LENGTH) ? high - LENGTH + 1 : 0;

        if (low > effective_end) return;

        for (unsigned long long n = low; n <= effective_end && !found.load(); ++n) {
            if (!fast_presieve(n)) continue; // Skip 99.9% of windows
            
            int cnt = count_primes_in_window(n);
            total_fully_checked.fetch_add(1, memory_order_relaxed);
            
            if (cnt == TARGET) {
                if (!found.exchange(true)) {
                    solution_start.store(n);
                    auto end_time = high_resolution_clock::now();
                    auto ms = duration_cast<milliseconds>(end_time - program_start).count();
                    
                    lock_guard<mutex> lock(io_mutex);
                    cout << "\n=========================================" << endl;
                    cout << "✓✓✓ SOLUTION FOUND BY THREAD #" << id << " ✓✓✓" << endl;
                    cout << "N = " << n << endl;
                    cout << "Interval: [" << n << ", " << n + LENGTH - 1 << "]" << endl;
                    cout << "Searched up to " << n/1e9 << "B" << endl;
                    cout << "Time: " << ms/1000.0 << "s | Fully checked: " << total_fully_checked.load() << endl;
                    cout << "=========================================" << endl;
                }
                return;
            }
        }
    }

public:
    void find() {
        cout << "=========================================" << endl;
        cout << "SMART SEARCH FOR 2004-LENGTH INTERVAL WITH 12 PRIMES" << endl;
        cout << "Range: up to " << MAX_LIMIT/1e12 << "T (10^14)" << endl;
        cout << "Strategy: Search near primorials and heuristic sparse centers" << endl;
        cout << "Fast pre-sieve skips ~99.9% of windows" << endl;
        cout << "=========================================" << endl << endl;

        program_start = high_resolution_clock::now();
        generate_primes();

        set<unsigned long long> centers;

        // 1. Primorials (p# for p = 50, 100, 150) — capped at 1e14
        vector<int> primorial_bases = {50, 100, 150};
        for (int p_max : primorial_bases) {
            __int128 primorial = 1;
            bool overflow = false;
            for (int p : small_primes_all) {
                if (p > p_max) break;
                if (primorial > (__int128)MAX_LIMIT / p) {
                    overflow = true;
                    break;
                }
                primorial *= p;
            }
            if (!overflow && primorial <= (__int128)MAX_LIMIT) {
                centers.insert(static_cast<unsigned long long>(primorial));
            }
        }

        // 2. Heuristic sparse centers (within 10^14)
        vector<unsigned long long> heuristic = {
            1000000000000ULL,      // 1e12
            5000000000000ULL,      // 5e12
            10000000000000ULL,     // 1e13
            50000000000000ULL,     // 5e13
            100000000000000ULL     // 1e14
        };
        for (auto c : heuristic) {
            if (c <= MAX_LIMIT) centers.insert(c);
        }

        // Output centers
        cout << "Generated " << centers.size() << " candidate centers:" << endl;
        for (auto c : centers) {
            cout << "  " << c << " (" << c/1e9 << "B)" << endl;
        }
        cout << endl;

        // Launch threads
        vector<thread> threads;
        int id = 0;
        for (unsigned long long center : centers) {
            threads.emplace_back([this, center, id]() {
                search_around_center(center, id);
            });
            id++;
        }

        // Wait for all
        for (auto& t : threads) t.join();

        if (!found.load()) {
            lock_guard<mutex> lock(io_mutex);
            cout << "\n✗ No solution found in sparse regions up to " << MAX_LIMIT/1e12 << "T." << endl;
            cout << "Total windows fully checked: " << total_fully_checked.load() << endl;
            cout << "Note: Solution likely requires N >> 10^14 (expectation ~10^72)." << endl;
        }
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    
    try {
        OptimizedPrimeFinder finder;
        finder.find();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
