//
// Created by Ernir Erlingsson on 8.3.2021.
//

#include "next_svm.h"
#ifdef OMP_ON
#include "magma_exa_omp.h"
#elif CUDA_ON
#include "magma_exa_cu.h"
#else
#include "magma_exa.h"
#endif

/*
 void data_process::select_and_process(magmaMPI mpi) noexcept {
    std::size_t million = 1000000;
    vector_resize(v_coord_id, n_coord, 0);
    exa::iota(v_coord_id, 0, v_coord_id.size(), 0);
    std::size_t n_sample_size = 100 * million;
    if (n_total_coord < n_sample_size) {
        // A bit of padding is added at the end to counter rounding issues
        n_sample_size = static_cast<int>(n_total_coord) + 2;
    }
#ifdef DEBUG_ON
    if (mpi.rank == 0) {
        std::cout << "sample size: " << n_sample_size << std::endl;
    }
#endif
    d_vec<int> v_id_chunk(n_sample_size, -1);
    d_vec<float> v_data_chunk(n_sample_size * n_dim);
    int node_transmit_size = magma_util::get_block_size(mpi.rank, static_cast<int>(n_sample_size), mpi.n_nodes);
    int node_transmit_offset = magma_util::get_block_offset(mpi.rank, static_cast<int>(n_sample_size), mpi.n_nodes);
#ifdef DEBUG_ON
    std::cout << "node: " << mpi.rank << " transmit offset: " << node_transmit_offset << " size: " << node_transmit_size << " : " << n_coord << std::endl;
#endif
    d_vec<int> v_point_id(v_coord_id);
    exa::iota(v_point_id, 0, v_point_id.size(), 0);
    v_coord_nn.resize(n_coord, 0);
    d_vec<int> v_point_nn(n_sample_size, 0);
    std::size_t transmit_cnt = 0;
    int n_iter = 0;

    while (transmit_cnt < n_coord) {
        exa::fill(v_id_chunk, 0, v_id_chunk.size(), -1);
        exa::fill(v_data_chunk, 0, v_data_chunk.size(), std::numeric_limits<float>::max());
        exa::fill(v_point_nn, 0, v_point_nn.size(), 0);
        if (transmit_cnt + node_transmit_size <= n_coord) {
            exa::copy(v_point_id, transmit_cnt, transmit_cnt + node_transmit_size,
                    v_id_chunk, node_transmit_offset);
            exa::copy(v_coord, transmit_cnt * n_dim,
                    (transmit_cnt + node_transmit_size) * n_dim,
                    v_data_chunk, node_transmit_offset * n_dim);
        } else {
            std::size_t size = n_coord - transmit_cnt;
            exa::copy(v_point_id, transmit_cnt, transmit_cnt + size,
                    v_id_chunk, node_transmit_offset);
            exa::copy(v_coord, transmit_cnt * n_dim,
                    (transmit_cnt + size) * n_dim,
                    v_data_chunk, node_transmit_offset * n_dim);
        }
        transmit_cnt += node_transmit_size;
#ifdef DEBUG_ON
        if (mpi.rank == 0)
            std::cout << "transmit iter: " << n_iter << ", elems sent:" << transmit_cnt << std::endl;
#endif
#ifdef MPI_ON
        if (mpi.n_nodes > 1)
            mpi.allGather(v_data_chunk);
#endif
        process_points(v_id_chunk, v_data_chunk);
        ++n_iter;
    }
}
 */

bool next_svm::next_data(magmaMPI mpi) {
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

void next_svm::process_data(magmaMPI mpi) {

}
