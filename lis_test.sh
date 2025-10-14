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

srun -c 16 ~/LIS/build/LIS -i ./input_example.txt --seed 123456 --step 1000 -o test_output.txt
