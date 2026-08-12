#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace std;

class MNAMatrix {
public:
    int rows;
    int cols;
    vector<vector<double>> data;

    MNAMatrix(int r = 0, int c = 0, double val = 0.0)
        : rows(r), cols(c), data(r, vector<double>(c, val)) {}

    void resize(int r, int c, double val = 0.0) {
        rows = r;
        cols = c;
        data.assign(r, vector<double>(c, val));
    }

    vector<double>& operator[](int r) { return data[r]; }
    const vector<double>& operator[](int r) const { return data[r]; }

    static bool solve(const MNAMatrix& A, const vector<double>& rhs, vector<double>& x) {
        int n = A.rows;
        if (n == 0 || A.cols != n || static_cast<int>(rhs.size()) != n) {
            return false;
        }

        vector<vector<double>> aug(n, vector<double>(n + 1, 0.0));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                aug[i][j] = A[i][j];
            }
            aug[i][n] = rhs[i];
        }

        for (int i = 0; i < n; ++i) {
            int pivot = i;
            double maxVal = abs(aug[i][i]);
            for (int k = i + 1; k < n; ++k) {
                if (abs(aug[k][i]) > maxVal) {
                    maxVal = abs(aug[k][i]);
                    pivot = k;
                }
            }

            if (maxVal < 1e-12) return false;

            if (pivot != i) swap(aug[i], aug[pivot]);

            for (int k = i + 1; k < n; ++k) {
                double factor = aug[k][i] / aug[i][i];
                for (int j = i; j <= n; ++j) {
                    aug[k][j] -= factor * aug[i][j];
                }
            }
        }

        x.assign(n, 0.0);
        for (int i = n - 1; i >= 0; --i) {
            double sum = aug[i][n];
            for (int j = i + 1; j < n; ++j) {
                sum -= aug[i][j] * x[j];
            }
            x[i] = sum / aug[i][i];
        }

        return true;
    }
};

#endif
