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
    static constexpr long long SEARCH_RADIUS = 1000000; // ±1M around sparse centers

    vector<int> small_primes_all;     // up to 10^7
    vector<int> small_primes_100;     // first 100 primes (for fast pre-sieve)
    mutex io_mutex;
    
    atomic<bool> found{false};
    atomic<long long> solution_start{0};
    atomic<long long> total_checked{0};
    high_resolution_clock::time_point program_start;

    // 生成小质数
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
        // 取前100个用于预筛
        small_primes_100.assign(small_primes_all.begin(), 
                               small_primes_all.begin() + min(100, (int)small_primes_all.size()));
        
        lock_guard<mutex> lock(io_mutex);
        cout << "✓ Primes: " << small_primes_all.size() << " (all), " 
             << small_primes_100.size() << " (fast)" << endl;
    }

    // 快速预筛：返回是否值得完整检查
    bool fast_presieve(long long start) {
        static thread_local vector<char> marked(LENGTH, false);
        fill(marked.begin(), marked.end(), false);
        int covered = 0;
        
        for (int p : small_primes_100) {
            if (p > LENGTH) break;
            long long first = ((start + p - 1) / p) * p;
            for (long long x = first; x < start + LENGTH; x += p) {
                int idx = x - start;
                if (idx >= 0 && idx < LENGTH && !marked[idx]) {
                    marked[idx] = true;
                    if (++covered > LENGTH - 8) return false; // 幸存者 < 8
                }
            }
        }
        int survivors = LENGTH - covered;
        return (survivors >= 10 && survivors <= 30); // 启发式窗口
    }

    // 完整分段筛（同前，略作简化）
    int count_primes_in_window(long long start) {
        const long long end = start + LENGTH - 1;
        vector<char> seg(LENGTH, true);
        if (start == 1) seg[0] = false;

        long long sqrt_end = sqrt(end) + 1;
        for (int p : small_primes_all) {
            if (p > sqrt_end) break;
            long long first = max(static_cast<long long>(p)*p, 
                                 ((start + p - 1)/p) * static_cast<long long>(p));
            for (long long x = first; x <= end; x += p) {
                if (x >= start) seg[x - start] = false;
            }
        }

        int count = 0;
        for (int i = 0; i < LENGTH; ++i) {
            if (seg[i]) count++;
        }
        return count;
    }

    // 在 [center - R, center + R] 搜索
    void search_around_center(long long center, int id) {
        long long low = max(center - SEARCH_RADIUS, 1LL);
        long long high = center + SEARCH_RADIUS;
        
        for (long long n = low; n <= high - LENGTH + 1 && !found.load(); ++n) {
            if (!fast_presieve(n)) continue; // 跳过99.9%
            
            int cnt = count_primes_in_window(n);
            total_checked.fetch_add(1, memory_order_relaxed);
            
            if (cnt == TARGET) {
                if (!found.exchange(true)) {
                    solution_start.store(n);
                    auto end_time = high_resolution_clock::now();
                    auto ms = duration_cast<milliseconds>(end_time - program_start).count();
                    
                    lock_guard<mutex> lock(io_mutex);
                    cout << "\n✓ SOLUTION FOUND near " << center << " by thread " << id << endl;
                    cout << "N = " << n << " | Time: " << ms/1000.0 << "s" << endl;
                }
                return;
            }
        }
    }

    // 计算 primorial(p#)
    long long compute_primorial(int p_max) {
        __int128 primorial = 1;
        for (int p : small_primes_all) {
            if (p > p_max) break;
            primorial *= p;
            if (primorial > (__int128)1e18) break; // 防溢出
        }
        return (long long)min(primorial, (__int128)1e18);
    }

public:
    void find() {
        cout << "=========================================" << endl;
        cout << "SMART SEARCH USING MATHEMATICAL SPARSITY" << endl;
        cout << "Strategy: Search near primorials, factorials, and known gaps" << endl;
        cout << "Fast pre-sieve skips 99.9% of windows" << endl;
        cout << "=========================================" << endl << endl;

        program_start = high_resolution_clock::now();
        generate_primes();

        // 生成候选中心点
        set<long long> centers;
        
        // 1. Primorials
        vector<int> primorial_bases = {100, 200, 500, 1000};
        for (int p : primorial_bases) {
            long long prim = compute_primorial(p);
            if (prim > 1e12) centers.insert(prim);
        }
        
        // 2. Factorials (approximate via log)
        vector<int> factorial_ns = {100, 150, 200};
        for (int n : factorial_ns) {
            double log_fact = 0;
            for (int i = 2; i <= n; ++i) log_fact += log(i);
            if (log_fact > 30) { // > e^30 ~ 1e13
                long long fact_approx = (long long)exp(min(log_fact, 40.0)); // cap at e^40
                centers.insert(fact_approx);
            }
        }
        
        // 3. Known large gaps (example centers)
        vector<long long> known_gaps = {
            18361375334787046697LL, // record gap ~1550
            1000000000000000000LL,
            10000000000000000000LL
        };
        for (auto g : known_gaps) centers.insert(g);

        cout << "Generated " << centers.size() << " candidate centers:" << endl;
        for (auto c : centers) {
            cout << "  " << c << " (" << c/1e9 << "B)" << endl;
        }
        cout << endl;

        // 多线程搜索
        vector<thread> threads;
        int id = 0;
        for (long long center : centers) {
            threads.emplace_back([this, center, id]() {
                search_around_center(center, id);
            });
            id++;
        }

        // 等待
        for (auto& t : threads) t.join();

        if (!found.load()) {
            cout << "\n✗ No solution found in sparse regions." << endl;
            cout << "Total windows fully checked: " << total_checked.load() << endl;
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
    }
    return 0;
}
