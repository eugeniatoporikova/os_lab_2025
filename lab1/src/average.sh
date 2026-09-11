#!/bin/bash
count=$#
sum=0
for arg in "$@"; do 
    sum=$((sum + arg))
done

average=$((sum / count))

echo "Количество входных аргументов: $count"
echo "Среднее арифметическое входных аргументов: $average"