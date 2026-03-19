#ifndef STATIC_VECTOR_HPP
#define STATIC_VECTOR_HPP

#include <cstddef> // for std::size_t
#include <vector>
#include <array>
#include <algorithm>

template <typename T, std::size_t N>
class StaticVector {
public:
    typedef T * iterator;
    typedef const T * const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    StaticVector() : size_(0) {}

    StaticVector(std::size_t size) : size_(size) {
        /* if (size > N) {
            throw std::out_of_range("Size exceeds capacity " + std::to_string(N));
        } */
    }

    StaticVector(const StaticVector<T, N>& other) : size_(other.size_) {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    StaticVector(const T (&array)[N]) : size_(N) {
        std::copy(std::begin(array), std::end(array), data_);
    }

    StaticVector(const std::vector<T>& vec) : size_(vec.size()) {
        /* if (size_ > N) {
            throw std::out_of_range("Vector size exceeds capacity " + std::to_string(N));
        } */
        std::copy(vec.begin(), vec.end(), data_);
    }

    StaticVector(const std::array<T, N>& arr) : size_(N) {
        std::copy(arr.begin(), arr.end(), data_);
    }

    StaticVector(StaticVector<T, N>&& other) noexcept : size_(other.size_) {
        std::move(other.data_, other.data_ + size_, data_);
        other.clear();
    }

    ~StaticVector() = default;

    StaticVector& operator=(const StaticVector<T, N>& other) {
        if (this != &other) {
            size_ = other.size_;
            std::copy(other.data_, other.data_ + size_, data_);
        }
        return *this;
    }

    StaticVector& operator=(const T (&array)[N]) {
        size_ = N;
        std::copy(std::begin(array), std::end(array), data_);
        return *this;
    }

    StaticVector& operator=(const std::array<T, N>& arr) {
        std::copy(arr.begin(), arr.end(), data_);
        size_ = N;
        return *this;
    }

    StaticVector& operator=(const std::vector<T>& vec) {
        size_ = vec.size();
        /* if (size_ > N) {
            throw std::out_of_range("Vector size exceeds capacity " + std::to_string(N));
        } */
        std::copy(vec.begin(), vec.end(), data_);
        return *this;
    }

    void clear() { size_ = 0; }

    bool empty() const { return size_ == 0; }

    void resize(std::size_t new_size) {
        /* if (new_size > N) {
            throw std::out_of_range("New size exceeds capacity " + std::to_string(N));
        } */
        size_ = new_size;
    }

    void push_back(const T& value) {
        if (size_ < N) {
            data_[size_] = value;
            ++size_;
        }/*  else {
            throw std::out_of_range("StaticVector of size " + std::to_string(N) + " is full");
        } */
    }

    void pop_back() {
        if (size_ > 0) {
            --size_;
        }/*  else {
            throw std::out_of_range("StaticVector is empty");
        } */
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (size_ < N) {
            data_[size_] = T(std::forward<Args>(args)...);
            ++size_;
        }/*  else {
            throw std::out_of_range("StaticVector of size " + std::to_string(N) + " is full");
        } */
    }

    void insert(std::size_t index, const T& value) {
        /* if (index > size_ ) {
            throw std::out_of_range("Index out of range: " + std::to_string(index) + ", size: " + std::to_string(size_) + ", capacity: " + std::to_string(N));
        }
        if (size_ >= N) {
            throw std::out_of_range("StaticVector of size " + std::to_string(N) + " is full");
        } */
        std::copy_backward(data_ + index, data_ + size_, data_ + size_ + 1);
        data_[index] = value;
        ++size_;
    }

    inline iterator insert(const_iterator __position, iterator __first, iterator __last) {
        std::size_t index = __position - data_;
        std::size_t count = __last - __first;
        /* if (size_ + count > N) {
            throw std::out_of_range("Insertion exceeds capacity " + std::to_string(N));
        } */
        std::copy_backward(data_ + index, data_ + size_, data_ + size_ + count);
        std::copy(__first, __last, data_ + index);
        size_ += count;
        return data_ + index;
    }

    template <std::size_t M>
    inline iterator append(StaticVector<T, M>& other) {
        /* if (size_ + other.size() > N) {
            throw std::out_of_range("Appending exceeds capacity " + std::to_string(N));
        } */
        std::copy(other.begin(), other.end(), data_ + size_);
        size_ += other.size();
        return data_ + size_;
    }
    inline iterator append(std::vector<T>& other) {
        /* if (size_ + other.size() > N) {
            throw std::out_of_range("Appending exceeds capacity " + std::to_string(N));
        } */
        std::copy(other.begin(), other.end(), data_ + size_);
        size_ += other.size();
        return data_ + size_;
    }

    template <std::size_t M>
    inline iterator append(const std::array<T, M>& other) {
        /* if (size_ + M > N) {
            throw std::out_of_range("Appending exceeds capacity " + std::to_string(N));
        } */
        std::copy(other.begin(), other.end(), data_ + size_);
        size_ += M;
        return data_ + size_;
    }

    std::size_t size() const { return size_; }

    std::size_t capacity() const { return N; }

    iterator begin() { return data_; }
    iterator end() { return data_ + size(); }
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    T& front() {
        /* if (size_ == 0) {
            throw std::out_of_range("StaticVector is empty");
        } */
        return data_[0];
    }

    T& back() {
        /* if (size_ == 0) {
            throw std::out_of_range("StaticVector is empty");
        } */
        return data_[size_ - 1];
    }

    const T & front() const {
        /* if (size_ == 0) {
            throw std::out_of_range("StaticVector is empty");
        } */
        return data_[0];
    }

    const T & back() const {
        /* if (size_ == 0) {
            throw std::out_of_range("StaticVector is empty");
        } */
        return data_[size_ - 1];
    }

    T& operator[](std::size_t index) {
        return data_[index];
    }

    const T& operator[](std::size_t index) const {
        return data_[index];
    }

private:
    T data_[N];
    std::size_t size_;
};

#endif // STATIC_VECTOR_HPP