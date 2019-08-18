#ifndef PRT_HASH_TABLE_H
#define PRT_HASH_TABLE_H

#include "src/memory/container_allocator.h"
#include "src/container/vector.h"

namespace prt {
    template<typename T> class hash_set;

    template<typename T>
    class hash_set_node {
    public:
        hash_set_node(): _present(false) {}

        ~hash_set_node() {
            if (_present) {
                value().~T();
            }
        }
        T& value() {
            assert(_present);
            return *reinterpret_cast<T*>(&_value[0]);
        }
        
    private:
        alignas(T) char _value[sizeof(T)];

        bool _present;

        hash_set_node(const T& value): _present(true) {
            new (&_value) T(value);
        }
        friend class hash_set<T>;
    };

    template<typename T>
    class hash_set {
    public:
        class iterator;

        hash_set()
        : hash_set(ContainerAllocator::getDefaultContainerAllocator()) {}

        hash_set(ContainerAllocator& allocator) 
        : _vector(allocator), _size(0) {
            _vector.resize(1);
        }
        
        void insert(const T& value) {

            if (2 * _size > _vector.capacity()) {
                increaseCapacity(2 * _vector.capacity());
            }
            
            size_t ind = hashIndex(value);

            while (_vector[ind]._present && _vector[ind].value() != value) {
                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
            }

            if (!_vector[ind]._present) {
                _size++;
                _vector[ind] = hash_set_node<T>(value);
            }   
        }

        // WILL IMPLEMENT WHEN NEEDED void remove(const T& value) {}

        inline size_t size() const { return _size; }

        iterator find(const T& value) {
            size_t ind = hashIndex(value);
            size_t counter = 0;
            
            while (_vector[ind]._present && counter < _vector.size()) {
                if (_vector[ind].value() == value) {
                    return iterator(&_vector[ind], _vector.end());
                }

                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
                counter++;
            }
            return end();
        }

        class iterator {
        public:        
            iterator(hash_set_node<T>* current, hash_set_node<T>* end)
            : _current(current), _end(end) {}

            const iterator& operator++() {
                while (_current != _end) {
                    _current++;
                    if (_current == _end || _current->_present) {
                        return *this;
                    } 
                }
                return *this; 
            }

            iterator operator++(int) {
                iterator result = *this; 
                ++(*this); 
                return result;
            }

            bool operator==(const iterator& other) {
                return _current == other._current;
            }

            bool operator!=(const iterator& other) {
                return !(*this == other);
            }

            hash_set_node<T>& operator*() { return *_current; }
            hash_set_node<T>* operator->() { return _current; }
        private:
            hash_set_node<T>* _current;
            hash_set_node<T>* _end;    
        };
        
        iterator begin() {
            for (size_t i = 0; i < _vector.size(); i++) {
                if (_vector[i]._present) {
                    return iterator(&_vector[i], _vector.end());
                }
            }
            return end();
        }
        iterator end() {
            return iterator(_vector.end(), _vector.end());
        }

    private:
        // vector to store the elements.
        vector<hash_set_node<T> > _vector;
        // number of key value pairs in table
        size_t _size;

        std::hash<T> hash_fn;

        inline size_t hashIndex(T value) const { return hash_fn(value) % _vector.size(); }
    
        void increaseCapacity(size_t capacity) {
            prt::vector<hash_set_node<T> > temp;

            temp.resize(_size);

            size_t count = 0;
            for (auto it = begin(); it != end(); it++) {
                temp[count++] = *it;
            }
            _vector.clear();
            _vector.resize(2 * capacity);

            for (size_t i = 0; i < temp.size(); i++) {
                size_t ind = hashIndex(temp[i].value());

                while (_vector[ind]._present && _vector[ind].value() != temp[i].value()) {
                    ind = ind == _vector.size() - 1 ? 0 : ind + 1;
                }
                _vector[ind] = temp[i];
            }
        }
    };
};

#endif