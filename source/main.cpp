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


//bool count_lines_and_dimensions(const std::string &in_file, std::size_t &n_elem, std::size_t &n_dim) noexcept {
//    std::ifstream is(in_file);
//    if (!is.is_open()) {
//        return false;
//    }
//    std::string line, buf;
//    int cnt = 0;
//    n_dim = 0;
//    while (std::getline(is, line)) {
//        if (n_dim == 0) {
//            std::istringstream iss(line);
//            std::vector<std::string> results(std::istream_iterator<std::string>{iss},
//                    std::istream_iterator<std::string>());
//            n_dim = results.size();
//        }
//        ++cnt;
//    }
//    n_elem = cnt;
//    is.close();
//    return true;
//}
//
//void inline clip_alpha(float &alpha, float const upper, float const lower) {
//    if (alpha > upper) {
//        alpha = upper;
//    } else if (lower > alpha) {
//        alpha = lower;
//    }
//}
//
//bool read_csv(std::string const &in_file, std::vector<float> &v_raw_data, std::size_t const n_dim) {
//    std::ifstream is(in_file, std::ifstream::in);
//    if (!is.is_open())
//        return false;
//    std::string line, buf;
//    std::stringstream ss;
//    while (std::getline(is, line)) {
//        ss.str(std::string());
//        ss.clear();
//        ss << line;
//        for (int j = 0; j < n_dim; j++) {
//            ss >> buf;
//            v_raw_data.push_back(static_cast<float>(atof(buf.c_str())));
//        }
//    }
//    is.close();
//    return true;
//}
//
//
//
//float dot_product(std::vector<float> &v1, int m1, int n1, std::vector<float> &v2, int m2, int n2) {
//
//}
//
bool data_separate(std::vector<float> &v_raw_data, std::vector<float> &v_x, std::vector<int> &v_y,
        std::size_t const m, std::size_t const n) {
    v_x.clear();
    v_y.clear();
    v_x.reserve(m * ( n - 1));
    v_y.reserve(m);
    for (std::size_t i = 0; i < m; ++i) {
        v_y.push_back(static_cast<int>(v_raw_data[i * n]));
        for (std::size_t j = 1; j < n; ++j) {
            v_x.push_back(v_raw_data[i * n + j]);
        }
    }
}
//
//void print_matrix(std::vector<float> v_matrix, std::size_t const m, std::size_t const n) {
//    for (std::size_t i = 0; i < m; ++i) {
//        for (std::size_t j = 0; j < n; ++j) {
//            std::cout << v_matrix[i * n + j] << " ";
//        }
//        std::cout << std::endl;
//    }
//}
//
//void simpleSMO(std::vector<float> &v_x, std::vector<float> v_y, std::size_t const m, std::size_t const n, float const c, float const tol, int const max_pass) {
//    float b = 0;
//    std::vector<float> alpha(m, 0);
//
//
//}


void read_input_bin(const std::string &in_file, h_vec<float> &v_points, int &m, int &n,
        int const n_nodes, int const i_node) noexcept {
    std::ifstream ifs(in_file, std::ios::in | std::ifstream::binary);
    ifs.read((char *) &m, sizeof(int));
    ifs.read((char *) &n, sizeof(int));
    auto size = magma_util::get_block_size(i_node, m, n_nodes);
    auto offset = magma_util::get_block_offset(i_node, m, n_nodes);
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
    data_separate(v_raw_data, v_x, v_y, m, n);
    v_raw_data.clear();
    v_raw_data.shrink_to_fit();
    std::cout << "Found " << m << " elements with " << n << " dimensions." << std::endl;

    auto start_timestamp_no_io = std::chrono::high_resolution_clock::now();

    next_svm ns(v_x, v_y, m, n - 1, c, l);

    magma_util::print_v("x: ", &ns.v_x[0], n);
    magma_util::print_v("y: ", &ns.v_y[0], 10);

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

    /*
    std::string input_file = "..\\input\\my_data.txt";
    std::size_t m, n;
    count_lines_and_dimensions(input_file, m, n);
    std::cout << "Detected " << m << " samples with " << n << " features" << std::endl;
    std::vector<float> v_raw_data;
    read_csv(input_file, v_raw_data, n);
    // data
    std::vector<float> v_x;
    // class
    std::vector<float> v_y;
    std::cout << "read " << v_raw_data.size() << " float(s)" << std::endl;
    data_separate(v_raw_data, v_x, v_y, m, n);
    std::cout << "x:" << std::endl;
//    print_matrix(v_x, m, n - 1);
    std::cout << "y:" << std::endl;
//    print_matrix(v_y, m, 1);
    */

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


