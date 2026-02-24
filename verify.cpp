#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <algorithm>
#include <primesieve.hpp>

const uint64_t CANDIDATE = 55350776431902878ULL;
const uint64_t INTERVAL_LEN = 2004;
const size_t WINDOW_SIZE = 14;  // 需要14个连续素数

struct GapRecord {
    uint64_t g;
    uint64_t p;      // P(g)
    uint64_t n;      // N(g)
    std::string finder;
};

std::vector<GapRecord> read_gaps(const std::string& filename) {
    std::vector<GapRecord> gaps;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return gaps;
    }
    std::string line;
    std::getline(file, line); // 跳过表头
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '-') continue;
        std::istringstream iss(line);
        uint64_t g, p, n;
        std::string finder;
        if (iss >> g >> p >> n) {
            std::getline(iss, finder);
            finder.erase(0, finder.find_first_not_of(" \t"));
            if (p < CANDIDATE) {
                gaps.push_back({g, p, n, finder});
            }
        }
    }
    return gaps;
}

// 检查以 start_prime 开头的连续12个素数（间隙前的情况）
bool check_forward(uint64_t start_prime, uint64_t p0) {
    primesieve::iterator it;
    it.jump_to(start_prime);
    uint64_t p1 = it.next_prime();
    if (p1 != start_prime) return false;

    std::deque<uint64_t> window;
    window.push_back(p1);
    for (size_t i = 1; i < WINDOW_SIZE; ++i) {
        window.push_back(it.next_prime());
    }
    uint64_t p12 = window[11];
    uint64_t p13 = window[12];
    uint64_t p14 = window[13];

    if (p12 - p1 <= INTERVAL_LEN - 1) {
        uint64_t L_min = std::max(p0 + 1, p12 - (INTERVAL_LEN - 1));
        uint64_t L_max = std::min(p1, p14 - INTERVAL_LEN);
        if (L_min <= L_max && L_min < CANDIDATE) {
            std::cout << "发现更小解 n = " << L_min
                      << " (来自前向检查，p1=" << p1 << ")" << std::endl;
            return true;
        }
    }
    return false;
}

// 检查以 end_prime 结尾的连续12个素数（间隙后的情况）
bool check_backward(uint64_t end_prime, uint64_t p13) {
    primesieve::iterator it;
    it.jump_to(end_prime);
    uint64_t p12 = it.next_prime();
    if (p12 != end_prime) return false;

    // 收集 p12 及之前的11个素数
    std::vector<uint64_t> primes;
    primes.push_back(p12);
    for (int i = 0; i < 11; ++i) {
        primes.push_back(it.prev_prime());
    }
    std::reverse(primes.begin(), primes.end()); // 现在 primes[0]=p1, primes[11]=p12
    uint64_t p1 = primes[0];

    // 获取 p0
    it.jump_to(p1);
    uint64_t p0 = it.prev_prime();

    // 获取 p14
    it.jump_to(p13);
    uint64_t p14 = it.next_prime();

    if (p12 - p1 <= INTERVAL_LEN - 1) {
        uint64_t L_min = std::max(p0 + 1, p12 - (INTERVAL_LEN - 1));
        uint64_t L_max = std::min(p1, p14 - INTERVAL_LEN);
        if (L_min <= L_max && L_min < CANDIDATE) {
            std::cout << "发现更小解 n = " << L_min
                      << " (来自后向检查，p12=" << p12 << ")" << std::endl;
            return true;
        }
    }
    return false;
}

int main() {
    std::string filename = "t0.txt";
    auto gaps = read_gaps(filename);
    std::cout << "加载了 " << gaps.size() << " 个间隙记录 (P(g) < " << CANDIDATE << ")" << std::endl;

    for (const auto& rec : gaps) {
        if (rec.g < 1000) continue; // 只检查较大间隙，可调整
        std::cout << "检查间隙 g=" << rec.g << " 起始素数=" << rec.p << std::endl;

        // 情况1：间隙在簇后面 (p 是 p12)
        if (check_backward(rec.p, rec.p + rec.g)) {
            return 0;
        }
        // 情况2：间隙在簇前面 (p+g 是 p1)
        if (check_forward(rec.p + rec.g, rec.p)) {
            return 0;
        }
    }
    return 0;
}
