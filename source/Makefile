OUT=nextsvm
omp:
	rm -f $(OUT)-omp
	icc -O3 -DOMP_ON=1 -fopenmp -std=c++14 main.cpp next_svm.cpp -o $(OUT)-omp
omp-debug:
	rm -f $(OUT)-omp-debug
	icc -O3 -DOMP_ON=1 -DDEBUG_ON=1 -fopenmp -std=c++14 main.cpp next_svm.cpp -o $(OUT)-omp


