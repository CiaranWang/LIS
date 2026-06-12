#!/bin/bash
#SBATCH -p main
#SBATCH -c 16
#SBATCH --mem=1G
#SBATCH -t 2:00:00
#SBATCH -J test_LIS
#SBATCH -o output_test_lis.txt
#SBATCH -e error_test_lis.txt
#SBATCH --array=1

mkdir ./lis_test/
cd ./lis_test/

~/LIS/build/LIS --input ./input_example.txt --param ./parameter.ini --seed 123456 --steps 1000 --output test_output.txt
