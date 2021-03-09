//
// Created by Ernir Erlingsson on 8.3.2021.
//

#ifndef NEXT_SVM_NEXT_SVM_H
#define NEXT_SVM_NEXT_SVM_H

#ifdef CUDA_ON
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
template <typename T>
using h_vec = thrust::host_vector<T>;
template <typename T>
using d_vec = thrust::device_vector<T>;
#else
#include <vector>
template <typename T>
using h_vec = std::vector<T>;
template <typename T>
using d_vec = std::vector<T>;
#endif
#include "magma_mpi.h"
#include "magma_util.h"

#ifdef OMP_ON
#include "magma_exa_omp.h"
#elif CUDA_ON
#include "magma_exa_cu.h"
#else
#include "magma_exa.h"
#endif

class next_svm {
private:
    int const sample_size = 1000000;
    int transmit_cnt = 0;


    float const c, l;
    int const m, n;
    d_vec<float> v_alpha;
    d_vec<float> v_x_chunk;
    d_vec<int> v_y_chunk;

public:
    d_vec<float> v_x;
    d_vec<int> v_y;
    explicit next_svm(h_vec<float> &v_x, h_vec<int> &v_y, const int m, const int n, const float c, const float l)
            : v_x(std::move(v_x)), v_y(std::move(v_y)), m(m), n(n), c(c), l(l) {
        v_x_chunk.resize(m * n);
        v_y_chunk.resize(m);
    }

    bool next_data(magmaMPI mpi) noexcept;

    void process_data(magmaMPI mpi) noexcept;

};


#endif //NEXT_SVM_NEXT_SVM_H
