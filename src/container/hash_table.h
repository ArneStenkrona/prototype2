#ifndef PRT_HASH_TABLE_H
#define PRT_HASH_TABLE_H

#include "src/memory/container_allocator.h"
#include "src/container/vector.h"
#include "src/container/array.h"

namespace prt {
    template<typename K, typename V> class HashTable;

    template<typename K, typename V>
    class HashNode {
    public:
        K key;
        V value;
        HashNode(): present(false) {}
    private:
        bool present;

        HashNode(K key, V value): key(key), value(value), present(true) {}
        friend class HashTable<K, V>;
    };

    template<typename K, typename V>
    class HashTable {
    public:
        class iterator;

        HashTable()
        : HashTable(ContainerAllocator::getDefaultContainerAllocator()) {}

        HashTable(ContainerAllocator& allocator) 
        : _vector(allocator), _size(0) {
            _vector.resize(1);
        }
        
        void insert(const K& key, const V& value) {
            if (2 * _size > _vector.capacity()) {
                increaseCapacity(2 * _vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while(_vector[ind].present && _vector[ind].key != key) {
                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
            }

            if (!_vector[ind].present) {
                _size++;
            }
            _vector[ind] = HashNode<K, V>(key, value);
        }

        void remove(const K& key) {
            size_t ind = hashIndex(key);

            while(_vector[ind].present) {
                if (_vector[ind].key == key) {
                    _vector[ind].present = false;
                    _size--;
                }
                ind = ind == _vector.size - 1 ? 0 : ind + 1;
            }
        }

        V & operator [](const K& key) {
            if (2 * _size > _vector.capacity()) {
                increaseCapacity(2 * _vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while(_vector[ind].present && _vector[ind].key != key) {
                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
            }

            if (!_vector[ind].present) {
                _vector[ind] = HashNode<K, V>(key, V());
                _size++;
            }
            return _vector[ind].value;
        }

        inline size_t size() const { return _size; }

        iterator find(const K& key) {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            
            while(_vector[ind].present && counter < _vector.size()) {
                
                if (_vector[ind].key == key) {
                    return iterator(&_vector[ind], _vector.end());
                }

                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
                counter++;
            }
            return end();
        }

        class iterator {
        public:        
            iterator(HashNode<K, V>* current, HashNode<K, V>* end)
            : _current(current), _end(end) {}

            const iterator& operator++() { 
                while (_current != _end) {
                    _current++;
                    if (_current->present) {
                        break;
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

            HashNode<K, V>& operator*() { return *_current; }
            HashNode<K, V>* operator->() { return _current; }
        private:
            HashNode<K, V>* _current;
            HashNode<K, V>* _end;    
        };
        
        iterator begin() {
            for (size_t i = 0; i < _vector.size(); i++) {
                if (_vector[i].present) {
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
        vector<HashNode<K, V> > _vector;
        // number of key value pairs in table
        size_t _size;

        inline size_t hashIndex(K key) const { return std::hash<K>{}(key) % _vector.size(); }
    
        void increaseCapacity(size_t capacity) {
            prt::array<HashNode<K, V> > temp(_size);

            size_t count = 0;
            for (auto it = begin(); it != end(); it++) {
                temp[count++] = *it;
            }
            
            _vector.clear();
            _vector.resize(2 * capacity);

            for (size_t i = 0; i < temp.size(); i++) {
                size_t ind = hashIndex(temp[i].key);

                while(_vector[ind].present && _vector[ind].key != temp[i].key) {
                    ind = ind == _vector.size() - 1 ? 0 : ind + 1;
                }
                _vector[ind] = temp[i];
            }
        }
    };
};

#endif