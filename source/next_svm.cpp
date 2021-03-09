//
// Created by Ernir Erlingsson on 8.3.2021.
//

#include "next_svm.h"

bool next_svm::next_data(magmaMPI mpi) noexcept{
    int node_transmit_size = magma_util::get_block_size(mpi.rank, sample_size, mpi.n_nodes);
    int node_transmit_offset = magma_util::get_block_offset(mpi.rank, sample_size, mpi.n_nodes);
    if (transmit_cnt >= m) {
        return false;
    }
    exa::fill(v_x_chunk, 0, v_x_chunk.size(), std::numeric_limits<float>::max());
    exa::fill(v_y_chunk, 0, v_y_chunk.size(), std::numeric_limits<int>::max());
    if (transmit_cnt + node_transmit_size <= m) {
        exa::copy(v_y, transmit_cnt, transmit_cnt + node_transmit_size,
                v_y_chunk, node_transmit_offset);
        exa::copy(v_x, transmit_cnt * n,
                (transmit_cnt + node_transmit_size) * n,
                v_x_chunk, node_transmit_offset * n);
    } else {
        std::size_t size = m - transmit_cnt;
        exa::copy(v_y, transmit_cnt, transmit_cnt + size,
                v_y_chunk, node_transmit_offset);
        exa::copy(v_x, transmit_cnt * n,
                (transmit_cnt + size) * n,
                v_x_chunk, node_transmit_offset * n);
    }
    transmit_cnt += node_transmit_size;
#ifdef MPI_ON
    if (mpi.n_nodes > 1) {
        mpi.allGather(v_x_agg);
        mpi.allGather(v_y_agg);
    }
#endif
    return true;
}

void next_svm::process_data(magmaMPI mpi) noexcept {
    v_alpha.resize(v_y_chunk.size(), 0);
    auto const _alpha = v_alpha.begin();
//    auto const _x = v_x_chunk.begin();
    auto const _x_c = v_x_chunk.begin();
    auto const _y_c = v_y_chunk.begin();
    auto const _m = m;
    auto const _n = n;
    auto const _l = l;
    auto const _c = c;
    auto const _i_max = std::numeric_limits<int>::max();
    float _b = 0;

    d_vec<float> v_alpha_new(v_alpha.size());
    auto const _alpha_new = v_alpha_new.begin();
    d_vec<float> v_fx(v_y_chunk.size());
    auto const _fx = v_fx.begin();

    auto offset = magma_util::get_block_offset(mpi.rank, static_cast<int>(v_y_chunk.size()), mpi.n_nodes);
    auto size = magma_util::get_block_size(mpi.rank, static_cast<int>(v_y_chunk.size()), mpi.n_nodes);
    d_vec<float> v_ii_jj(v_y_chunk.size(), 0);
    auto const _ii_jj = v_ii_jj.begin();
    d_vec<float> v_ij(size);
    auto const _ij = v_ij.begin();


    exa::for_each(offset, offset + size, [=]
#ifdef CUDA_ON
    __device__
#endif
    (auto const &i) -> void {
        if (_y_c[i] == _i_max)
            return;
        float fxi = 0;
        for (int j = 0; j < _m; ++j) {
            for (int k = 0; k < _n; ++k) {
                fxi += (_alpha[j] * _y_c[j]) * (_x_c[i * _n + k] * _x_c[j * _n + k]);
            }
        }
        _fx[i] = fxi;
    });
#ifdef MPI_ON
    if (mpi.n_nodes > 1)
        mpi.allReduce(v_fxi, magmaMPI::sum);
#endif
    exa::for_each(offset, offset + size, [=]
#ifdef CUDA_ON
    __device__
#endif
    (auto const &i) -> void {
        float val = 0;
        for (int k = 0; k < _n; ++k) {
            val += _x_c[i * _n + k] * _x_c[i * _n + k];
        }
        _ii_jj[i] = val;
    });
#ifdef MPI_ON
    if (mpi.n_nodes > 1)
        mpi.allReduce(v_ii_jj, magmaMPI::sum);
#endif

    for (std::size_t i = 0; i < v_y_chunk.size(); ++i) {
        float Ei = _fx[i] + _b - _y_c[i];
        if (!((_y_c[i] * Ei < -_l && _alpha[i] < _c) || (_y_c[i] * Ei > _l && _alpha[i] > 0))) {
            continue;
        }
        exa::for_each(offset, offset + size, [=]
#ifdef CUDA_ON
        __device__
#endif
        (auto const &j) -> void {
            float val = 0;
            for (int k = 0; k < _n; ++k) {
                val += _x_c[i * _n + k] * _x_c[j * _n + k];
            }
            _ij[j - offset] = val;
        });
        exa::fill(v_alpha_new, offset, offset + size, static_cast<float>(0));
        exa::for_each(offset, offset + size, [=]
#ifdef CUDA_ON
        __device__
#endif
        (auto const &j) -> void {
            if (i == j)
                return;
            float L, H;
            if (_y_c[i] != _y_c[j]) {
                L = _alpha[j] - _alpha[i] <= 0 ? 0 : _alpha[j] - _alpha[i];
                H = c + _alpha[j] - _alpha[i] > c ? c : c + _alpha[j] - _alpha[i];
            } else {
                L = _alpha[j] + _alpha[i] - c <= 0 ? 0 : _alpha[j] + _alpha[i] - c;
                H = _alpha[j] + _alpha[i] > c ? c : _alpha[j] + _alpha[i];
            }
            if (L == H)
                return;
            float eta = 2 * _ij[j - offset] - _ii_jj[i] - _ii_jj[j];
            if (eta >= 0)
                return;
            auto Ej = _fx[j] + _b - _y_c[j];
            float _alpha_mod = _alpha[j] - _y_c[j] * (Ei - Ej) / eta;
            // clip
            if (_alpha_mod > H) {
                _alpha_mod = H;
            } else if (L > _alpha_mod) {
                _alpha_mod = L;
            }
            if ((_alpha_mod < 0 && _alpha_mod <= l) || (_alpha_mod > 0 && _alpha_mod >= l)) {
                _alpha_new[j] = _alpha_mod;
            }
        });

        // find best local alpha
        // mpi> collect all best alphas and select
//#ifdef MPI_ON
//        if (mpi.n_nodes > 1)
//        mpi.allReduce(v_alpha_new, magmaMPI::sum);
//#endif

    } // for loop over i
}

