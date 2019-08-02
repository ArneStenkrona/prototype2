#ifndef PRT_HASH_TABLE_H
#define PRT_HASH_TABLE_H

#include "src/memory/container_allocator.h"
#include "src/container/vector.h"

namespace prt {
    template<typename K, typename V>
    class HashNode {
    public:
        K key;
        V value;

        bool present;

        HashNode(): present(false) {}
        HashNode(K key, V value): key(key), value(value), present(true) {}
    };

    template<typename K, typename V>
    class HashTable {
    public:
        struct iterator;

        HashTable(ContainerAllocator& allocator) 
            : _vector(allocator) {}
        
        void insert(const K& key, const V& value) {
            if (2 * _size > _vector.capacity()) {
                _vector.resize(2 * _vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while(_vector[ind].present && _vector[ind].key != key) {
                ind = ind == _vector.size - 1 ? 0 : ind + 1;
            }

            if (!_vector[ind].present) {
                _size++;
            }
            _vector[ind] = HashNode<K, V>(key, value);
        }
        // void update(const K& key, const V& value) {
        // }
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

        iterator find(const K& key) {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            
            while(_vector[ind].present && counter < _vector.size()) {
                
                if (_vector[ind].key == key) {
                    return iterator(&_vector[ind].value, _vector.end());
                }

                ind = ind == _vector.size - 1 ? 0 : ind + 1;
                counter++;
            }
            return end();
        }

        struct iterator {
            HashNode<K, V>* _current;
            HashNode<K, V>* _end;            
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
            HashNode<K, V>& operator*() { return &_current; }
        };
        
        const iterator begin() {
            for (size_t i = 0; i < _vector.size(); i++) {
                if (_vector[i].present) {
                    return iterator(&_vector[i], _vector.end());
                }
            }
        }
        const iterator end() {
            return iterator(_vector.end(), _vector.end());
        }

    private:
        // vector to store the elements.
        vector<HashNode<K, V> > _vector;
        // number of key value pairs in table
        size_t _size;

        inline size_t hashIndex(K key) const { return hash(key) % _vector.size(); }

        
    };
};

#endif