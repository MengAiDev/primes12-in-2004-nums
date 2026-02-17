import sys
import time

# ==================== 可配置参数 ====================
UPPER = 413523644431096819312710 
MAX_CONSECUTIVE_FAILS = 1000000000  # 连续未找到解的窗口个数阈值
# ===================================================

# ---------- Miller-Rabin 素性测试----------
_SMALL_PRIMES = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37]

def is_probable_prime(n: int) -> bool:
    """Miller-Rabin 素性测试，对于 n < 2^64 可视为确定性，更大时错误率极低"""
    if n < 2:
        return False
    # 小质数直接判断
    for p in _SMALL_PRIMES:
        if n % p == 0:
            return n == p
    # 写 n-1 = d * 2^s
    d = n - 1
    s = 0
    while d % 2 == 0:
        d //= 2
        s += 1
    # 对每个基进行测试
    for a in _SMALL_PRIMES:
        if a >= n:
            continue
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(s - 1):
            x = (x * x) % n
            if x == n - 1:
                break
        else:
            return False
    return True

# ---------- 向下滑动窗口扫描（收集所有解）----------
def find_all_solutions_down(start_from: int, max_fails: int):
    """
    从 start_from 开始向下递减，寻找所有满足条件的起始数。
    条件：从该数开始的连续 2004 个整数中恰好有 12 个质数。
    每找到一个解就记录，并继续向下，直到连续 max_fails 次尝试均无解时停止。
    返回解列表。
    """
    window_size = 2004
    current_start = start_from

    print(f"初始化窗口，起始于 {current_start} ...")
    t0 = time.time()
    # 初始化第一个窗口 [current_start, current_start+2003]
    prime_flags = [False] * window_size
    count = 0
    for i in range(window_size):
        n = current_start + i
        if is_probable_prime(n):
            prime_flags[i] = True
            count += 1
        if i % 500 == 0:
            print(f"  初始进度: {i}/{window_size} 个数已测")
    print(f"初始窗口完成，质数个数: {count}，耗时: {time.time()-t0:.2f}s")

    solutions = []
    consecutive_fails = 1   # 因为初始窗口已经失败一次（若它恰好是解，下面会处理）
    total_tested = window_size
    last_report = time.time()

    # 检查初始窗口
    if count == 12:
        solutions.append(current_start)
        print(f"🎯 找到解 (初始窗口): {current_start}")
        consecutive_fails = 0   # 刚找到解，重置失败计数

    # 开始向下移动
    while True:
        # 窗口向下移动一步：新起始 = current_start - 1
        new_start = current_start - 1
        # 添加最左边的数（new_start），移除最右边的数（current_start + window_size）
        old_right = current_start + window_size - 1  # 即将被移除的数
        new_left = new_start                          # 新加入的数

        # 移除最右边的数
        if prime_flags[-1]:
            count -= 1
        # 滚动数组：将标志数组向左移动一位（移除最右，空出最左）
        prime_flags = [False] + prime_flags[:-1]
        # 测试新加入的数（最左边）
        is_prime = is_probable_prime(new_left)
        prime_flags[0] = is_prime
        if is_prime:
            count += 1

        current_start = new_start
        total_tested += 1

        # 检查是否找到解
        if count == 12:
            solutions.append(current_start)
            print(f"🎯 找到解 (起始 {current_start})")
            consecutive_fails = 0   # 重置连续失败计数
        else:
            consecutive_fails += 1

        # 定期报告进度
        if current_start % 10000 == 0:
            now = time.time()
            if now - last_report > 5:
                speed = total_tested / (now - t0)
                print(f"当前起始: {current_start}, 已测试 {total_tested} 个数, "
                      f"速度: {speed:.0f} 数/秒, 当前质数计数: {count}, "
                      f"连续失败: {consecutive_fails}")
                last_report = now

        # 检查是否达到连续失败阈值
        if consecutive_fails >= max_fails:
            print(f"连续尝试 {max_fails} 个起始点均未找到解，停止搜索。")
            break

    return solutions

# ---------- 主程序 ----------
def main():
    print("=== 寻找所有连续2004个数中恰有12个质数的起始数（向下搜索）===")
    print(f"起始上限: {UPPER}")
    print(f"连续失败阈值: {MAX_CONSECUTIVE_FAILS}")
    print("开始搜索...")
    solutions = find_all_solutions_down(UPPER, MAX_CONSECUTIVE_FAILS)
    if not solutions:
        print(f"在连续 {MAX_CONSECUTIVE_FAILS} 次尝试内未找到任何解。")
    else:
        print("\n" + "="*60)
        print(f"总共找到 {len(solutions)} 个解：")
        for idx, sol in enumerate(solutions, 1):
            print(f"{idx:3d}. 起始数: {sol}")
            primes = []
            for i in range(2004):
                if is_probable_prime(sol + i):
                    primes.append(i)
            print(f"    质数偏移量: {primes}")
        print("="*60)

if __name__ == '__main__':
    main()
