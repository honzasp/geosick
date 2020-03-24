#pragma once
#include <iterator>

namespace geosick {

template<class Iterator>
class Slice {
public:
    using reference = typename std::iterator_traits<Iterator>::reference;
    using iterator = Iterator;

private:
    iterator m_begin;
    iterator m_end;

public:
    Slice() = default;
    Slice(iterator begin, iterator end)
    : m_begin(begin), m_end(end)
    { }

    size_t size() const {
        return m_end - m_begin;
    }

    reference at(const size_t idx) const {
        if (idx >= this->size()) {
            throw std::out_of_range("Slice at out of range");
        }
        return *(m_begin + idx);
    }

    reference operator[](const size_t idx) const {
        return *(m_begin + idx);
    }

    iterator begin() const {
        return m_begin;
    }

    iterator end() const {
        return m_end;
    }

    reference front() const {
        assert(size() != 0);
        return *(m_begin);
    }

    reference back() const {
        assert(size() != 0);
        return *(m_begin + size() - 1);
    }
};

template <typename Container>
using ContainerSlice = Slice<typename Container::iterator>;

template<class Container>
auto make_slice(const Container& cont) {
    return Slice(cont.begin(), cont.end());
}

template <typename T>
using ArrayView = Slice<const T*>;

template<class T>
auto make_view(const T* begin, const T* end) {
    return Slice<const T*>(begin, end);
}

template<class Container>
auto make_view(const Container& cont) {
    using Value = typename Container::value_type;
    return Slice<const Value*>(cont.data(), cont.data()+cont.size());
}

}
