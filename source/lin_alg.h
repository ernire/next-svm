//
// Created by Ernir Erlingsson on 8.3.2021.
//

#ifndef NEXT_SVM_LIN_ALG_H
#define NEXT_SVM_LIN_ALG_H

namespace la {

    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    void transpose(std::vector<T> &matrix, int &m, int &n) noexcept {
        int m2 = n;
        int n2 = m;
        auto matrix2 = matrix;
        for (int i = 0; i < m; ++i) {
            for ()
//            for (int j = 0; j < n; ++j) {
//                if (i == j)
//            }
        }
        m = m2;
        n = n2;
        matrix = matrix2;
    }

}

#endif //NEXT_SVM_LIN_ALG_H
