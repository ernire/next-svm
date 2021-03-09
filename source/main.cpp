//
// Created by Ernir Erlingsson on 23.2.2021.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#ifdef MPI_ON
#include <mpi.h>
#endif
#ifdef OMP_ON
#include <omp.h>
#endif
#include "next_svm.h"


bool count_lines_and_dimensions(const std::string &in_file, std::size_t &n_elem, std::size_t &n_dim) noexcept {
    std::ifstream is(in_file);
    if (!is.is_open()) {
        return false;
    }
    std::string line, buf;
    int cnt = 0;
    n_dim = 0;
    while (std::getline(is, line)) {
        if (n_dim == 0) {
            std::istringstream iss(line);
            std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                    std::istream_iterator<std::string>());
            n_dim = results.size();
        }
        ++cnt;
    }
    n_elem = cnt;
    is.close();
    return true;
}

bool read_csv(std::string const &in_file, std::vector<float> &v_raw_data, std::size_t const n_dim) {
    std::ifstream is(in_file, std::ifstream::in);
    if (!is.is_open())
        return false;
    std::string line, buf;
    std::stringstream ss;
    while (std::getline(is, line)) {
        ss.str(std::string());
        ss.clear();
        ss << line;
        for (int j = 0; j < n_dim; j++) {
            ss >> buf;
            v_raw_data.push_back(static_cast<float>(atof(buf.c_str())));
        }
    }
    is.close();
    return true;
}

void data_separate(h_vec<float> &v_raw_data, h_vec<float> &v_x, h_vec<int> &v_y,
        std::size_t const m, std::size_t const n) {
    v_x.resize(m * ( n - 1));
    v_y.resize(m);
    auto const _n = n;
    auto const _y = v_y.begin();
    auto const _x = v_x.begin();
    exa::for_each(0, m, [=]
#ifdef CUDA_ON
    __device__
#endif
    (auto const &i) -> void {
        _y[i] = (int)v_raw_data[i * _n];
        for (std::size_t j = 1; j < n; ++j) {
            _x[i * (_n - 1) + j - 1] = v_raw_data[i * _n + j];
        }
    });
}

void read_input_bin(const std::string &in_file, h_vec<float> &v_points, int &m, int &n,
        int const n_nodes, int const i_node) noexcept {
    std::ifstream ifs(in_file, std::ios::in | std::ifstream::binary);
    ifs.read((char *) &m, sizeof(int));
    ifs.read((char *) &n, sizeof(int));
    auto size = magma_util::get_block_size(i_node, m, n_nodes);
    auto offset = magma_util::get_block_offset(i_node, m, n_nodes);
    m = size;
    auto feature_offset = 2 * sizeof(int) + (offset * n * sizeof(float));
    v_points.resize(size * n);
    ifs.seekg(feature_offset, std::istream::beg);
    ifs.read((char *) &v_points[0], size * n * sizeof(float));
    ifs.close();
}

int main(int argc, char** argv) {
    std::string input_file;
    std::string output_file;
    // The number of OMP threads
    int t = 1;
    // SVM slack variable
    float c = 0;
    // maximum number of passes
    int p = 1;
    // gap
    float l = 0.001;

    for (int i = 1; i < argc; i += 2) {
        std::string str(argv[i]);
        if (str == "-i") {
            input_file = argv[i + 1];
        } else if (str == "-o") {
            output_file = argv[i + 1];
        } else if (str == "-t") {
            t = std::stoi(argv[i + 1]);
        } else if (str == "-c") {
            c = std::stof(argv[i + 1]);
        } else if (str == "-l") {
            l = std::stof(argv[i + 1]);
        } else if (str == "-p") {
            p = std::stoi(argv[i + 1]);
        }
    }
#ifdef MPI_ON
    MPI_Init(&argc, &argv);
#endif
    auto start_timestamp = std::chrono::high_resolution_clock::now();

    auto mpi = magmaMPI::build();
#ifdef OMP_ON
    omp_set_num_threads(t);
#endif
    if (mpi.rank == 0) {
        std::cout << "Starting NextSVM with input file: " << input_file << " output file: " << output_file
            << " t: " << t << " c: " << c << " l: " << l << " p: " << p << std::endl;
    }
    h_vec<float> v_raw_data;
    h_vec<float> v_x;
    h_vec<int> v_y;
    int m, n;
    read_input_bin(input_file, v_raw_data, m, n, mpi.n_nodes, mpi.rank);
#ifdef DEBUG_ON
    std::cout << "input read" << std::endl;
#endif
    data_separate(v_raw_data, v_x, v_y, m, n);
    v_raw_data.clear();
    v_raw_data.shrink_to_fit();
    std::cout << "Found " << m << " elements with " << n << " dimensions." << std::endl;
    auto start_timestamp_no_io = std::chrono::high_resolution_clock::now();

    next_svm ns(v_x, v_y, m, n - 1, c, l);
    int n_iter = 0;
    while (ns.next_data(mpi)) {
        std::cout << "Iteration: " << ++n_iter << std::endl;
        ns.process_data(mpi);
    }

#ifdef MPI_ON
    MPI_Finalize();
#endif
    auto end_timestamp = std::chrono::high_resolution_clock::now();
    auto total_dur = std::chrono::duration_cast<std::chrono::milliseconds>(end_timestamp - start_timestamp).count();
    auto total_dur_no_io = std::chrono::duration_cast<std::chrono::milliseconds>(end_timestamp - start_timestamp_no_io).count();
    if (mpi.rank == 0) {
        std::cout << "Total Execution Time: " << total_dur << " milliseconds" << std::endl;
        std::cout << "Total Execution Time (without I/O): " << total_dur_no_io << " milliseconds" << std::endl;
    }
}



