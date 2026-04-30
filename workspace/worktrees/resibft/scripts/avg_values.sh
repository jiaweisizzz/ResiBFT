#!/bin/bash

# 计算 values-* 文件中三个数的平均值
# 文件格式：每行三个浮点数，空格分隔，如 "0.911472 438.850398 254.682000"
# 使用方法：./avg_values.sh <results_directory>

RESULTS_DIR=${1:-"results"}

# 检查目录是否存在
if [ ! -d "$RESULTS_DIR" ]; then
    echo "Error: Directory $RESULTS_DIR does not exist"
    exit 1
fi

# 初始化累加器（使用 awk 处理浮点数）
count=0

# 使用 awk 处理所有文件并计算平均值
awk '
BEGIN {
    sum1 = 0; sum2 = 0; sum3 = 0; count = 0
}
{
    # 每行三个数，空格分隔
    val1 = $1; val2 = $2; val3 = $3
    sum1 += val1
    sum2 += val2
    sum3 += val3
    count++
    print FILENAME ": " val1 "  " val2 "  " val3
}
END {
    print ""
    print "=========================================="
    print "Total files processed: " count
    print "Average values:"
    print "  Value 1 average: " (sum1/count)
    print "  Value 2 average: " (sum2/count)
    print "  Value 3 average: " (sum3/count)
    print "=========================================="
}
' "$RESULTS_DIR"/values-*