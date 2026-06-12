#!/bin/bash
#SBATCH -p main
#SBATCH -c 16
#SBATCH --mem=1G
#SBATCH -t 2:00:00
#SBATCH -J test_LIS
#SBATCH -o output_test_lis.txt
#SBATCH -e error_test_lis.txt
#SBATCH --array=1

rm -rf lis_test/
mkdir -p lis_test

./build/LIS --input input_example.txt --param parameter.ini --seed 123456 --steps 100 --output lis_test/test_output.txt
