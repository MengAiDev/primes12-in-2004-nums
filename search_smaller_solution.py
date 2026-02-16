import random
import time
from collections import defaultdict

# ---------- 生成前1000个小质数用于快速试除 ----------
def generate_small_primes(limit=1000):
    sieve = [True] * (limit + 1)
    sieve[0] = sieve[1] = False
    for i in range(2, int(limit**0.5) + 1):
        if sieve[i]:
            step = i
            start = i * i
            sieve[start:limit+1:step] = [False] * ((limit - start) // step + 1)
    return [i for i, is_p in enumerate(sieve) if is_p]

SMALL_PRIMES = generate_small_primes(1000)
MR_BASES = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37]   # Miller-Rabin 基组

# ---------- 素性测试 (小质数试除 + Miller-Rabin) ----------
def is_prime(n: int) -> bool:
    """确定性概率素性测试（对于超大整数仍适用）"""
    if n < 2:
        return False
    # 用前1000个小质数试除
    for p in SMALL_PRIMES:
        if p * p > n:
            break
        if n % p == 0:
            return n == p
    # 如果 n 小于最大小质数的平方，则已通过所有试除，必为质数
    if n <= SMALL_PRIMES[-1] ** 2:
        return True

    # Miller-Rabin
    d = n - 1
    s = 0
    while d % 2 == 0:
        d //= 2
        s += 1

    for a in MR_BASES:
        if a % n == 0:
            continue
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(s - 1):
            x = pow(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    return True

# ---------- 统计区间 [start, start+2003] 内质数个数（含提前退出优化）----------
def count_primes_in_range(start: int) -> int:
    count = 0
    for i in range(2004):
        remaining = 2004 - i               # 包括当前数在内还剩多少个
        if count + remaining < 12:          # 即使后面全是质数也达不到12
            return count
        n = start + i
        if is_prime(n):
            count += 1
            if count > 12:                  # 已经超过12，提前返回
                return count
    return count

# ---------- 主搜索函数 ----------
def find_min_solution(low: int = 10**11, high: int = 10**70,
                      window_size: int = 10**10,
                      max_jumps_per_window: int = 10**4,
                      total_attempts_limit: int = 10**6):
    """
    在 [low, high-2003] 范围内随机跳跃寻找解。
    每个窗口（长度 window_size）最多尝试 max_jumps_per_window 次。
    记录所有找到的解，最后输出最小的起始数。
    """
    max_start = high - 2003
    if max_start < low:
        raise ValueError("范围太小，无法容纳 2004 个连续整数")

    # 记录每个窗口已尝试的次数
    window_attempts = defaultdict(int)
    best_solution = None
    total_attempts = 0
    start_time = time.time()

    while total_attempts < total_attempts_limit:
        # 随机生成一个起始点
        start = random.randint(low, max_start)
        # 计算所属窗口索引（窗口从 low 开始对齐）
        idx = (start - low) // window_size

        # 检查该窗口是否已达到跳跃次数上限
        if window_attempts[idx] >= max_jumps_per_window:
            continue   # 跳过该点，不增加总尝试次数

        # 接受该点
        total_attempts += 1
        window_attempts[idx] += 1

        # 统计区间内质数个数
        cnt = count_primes_in_range(start)

        if cnt == 12:
            if best_solution is None or start < best_solution:
                best_solution = start
                print(f"[{time.time()-start_time:.1f}秒] 找到新解: {start} (当前最小)")

        # 每 1000 次有效尝试输出一次进度
        if total_attempts % 1000 == 0:
            elapsed = time.time() - start_time
            min_str = str(best_solution) if best_solution else "无"
            print(f"[{elapsed:.1f}秒] 已尝试 {total_attempts} 次，当前最小解: {min_str}")

    # 搜索结束
    elapsed = time.time() - start_time
    print("\n" + "="*60)
    print(f"搜索结束，总有效尝试次数: {total_attempts}")
    if best_solution:
        print(f"找到的最小起始数: {best_solution}")
    else:
        print("未找到任何解")
    print(f"耗时: {elapsed:.2f} 秒")
    return best_solution

if __name__ == "__main__":
    # 可根据需要调整参数
    find_min_solution(low=10**20, high=10**70,
                      window_size=10**10,
                      max_jumps_per_window=10**4,
                      total_attempts_limit=10**6)
