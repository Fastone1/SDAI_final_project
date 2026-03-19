#ifndef ARRAY2D_HPP
#define ARRAY2D_HPP

#include <array>
#include <cstddef> // for std::size_t
// #include <stdexcept> // for std::out_of_range

template <typename T, std::size_t Rows, std::size_t Cols>
class Array2D {
    private:
        std::array<T, Rows * Cols> data;
    public:
        constexpr Array2D() = default;
        constexpr Array2D(const std::array<T, Rows * Cols>& data) : data(data) {}
        constexpr Array2D(const Array2D<T, Rows, Cols>& other) : data(other.data) {}

        constexpr Array2D(const std::array<std::array<T, Cols>, Rows>& other) {
            for (std::size_t i = 0; i < Rows; ++i) {
                for (std::size_t j = 0; j < Cols; ++j) {
                    data[i * Cols + j] = other[i][j];
                }
            }
        }

        T& at(std::size_t row, std::size_t col) {
            /* if (row >= Rows || col >= Cols) {
                throw std::out_of_range("Index out of range in at() method. Size: " + std::to_string(Rows) + "x" + std::to_string(Cols) + ", Accessed: (" + std::to_string(row) + ", " + std::to_string(col) + ")");
            } */
            return data[row * Cols + col];
        }

        const T& at(std::size_t row, std::size_t col) const {
            /* if (row >= Rows || col >= Cols) {
                throw std::out_of_range("Index out of range in at() method. Size: " + std::to_string(Rows) + "x" + std::to_string(Cols) + ", Accessed: (" + std::to_string(row) + ", " + std::to_string(col) + ")");
            } */
            return data[row * Cols + col];
        }

        void insert(std::size_t row, std::size_t col, const T& value) {
            /* if (row >= Rows || col >= Cols) {
                throw std::out_of_range("Index out of range in insert() method. Size: " + std::to_string(Rows) + "x" + std::to_string(Cols) + ", Accessed: (" + std::to_string(row) + ", " + std::to_string(col) + ")");
            } */
            data[row * Cols + col] = value;
        }

        void insert(std::size_t row, const std::array<T, Cols>& value) {
            /* if (row >= Rows) {
                throw std::out_of_range("Index out of range in insert() method. Size: " + std::to_string(Rows) + "x" + std::to_string(Cols) + ", Accessed: (" + std::to_string(row) + ", 0)");
            } */
            for (std::size_t j = 0; j < Cols; ++j) {
                data[row * Cols + j] = value[j];
            }
        }

        void insert(const std::array<std::array<T, Cols>, Rows>& value) {
            for (std::size_t i = 0; i < Rows; ++i) {
                for (std::size_t j = 0; j < Cols; ++j) {
                    data[i * Cols + j] = value[i][j];
                }
            }
        }

        std::size_t size() const {
            return Rows * Cols;
        }

        std::size_t rows() const {
            return Rows;
        }

        std::size_t cols() const {
            return Cols;
        }

        std::array<T, Rows * Cols> get_data() const {
            return data;
        }

        void fill(const T& value) {
            data.fill(value);
        }

        void clear() {
            data.fill(T());
        }
};

#endif // ARRAY2D_HPP