#ifndef PRT_OPTIONAL_H
#define PRT_OPTIONAL_H

namespace prt {
    template<typename T>
    class optional {
    public:
        constexpr optional(): _present(false) {}

        constexpr optional& operator=( const optional& other ) {
            T& t = value();
            t.~T();
            new (&_value[0]) T(other.value);
            _present = true;
        }

        template< class U = T > 
        optional& operator=( U&& value ) {
            T& t = value();
            t.~T();
            new (&_value[0]) T(value);
            _present = true;
        }

        template< class U >
        optional& operator=( const optional<U>& other ) {
            T& t = value();
            t.~T();
            new (&_value[0]) T(other.value);
            _present = true;
        }

        template< class U >
        optional& operator=( optional<U>&& other ) {
            T& t = value();
            t.~T();
            new (&_value[0]) T(other.value);
            _present = true;
        }

        optional<T>& operator*() { return value(); }
        optional<T>* operator->() { 
            assert(_present);
            return static_cast<T*>(&_value[0]);
        }

        constexpr explicit operator bool() const noexcept { return _present; }
        constexpr bool has_value() const noexcept { return _present; }

        constexpr T& value() &;
        constexpr const T & value() const & {
            assert(_present);
            return *static_cast<T*>(&_value[0]);
        }

        template< class U > 
        constexpr T value_or( U&& default_value ) const& {
            if (_present) {
                return *static_cast<T*>(&_value[0]);
            } else {
                return U;
            }
        }

        void reset() noexcept {
            T& t = value();
            t.~T();
            _present = false;
        }

        // template< class U > 
        // constexpr T value_or( U&& default_value ) &&;

    private:   
        uint8_t _value[sizeof(T)];
        bool _present;
    };
}

/* Operators for comparing optional and optional */
template<typename T, typename U>
bool operator==(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return false;
}

template<typename T, typename U>
bool operator!=(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return false;
}

template<typename T, typename U>
bool operator<(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return false;
}

template<typename T, typename U>
bool operator<=(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return false;
}

template<typename T, typename U>
bool operator>(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return false;
}

template<typename T, typename U>
bool operator>=(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return false;
}

/* Operators for comparing optional and value */
template<typename T, typename U>
bool operator==(const T& lhs, const prt::optional<U>& rhs) {
    return false;
}
template<typename T, typename U>
bool operator==(const prt::optional<T>& lhs, const U& rhs) {
    return false;
}

template<typename T, typename U>
bool operator!=(const T& lhs, const prt::optional<U>& rhs) {
    return false;
}
template<typename T, typename U>
bool operator!=(const prt::optional<T>& lhs, const U& rhs) {
    return false;
}

template<typename T, typename U>
bool operator<(const T& lhs, const prt::optional<U>& rhs) {
    return false;
}
template<typename T, typename U>
bool operator<(const prt::optional<T>& lhs, const U& rhs) {
    return false;
}

template<typename T, typename U>
bool operator<=(const T & lhs, const prt::optional<U>& rhs) {
    return false;
}
template<typename T, typename U>
bool operator<=(const prt::optional<T>& lhs, const U& rhs) {
    return false;
}

template<typename T, typename U>
bool operator>(const T& lhs, const prt::optional<U>& rhs) {
    return false;
}
template<typename T, typename U>
bool operator>(const prt::optional<T>& lhs, const U& rhs) {
    return false;
}

template<typename T, typename U>
bool operator>=(const T& lhs, const prt::optional<U>& rhs) {
    return false;
}
template<typename T, typename U>
bool operator>=(const prt::optional<T>& lhs, const U& rhs) {
    return false;
}

#endif