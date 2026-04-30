#!/usr/bin/env python3
"""
计算 values-* 文件中三个数的平均值
文件格式：每行三个浮点数，空格分隔，如 "0.911472 438.850398 254.682000"
使用方法：python3 avg_values.py <results_directory>
"""

import os
import sys

def main():
    results_dir = sys.argv[1] if len(sys.argv) > 1 else "results"

    if not os.path.isdir(results_dir):
        print(f"Error: Directory {results_dir} does not exist")
        sys.exit(1)

    # 初始化累加器
    sum1, sum2, sum3 = 0.0, 0.0, 0.0
    count = 0

    # 遍历所有 values-* 文件
    values_files = sorted([f for f in os.listdir(results_dir) if f.startswith("values-")])

    for filename in values_files:
        filepath = os.path.join(results_dir, filename)
        try:
            with open(filepath, 'r') as f:
                # 读取一行，空格分隔三个数
                line = f.read().strip()
                parts = line.split()
                if len(parts) >= 3:
                    val1 = float(parts[0])
                    val2 = float(parts[1])
                    val3 = float(parts[2])

                    sum1 += val1
                    sum2 += val2
                    sum3 += val3
                    count += 1

                    print(f"File: {filename} -> {val1:.6f}  {val2:.6f}  {val3:.6f}")
        except Exception as e:
            print(f"Error reading {filename}: {e}")

    # 计算并输出平均值
    if count > 0:
        avg1 = sum1 / count
        avg2 = sum2 / count
        avg3 = sum3 / count

        print("")
        print("=" * 50)
        print(f"Total files processed: {count}")
        print("Average values:")
        print(f"  Value 1 average: {avg1:.6f}")
        print(f"  Value 2 average: {avg2:.6f}")
        print(f"  Value 3 average: {avg3:.6f}")
        print("=" * 50)
    else:
        print(f"No values-* files found in {results_dir}")

if __name__ == "__main__":
    main()