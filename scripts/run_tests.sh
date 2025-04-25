#!/bin/bash

# Скрипт для проверки корректности работы программы

set -e

# Переход в директорию сборки
cd build

# Запуск программы с различными параметрами для проверки работоспособности
echo "Running test with function k=1 (f(x,y) = 1)"
./Approximation_2D -1 1 -1 1 50 50 1 1e-6 100 1 || { echo "Test failed for k=1"; exit 1; }

echo "Running test with function k=2 (f(x,y) = x)"
./Approximation_2D -1 1 -1 1 50 50 2 1e-6 100 1 || { echo "Test failed for k=2"; exit 1; }

echo "Running test with function k=3 (f(x,y) = y)"
./Approximation_2D -1 1 -1 1 50 50 3 1e-6 100 1 || { echo "Test failed for k=3"; exit 1; }

echo "Running test with function k=4 (f(x,y) = x+y)"
./Approximation_2D -1 1 -1 1 50 50 4 1e-6 100 1 || { echo "Test failed for k=4"; exit 1; }

echo "Running test with function k=5 (f(x,y) = x²+y²)"
./Approximation_2D -1 1 -1 1 50 50 5 1e-6 100 1 || { echo "Test failed for k=5"; exit 1; }

echo "Running test with high resolution"
./Approximation_2D -1 1 -1 1 100 100 5 1e-6 100 1 || { echo "Test failed with high resolution"; exit 1; }

echo "Running test with multithreading"
./Approximation_2D -1 1 -1 1 50 50 5 1e-6 100 4 || { echo "Test failed with multithreading"; exit 1; }

echo "All tests passed successfully!"
exit 0 