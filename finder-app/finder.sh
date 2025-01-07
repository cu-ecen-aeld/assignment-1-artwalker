#!/bin/sh

# 检查参数是否被正确提供
if [ $# -ne 2 ]; then
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1
fi

filesdir="$1"
searchstr="$2"

# 检查提供的路径是否是一个目录
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a directory"
    exit 1
fi

# 计算目录及其子目录中的文件数量
file_count=$(find "$filesdir" -type f | wc -l)

# 查找包含searchstr的行数
# 使用grep -r 递归搜索，-c 统计匹配的行数，-l 列出匹配的文件（这里为了统计文件而不是直接统计行）
matching_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

# 打印结果
echo "The number of files are $file_count and the number of matching lines are $matching_lines"
