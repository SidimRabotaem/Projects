#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <cstdint>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept
    : buffer_(other.buffer_)
    , capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    Vector(size_t size)
    : data_(size)
    , size_(size){
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
    : data_(other.size_)
    , size_(other.size_) {
       std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept 
    : data_(std::move(other.data_))
    , size_(other.size_) {
        other.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector temp(rhs);
                Swap(temp);
            } else {
                Realloc(rhs);
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        assert(index < size_);
        return data_[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);

        if constexpr(std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        } else if (new_size > size_) {
            if (new_size > data_.Capacity()) {
                RawMemory<T> new_data(new_size);

                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
                } else {
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                }

                std::uninitialized_value_construct_n(new_data.GetAddress() + size_, new_size - size_);

                std::destroy_n(data_.GetAddress(), size_);
                data_.Swap(new_data);
            } else {
                std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            }
            size_ = new_size;
        }
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
            std::destroy_at(data_ + size_);
        }
    }

template <typename... Args>
T& EmplaceBack(Args&&... args) {
    if (size_ < data_.Capacity()) {
        new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return data_[size_ - 1];
    }

    size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
    RawMemory<T> new_data(new_capacity);

    T* new_element_ptr = new_data + size_;
    new (new_element_ptr) T(std::forward<Args>(args)...);

    try {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
    } catch (...) {
        new_element_ptr->~T(); 
        throw;
    }

    std::destroy_n(data_.GetAddress(), size_);
    
    data_.Swap(new_data);
    ++size_;

    return data_[size_ - 1];
}

    iterator begin() noexcept { return data_.GetAddress(); }
    iterator end() noexcept { return data_.GetAddress() + size_; }
    const_iterator begin() const noexcept { return data_.GetAddress(); }
    const_iterator end() const noexcept { return data_.GetAddress() + size_; }
    const_iterator cbegin() const noexcept { return data_.GetAddress(); }
    const_iterator cend() const noexcept { return data_.GetAddress() + size_; }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

template <typename... Args>
iterator Emplace(const_iterator pos, Args&&... args) {
    assert(pos >= begin() && pos <= end());
    size_t idx = static_cast<size_t>(pos - data_.GetAddress());

    if (idx > size_) {
        throw std::out_of_range("Index out of range");
    }

    if (size_ == data_.Capacity()) {
        RawMemory<T> new_buf(size_ == 0 ? 1 : 2 * size_);

        T* nb = new_buf.GetAddress();
        T* ob = data_.GetAddress();
        size_t cnt = 0;

        try {
            if (idx > 0) {
                std::uninitialized_copy_n(
                    std::make_move_iterator(ob),
                    idx,
                    nb
                );
                cnt += idx;
            }

            new (nb + idx) T(std::forward<Args>(args)...);
            cnt += 1;

            if (size_ > idx) {
                std::uninitialized_copy_n(
                    std::make_move_iterator(ob + idx),
                    size_ - idx,
                    nb + idx + 1
                );
                cnt += (size_ - idx);
            }
        } catch (...) {
            std::destroy_n(nb, cnt);
            throw;
        }

        std::destroy_n(ob, size_);
        data_.Swap(new_buf);
    } else {
        T* buf = data_.GetAddress();
        T tmp(std::forward<Args>(args)...);

        if (idx < size_) {
            new (buf + size_) T(std::move(buf[size_ - 1]));

            try {
                std::move_backward(buf + idx, buf + size_ - 1, buf + size_);
            } catch (...) {
                buf[size_].~T();
                throw;
            }

            buf[idx] = std::move(tmp);
        } else {
            new (buf + size_) T(std::forward<Args>(args)...);
        }
    }

    ++size_;
    return data_.GetAddress() + idx;
}

    iterator Erase(const_iterator pos) {
        assert(pos >= begin() && pos <= end());
        size_t index = pos - data_.GetAddress();
        if (index >= size_) throw std::out_of_range("Index out of range");

        std::move(data_.GetAddress() + index + 1, data_.GetAddress() + size_, data_.GetAddress() + index);

        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
        return data_.GetAddress() + index;
    }

void Realloc(const Vector& rhs) {
    T* dst = data_.GetAddress();
    const T* src = rhs.data_.GetAddress();

    if (rhs.size_ < size_) {
        std::destroy_n(dst + rhs.size_, size_ - rhs.size_);
    }

    size_t common = std::min(size_, rhs.size_);
    std::copy(src, src + common, dst);

    if (rhs.size_ > size_) {
        std::uninitialized_copy_n(src + common, rhs.size_ - common, dst + common);
    }

    size_ = rhs.size_;
}

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i < n; ++i) {
            buf[i].~T();
        }
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};

