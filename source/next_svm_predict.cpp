//
// Created by Ernir Erlingsson on 21.9.2018.
//

#include <iostream>
#include <fstream>
#include "next_svm_predict.h"
#include "next_svm_io.h"
#include "test_util.h"
#include "pisvm.h"

void perform_io_unit_tests() {
    char *in_file;
    #ifdef _WIN32
        in_file = const_cast<char *>("..\\output\\out.bin");
    #else
        in_file = const_cast<char *>("../output/out.bin");
    #endif
    // 1
    do_unit_test(const_cast<char *>("1. Testing file read I"), [&in_file]() {
        bool r = true;
        // Single block, complete data
        auto *data = new next_svm_data(in_file, 1, 0, 10000);
        int read_bytes = data->read_next_samples();
        if (data->total_number_of_samples != 1000 || data->total_number_of_features != 30
            || data->remaining_samples != 0 || read_bytes != 124000 || data->classes[0] != 54
            || data->classes[data->total_number_of_samples - 1] != 52 || data->features[0] - 0.693624 > 0.001
            || data->features[data->total_number_of_samples * data->total_number_of_features - 1] - 0.466426 > 0.001) {
            r = false;
        }
        return r;
    });
    // 2
    do_unit_test(const_cast<char *>("2. Testing file read II"), [&in_file]() {
        bool r = true;
        // 5 blocks, 1 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 5, 1, 100);
        int read_bytes = data->read_next_samples();
        if (data->total_number_of_samples != 1000 || data->total_number_of_features != 30 ||
            data->remaining_samples != 100 || read_bytes != 12400 || data->classes[0] != 10 ||
            data->features[0] - 0.858864 > 0.001 || data->classes[99] != 17
            || data->features[100*data->total_number_of_features-1] - 0.468884 > 0.001) {
            r = false;
        }
        delete data;
        return r;
    });
    // 3
    do_unit_test(const_cast<char *>("3. Testing file read III"), [&in_file]() {
        bool r = true;
        // 2 blocks, 0 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 2, 0, 100);
        data->read_next_samples();
        data->read_next_samples();
        int read_bytes = data->read_next_samples();
        if (data->total_number_of_samples != 1000 || data->total_number_of_features != 30 ||
            data->remaining_samples != 200 || read_bytes != 12400 || data->classes[0] != 10 ||
            data->features[0] - 0.858864 > 0.001 || data->classes[99] != 17
            || data->features[100*data->total_number_of_features-1] - 0.468884 > 0.001) {
            r = false;
        }
        delete data;
        return r;
    });
    // 4
    do_unit_test(const_cast<char *>("4. Testing file read IV"), [&in_file]() {
        bool r = true;
        // 3 blocks, 0 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 3, 0, 100);
        int read_bytes = data->read_next_samples();
        if (read_bytes != 12400 || data->remaining_samples != 234) {
            r = false;
        }
        delete data;
        return r;
    });
    // 5
    do_unit_test(const_cast<char *>("5. Testing file read V"), [&in_file]() {
        bool r = true;
        // 3 blocks, 1 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 3, 1, 100);
        int read_bytes = data->read_next_samples();
        if (read_bytes != 12400 || data->remaining_samples != 233) {
            r = false;
        }
        delete data;
        return r;
    });
}

void perform_unit_tests() {
    perform_io_unit_tests();
}

/*
 *  int i;
      int nr_class = model->nr_class;
      int l = model->l;

      double *kvalue = Malloc(double,l);
      for(i=0;i<l;i++)
	kvalue[i] = Kernel::k_function(x, nz_x, lx,
				       model->SV[i], model->nz_sv[i],
				       model->sv_len[i], model->param);

      int *start = Malloc(int,nr_class);
      start[0] = 0;
      for(i=1;i<nr_class;i++)
	start[i] = start[i-1]+model->nSV[i-1];

      int p=0;
      int pos=0;
      for(i=0;i<nr_class;i++)
	for(int j=i+1;j<nr_class;j++)
	  {
	    double sum = 0;
	    int si = start[i];
	    int sj = start[j];
	    int ci = model->nSV[i];
	    int cj = model->nSV[j];

	    int k;
	    double *coef1 = model->sv_coef[j-1];
	    double *coef2 = model->sv_coef[i];
	    for(k=0;k<ci;k++)
	      sum += coef1[si+k] * kvalue[si+k];
	    for(k=0;k<cj;k++)
	      sum += coef2[sj+k] * kvalue[sj+k];
	    sum -= model->rho[p++];
	    dec_values[pos++] = sum;
	  }

      free(kvalue);
      free(start);
    }
 */

/*
 * 	return exp(-param.gamma*(
				 dot(x, nz_x, lx, x, nz_x, lx)+
				 dot(y, nz_y, ly, y, nz_y, ly)-
				 2*dot(x, nz_x, lx, y, nz_y, ly)));

double Kernel::dot(const Xfloat *x, const int *nz_x, const int lx,
		   const Xfloat *y, const int *nz_y, const int ly)
{
  register double sum = 0;
  register int i = 0;
  register int j = 0;
  while(i < lx && j < ly)
    {
      if(nz_x[i] == nz_y[j])
	{
	  sum += x[i] * y[j];
	  ++i; ++j;
	}
      else if(nz_x[i] > nz_y[j])
	++j;
      else if(nz_x[i] < nz_y[j])
	++i;
    }
  return sum;
}
 */

inline double dot() {

}

void kernel_rbf() {
    /*
    return exp(-param.gamma*(
            dot(x, nz_x, lx, x, nz_x, lx)+
            dot(y, nz_y, ly, y, nz_y, ly)-
            2*dot(x, nz_x, lx, y, nz_y, ly)));
            */
}


void next_svm_predict(svm_model *model, next_svm_data *data) {
    int no_of_classes = model->nr_class;
    // support vectors
    int svs = model->l;

    auto *kvalue = new double[svs];
/*
 * kvalue[i] = Kernel::k_function(x, nz_x, lx,
				       model->SV[i], model->nz_sv[i],
				       model->sv_len[i], model->param);
 */

    for(int i = 0; i < svs; i++) {

    }

    delete[] kvalue;
}

int main(int argc, char **argv) {

    if (argc == 1) {
        perform_unit_tests();
        return 0;
    } else if (argc != 3) {
        std::cout << "Wrong number of arguments, should be 3: [model file] [test file]" << std::endl;
        return -1;
    }

    auto *model_file = argv[1];
    auto *test_file = argv[2];
    svm_model *model = svm_load_model(model_file);
    std::cout << "***MODEL INFO***" << std::endl;
    std::cout << "Cache size: " << model->param.cache_size << std::endl;
    std::cout << "degree: " << model->param.degree << std::endl;
    std::cout << "gamma: " << model->param.gamma << std::endl;
    std::cout << "coef0: " << model->param.coef0 << std::endl;
    std::cout << "C: " << model->param.C << std::endl;
    std::cout << "svm_type " << model->param.svm_type<< std::endl;
    std::cout << "kernel_type " << model->param.kernel_type<< std::endl;
    std::cout << "number of classes: " << model->nr_class << std::endl;
    std::cout << "number of SVs: " << model->l << std::endl;
    std::cout << "***" << std::endl;
    auto *data = new next_svm_data(test_file, 1, 0, 10000);
    int read_bytes = data->read_next_samples();
    std::cout << "read test data bytes: " << read_bytes << std::endl;
    next_svm_predict(model, data);
}