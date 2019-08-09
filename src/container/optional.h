#ifndef PRT_OPTIONAL_H
#define PRT_OPTIONAL_H

namespace prt {
    template<typename T>
    class optional {
    public:
        constexpr optional();

        optional<T>& operator*() { return *_current; }
        optional<T>* operator->() { return _current; }

        constexpr explicit operator bool() const noexcept {}
        constexpr bool has_value() const noexcept {}

        constexpr T& value() &;
        constexpr const T & value() const &;

        template< class U > 
        constexpr T value_or( U&& default_value ) const&;

        template< class U > 
        constexpr T value_or( U&& default_value ) &&;
        
    private:   
        T _value;
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