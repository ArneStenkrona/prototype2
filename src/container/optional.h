#ifndef PRT_OPTIONAL_H
#define PRT_OPTIONAL_H

namespace prt {
    template<typename T>
    class optional {
    public:
        constexpr optional();
    private:   
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
bool operator<=(const T& lhs, const prt::optional<U>& rhs) {
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