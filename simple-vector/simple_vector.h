#pragma once

#include "array_ptr.h"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <stdexcept>

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(const size_t capacity) : reserve_(capacity) {}

    size_t GetReserve() const noexcept {
        return reserve_;
    }

private:
    size_t reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : data_(size)
        , capacity_(size)
        , size_(size) {
        std::generate(begin(), end(), []() {return Type(); });
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : data_(size)
        , capacity_(size)
        , size_(size) {
        std::fill_n(begin(), size, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : data_(init.size())
        , capacity_(init.size())
        , size_(init.size()) {
        std::copy(init.begin(), init.end(), data_.Get());
    }

    SimpleVector(const SimpleVector& other)
        : data_(other.size_)
        , capacity_(other.size_)
        , size_(other.size_) {
        std::copy(other.begin(), other.end(), data_.Get());
    }

    SimpleVector(SimpleVector&& other) noexcept
        : data_(std::move(other.data_))
        , capacity_(std::exchange(other.capacity_, 0))
        , size_(std::exchange(other.size_, 0)) {
    }

    explicit SimpleVector(ReserveProxyObj obj)
        : data_(obj.GetReserve())
        , capacity_(obj.GetReserve()) {
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }
        return data_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), end(), tmp.Get());
            data_.swap(tmp);
            capacity_ = new_capacity;
        }
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            Reserve(CalculateNewCapacity(new_size));
        }
        if (new_size > size_) {
            std::generate(end(), begin() + new_size, []() {return Type(); });
        }
        size_ = new_size;
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector rhs_copy(rhs);
            swap(rhs_copy);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this != &rhs) {
            SimpleVector rhs_copy(std::move(rhs));
            swap(rhs_copy);
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        Emplace(end(), item);
    }

    void PushBack(Type&& item) {
        Emplace(end(), std::move(item));
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        return Emplace(pos, value);
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        return Emplace(pos, std::move(value));
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(size_ != 0);
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        const auto ptr = Iterator(pos);
        if (size_) {
            std::move(ptr + 1, end(), ptr);
            --size_;
        }
        return ptr;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        data_.swap(other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return end();
    }

private:
    ArrayPtr<Type> data_;
    size_t capacity_ = 0;
    size_t size_ = 0;

    size_t CalculateNewCapacity(const size_t new_size) const {
        return std::max(new_size, capacity_ * 2);
    }

    template<typename ValType>
    Iterator Emplace(ConstIterator pos, ValType&& value) {
        const auto index = pos - cbegin();
        if (size_ == capacity_) {
            assert(size_ != SIZE_MAX);
            Reserve(CalculateNewCapacity(size_ + 1));
        }
        Type* const ptr = begin() + index;
        Type* const last = end();
        if (ptr != last) {
            std::move_backward(ptr, last, last + 1);
        }
        *ptr = std::forward<ValType>(value);
        ++size_;
        return ptr;
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize()
        && std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}