/*
 def simplifiedSMO(dataX, classY, C, tol, max_passes):
       X = mat(dataX);
       Y = mat(classY).T
       m,n = shape(X)
       # Initialize b: threshold for solution
       b = 0;
       # Initialize alphas: lagrange multipliers for solution
       alphas = mat(zeros((m,1)))
       passes = 0
       while (passes < max_passes):
              num_changed_alphas = 0
              for i in range(m):
                     # Calculate Ei = f(xi) - yi
                     fXi = float(multiply(alphas,Y).T*(X*X[i,:].T)) + b
                     Ei = fXi - float(Y[i])
                     if ((Y[i]*Ei < -tol) and (alphas[i] < C)) or ((Y[i]*Ei > tol)
                                  and (alphas[i] > 0)):
                           # select j # i randomly
                           j = selectJrandom(i,m)
                           # Calculate Ej = f(xj) - yj
                           fXj = float(multiply(alphas,Y).T*(X*X[j,:].T)) + b
                           Ej = fXj - float(Y[j])
                           # save old alphas's
                           alphaIold = alphas[i].copy();
                           alphaJold = alphas[j].copy();
                           # compute L and H
                           if (Y[i] != Y[j]):
                                  L = max(0, alphas[j] - alphas[i])
                                  H = min(C, C + alphas[j] - alphas[i])
                           else:
                                  L = max(0, alphas[j] + alphas[i] - C)
                                  H = min(C, alphas[j] + alphas[i])
                           # if L = H the continue to next i
                           if L==H:
                                  continue
                           # compute eta
                           eta = 2.0 * X[i,:]*X[j,:].T - X[i,:]*X[i,:].T - X[j,:]*X[j,:].T
                           # if eta >= 0 then continue to next i
                           if eta >= 0:
                                  continue
                           # compute new value for alphas j
                           alphas[j] -= Y[j]*(Ei - Ej)/eta
                           # clip new value for alphas j
                           alphas[j] = clipAlphasJ(alphas[j],H,L)
                           # if |alphasj - alphasold| < 0.00001 then continue to next i
                           if (abs(alphas[j] - alphaJold) < 0.00001):
                                  continue
                           # determine value for alphas i
                           alphas[i] += Y[j]*Y[i]*(alphaJold - alphas[j])
                           # compute b1 and b2
                           b1 = b - Ei- Y[i]*(alphas[i]-alphaIold)*X[i,:]*X[i,:].T -
                                      Y[j]*(alphas[j]-alphaJold)*X[i,:]*X[j,:].T
                           b2 = b - Ej- Y[i]*(alphas[i]-alphaIold)*X[i,:]*X[j,:].T
                                    - Y[j]*(alphas[j]-alphaJold)*X[j,:]*X[j,:].T
                           # compute b
                           if (0 < alphas[i]) and (C > alphas[i]):
                                  b = b1
                           elif (0 < alphas[j]) and (C > alphas[j]):
                                  b = b2
                           else:
                                  b = (b1 + b2)/2.0
                           num_changed_alphas += 1
                     if (num_changed_alphas == 0): passes += 1
                     else: passes = 0
       return b,alphas
 */
