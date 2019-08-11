#ifndef PRT_OPTIONAL_H
#define PRT_OPTIONAL_H

namespace prt {
    template<typename T>
    class optional {
    public:
        constexpr optional(): _present(false) {}

        template < class U = T >
        constexpr optional( U&& value ): _present(true) {
            new (&_value[0]) T(value);
            _present = true;
        }

        constexpr optional( const optional& other ): _present(false) {
            if (other.has_value()) {
                new (&_value[0]) T(other.value());
                _present = true;
            }
        }

        constexpr optional( optional&& other ): _present(false) {
            if (other.has_value()) {
                new (&_value[0]) T(other.value());
                _present = true;
            }
        }

        constexpr optional& operator=( const optional& other ) {
            if (_present) {
                T& t = value();
                t.~T();
            }
            new (&_value[0]) T(other.value);
            _present = true;
        }

        template< class U = T > 
        optional& operator=( U&& value ) {
            if (_present) {
                T& t = this->value();
                t.~T();
            }
            new (&_value[0]) T(value);
            _present = true;
            return *this;
        }

        template< class U >
        optional& operator=( const optional<U>& other ) {
            if (_present) {
                T& t = value();
                t.~T();
            }
            new (&_value[0]) T(other.value);
            _present = true;
            return *this;
        }

        template< class U >
        optional& operator=( optional<U>&& other ) {
            T& t = value();
            t.~T();
            new (&_value[0]) T(other.value);
            _present = true;
            return *this;
        }

        optional<T>& operator*() { return value(); }
        optional<T>* operator->() { 
            assert(_present);
            return reinterpret_cast<T*>(&_value[0]);
        }

        constexpr explicit operator bool() const noexcept { return _present; }
        constexpr bool has_value() const noexcept { return _present; }

        constexpr T& value() & {
            assert(_present);
            return *reinterpret_cast<T*>(&_value[0]);
        }
        constexpr const T & value() const & {
            assert(_present);
            return *reinterpret_cast<const T*>(&_value[0]);
        }

        template< class U > 
        constexpr T value_or( U&& default_value ) const& {
            if (_present) {
                return *reinterpret_cast<const T*>(&_value[0]);
            } else {
                return default_value;
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
        alignas(alignof(T)) char _value[sizeof(T)];
        bool _present;
    };
}

/* Operators for comparing optional and optional */
template<typename T, typename U>
bool operator==(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    if (!bool(lhs) || !bool(rhs)) {
        return false;
    }
    return lhs.value() == rhs.value();
}

template<typename T, typename U>
bool operator!=(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    return !(lhs == rhs);
}

template<typename T, typename U>
bool operator<(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    if (!bool(rhs)) {
        return false;
    } else if (!bool(lhs)) {
        return true;
    } 
    return *lhs < *rhs;
}

template<typename T, typename U>
bool operator<=(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    if (!bool(rhs)) {
        return false;
    } else if (!bool(lhs)) {
        return true;
    } 
    return *lhs <= *rhs;
}

template<typename T, typename U>
bool operator>(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    if (!bool(lhs)) {
        return false;
    } else if (!bool(rhs)) {
        return true;
    } 
    return *lhs > *rhs;
}

template<typename T, typename U>
bool operator>=(const prt::optional<T>& lhs, const prt::optional<U>& rhs) {
    if (!bool(lhs)) {
        return false;
    } else if (!bool(rhs)) {
        return true;
    } 
    return *lhs >= *rhs;
}

/* Operators for comparing optional and value */
template<typename T, typename U>
bool operator==(const T& value, const prt::optional<U>& opt) {
    return bool(opt) ? value == *opt : false;
}
template<typename T, typename U>
bool operator==(const prt::optional<T>& opt, const U& value) {
    return bool(opt) ? *opt == value : false;
}

template<typename T, typename U>
bool operator!=(const T& value, const prt::optional<U>& opt) {
    return bool(opt) ? value != *opt : false;
}
template<typename T, typename U>
bool operator!=(const prt::optional<T>& opt, const U& value) {
    return bool(opt) ? *opt != value : false;
}

template<typename T, typename U>
bool operator<(const T& value, const prt::optional<U>& opt) {
    return bool(opt) ? value < *opt : false;
}
template<typename T, typename U>
bool operator<(const prt::optional<T>& opt, const U& value) {
    return bool(opt) ? *opt < value : false;
}

template<typename T, typename U>
bool operator<=(const T & value, const prt::optional<U>& opt) {
    return bool(opt) ? value <= *opt : false;
}
template<typename T, typename U>
bool operator<=(const prt::optional<T>& opt, const U& value) {
    return bool(opt) ? *opt <= value : false;
}

template<typename T, typename U>
bool operator>(const T& value, const prt::optional<U>& opt) {
    return bool(opt) ? value > *opt : false;
}
template<typename T, typename U>
bool operator>(const prt::optional<T>& opt, const U& value) {
    return bool(opt) ? *opt > value : false;
}

template<typename T, typename U>
bool operator>=(const T& value, const prt::optional<U>& opt) {
    return bool(opt) ? value >= *opt : false;
}
template<typename T, typename U>
bool operator>=(const prt::optional<T>& opt, const U& value) {
    return bool(opt) ? *opt >= value : false;
}

#endif