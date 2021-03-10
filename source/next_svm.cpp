//
// Created by Ernir Erlingsson on 8.3.2021.
//

#include "next_svm.h"

bool next_svm::next_data(magmaMPI mpi) noexcept{
    if (sample_size % mpi.n_nodes != 0) {
        sample_size += sample_size % mpi.n_nodes;
    }
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
        if (size % mpi.n_nodes != 0) {
            size += size % mpi.n_nodes;
        }
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
    auto const _n_elem = v_y_chunk.size();

    auto offset = magma_util::get_block_offset(mpi.rank, static_cast<int>(v_y_chunk.size()), mpi.n_nodes);
    auto size = magma_util::get_block_size(mpi.rank, static_cast<int>(v_y_chunk.size()), mpi.n_nodes);
#ifdef DEBUG_ON
    if (v_y_chunk.size() % mpi.n_nodes != 0) {
        std::cerr << "CRITICAL ERROR, UNEVEN NODE DATA PARTITIONS" << std::endl;
        exit(-2);
    }
#endif
    d_vec<float> v_ii_jj(v_y_chunk.size(), 0);
    auto const _ii_jj = v_ii_jj.begin();
    d_vec<float> v_ij(v_y_chunk.size());
    auto const _ij = v_ij.begin();
    d_vec<int> v_enum(v_y_chunk.size());
    exa::iota(v_enum, 0, v_enum.size(), 0);

    exa::for_each(offset, offset + size, [=]
#ifdef CUDA_ON
    __device__
#endif
    (auto const &i) -> void {
        if (_y_c[i] == _i_max)
            return;
        float fxi = 0;
        for (std::size_t j = 0; j < _n_elem; ++j) {
            if (_y_c[j] == _i_max)
                continue;
            float val = 0;
            for (int k = 0; k < _n; ++k) {
                val += (_x_c[i * _n + k] * _x_c[j * _n + k]);
            }
            val *= (_alpha[j] * _y_c[j]);
            fxi += val;
        }
        _fx[i] = fxi;
    });

#ifdef MPI_ON
    if (mpi.n_nodes > 1)
        mpi.allGather(v_fx);
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

    bool is_running = true;
    int n_iter = 0;
    while (is_running) {
        std::cout << "iter: " << ++n_iter << std::endl;
        is_running = false;
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
                _ij[j] = val;
            });

#ifdef MPI_ON
            if (mpi.n_nodes > 1)
                mpi.allGather(v_ij);
#endif

            exa::fill(v_alpha_new, 0, v_alpha_new.size(), static_cast<float>(0));
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
                float eta = 2 * _ij[j] - _ii_jj[i] - _ii_jj[j];
                if (eta >= 0)
                    return;
                auto Ej = _fx[j] + _b - _y_c[j];
                float _new_alpha = _alpha[j] - (_y_c[j] * (Ei - Ej) / eta);
                // clip
                if (_new_alpha > H) {
                    _new_alpha = H;
                } else if (L > _new_alpha) {
                    _new_alpha = L;
                }
                float _alpha_diff = _new_alpha - _alpha[j];
                if ((_alpha_diff < 0 && _alpha_diff <= l) || (_alpha_diff > 0 && _alpha_diff >= l)) {
//                    std::cout << "test: " << i << std::endl;
                    _alpha_new[j] = _new_alpha;
                }
//                else {
//                    _alpha_new[j] = _alpha[j];
//                }
            });


#ifdef MPI_ON
            if (mpi.n_nodes > 1)
                mpi.allGather(v_alpha_new);
//            mpi.allReduce(v_alpha_new, magmaMPI::sum);
#endif
            // find best alpha
//            magma_util::print_v("_alpha_new: ", &v_alpha_new[0], v_alpha_new.size());
//            magma_util::print_v("_alpha: ", &v_alpha[0], v_alpha.size());

            auto max_j = exa::max_element(v_enum, 0, v_enum.size(), [=]
#ifdef CUDA_ON
            __device__
#endif
            (auto const &i, auto const &j) -> bool {
                float val1 = _alpha_new[i] - _alpha[i];
                float val2 = _alpha_new[j] - _alpha[j];
                return val1 < val2;
            });
            float _alpha_diff = _alpha_new[max_j] - _alpha[max_j];
            if ((_alpha_diff < 0 && _alpha_diff <= l) || (_alpha_diff > 0 && _alpha_diff >= l)) {
                std::cout << "checkpoint i: " << i << " j: " << max_j << std::endl;
                is_running = true;
                std::cout << "i: " << i << " selected j: " << max_j << " with " << _alpha_new[max_j] << " cmp: " << _alpha[max_j] << std::endl;
                // update other alpha and calculate new b
                auto _alpha_i_old = _alpha[i];
                _alpha[i] += static_cast<float>(_y_c[max_j]) * static_cast<float>(_y_c[i]) *
                             (_alpha[max_j] - _alpha_new[max_j]);
                std::cout << "_alpha i old: " << _alpha_i_old << " new: " << _alpha[i] << std::endl;
                std::cout << "old b: " << _b << std::endl;
                if (0 < _alpha[i] && c > _alpha[i]) {
                    _b = _b - Ei - _y_c[i] * (_alpha[i] - _alpha_i_old) * _ii_jj[i] -
                         _y_c[max_j] * (_alpha[max_j] - _alpha_new[max_j]) * _ij[max_j];
                } else if (0 < _alpha[max_j] && c > _alpha[max_j]) {
                    _b = _b - (_fx[max_j] + _b - _y_c[max_j]) - _y_c[i] * (_alpha[i] - _alpha_i_old) * _ij[max_j] -
                         _y_c[max_j] * (_alpha[max_j] - _alpha_new[max_j]) * _ii_jj[max_j];
                } else {
                    _b = ((_b - Ei - _y_c[i] * (_alpha[i] - _alpha_i_old) * _ii_jj[i] -
                           _y_c[max_j] * (_alpha[max_j] - _alpha_new[max_j]) * _ij[max_j]) +
                          (_b - (_fx[max_j] + _b - _y_c[max_j]) - _y_c[i] * (_alpha[i] - _alpha_i_old) * _ij[max_j] -
                           _y_c[max_j] * (_alpha[max_j] - _alpha_new[max_j]) * _ii_jj[max_j])) / 2;
                }
                std::cout << "new b: " << _b << std::endl;

                // update fx table by removing old and replacing it with new ai and aj

                exa::for_each(offset, offset + size, [=]
#ifdef CUDA_ON
                __device__
#endif
                (auto const &ii) -> void {
                    if (_y_c[ii] == _i_max)
                        return;
                    float val = 0;
                    float val1 = 0;
                    for (int k = 0; k < _n; ++k) {
                        val1 += (_x_c[ii * _n + k] * _x_c[max_j * _n + k]);
                    }
                    val -= (_alpha[max_j] * _y_c[max_j]) * val1;
                    float val2 = 0;
                    for (int k = 0; k < _n; ++k) {
                        val2 += (_x_c[ii * _n + k] * _x_c[i * _n + k]);
                    }
                    val -= (_alpha_i_old * _y_c[i]) * val2;
                    val += (_alpha_new[max_j] * _y_c[max_j]) * val1;
                    val += (_alpha[i] * _y_c[i]) * val2;
                    _fx[ii] += val;
                });

#ifdef MPI_ON
                if (mpi.n_nodes > 1)
                    mpi.allGather(v_fx);
#endif
                std::cout << "_alpha j old: " << _alpha[max_j] << " new: " << _alpha_new[max_j] << std::endl;
                _alpha[max_j] = _alpha_new[max_j];
            }
        } // for loop over i
    }
    std::cout << "b: " << _b << std::endl;
    magma_util::print_v("alpha: ", &v_alpha[0], v_alpha.size());
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
//                           float _alpha_mod = _alpha[j] - _y_c[j] * (Ei - Ej) / eta;
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
