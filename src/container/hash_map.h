#ifndef PRT_HASH_MAP_H
#define PRT_HASH_MAP_H

#include "src/memory/container_allocator.h"
#include "src/container/vector.h"

namespace prt {
    template<typename K, typename V> class hash_map;

    template<typename K, typename V>
    class hash_map_node {
    public:
        hash_map_node(): _present(false) {}

        //~hash_map_node() {
        //    if (_present) {
        //        key().~K();
        //        value().~V();
        //    }
        //}

        K& key() {
            assert(_present);
            return reinterpret_cast<K&>(_key);
        }
        V& value() {
            assert(_present);
            return reinterpret_cast<V&>(_value);
        }
        K const & key() const {
            assert(_present);
            return reinterpret_cast<K const&>(_key);
        }
        V const & value() const {
            assert(_present);
            return reinterpret_cast<V const&>(_value);
        }
        
    private:
        alignas(K) char _key[sizeof(K)];
        alignas(V) char _value[sizeof(V)];

        bool _present;

        hash_map_node(const K& key, const V& value): _present(true) {
            new (&_key) K(key);
            new (&_value) V(value);
        }
        friend class hash_map<K, V>;
    };

    template<typename K, typename V>
    class hash_map {
    public:
        class iterator;

        hash_map()
        : hash_map(ContainerAllocator::getDefaultContainerAllocator()) {}

        hash_map(ContainerAllocator& allocator) 
        : _vector(allocator), _size(0) {
            increaseCapacity(2);
        }
        
        void insert(const K& key, const V& value) {
            if (2 * _size > _vector.capacity()) {
                increaseCapacity(2 * _vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while (_vector[ind]._present && _vector[ind].key() != key) {
                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
            }

            if (!_vector[ind]._present) {
                _size++;
            }
            _vector[ind] = hash_map_node<K, V>(key, value);
        }

        void erase(const K& key) {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            // Loop through until first gap.
            // Worst case is O(n), though average is O(1)
            while (_vector[ind]._present) {
                if (_vector[ind].key() == key) {
                    // remove the value and shift appropriate
                    // nodes to avoid invalidating future searches
                    _size--;
                    _vector[ind]._present = false;
                    size_t next = ind == _vector.size() - 1 ? 0 : ind + 1;
                    // Loop through nodes to be shifted
                    // Worst case is O(n), though average is O(1)
                    while (_vector[next]._present) {
                        // Store the value as temp and remove it
                        K& tempK = _vector[next].key();
                        V& tempV = _vector[next].value();
                        _vector[next]._present = false;
                        size_t nextInd = hashIndex(tempK);
                        // Reinsert
                        // Worst case is O(n), though average is O(1)
                        while (_vector[nextInd]._present) {
                            nextInd = nextInd == _vector.size() - 1 ? 0 : nextInd + 1;
                        }
                        _vector[nextInd] = hash_map_node<K, V>(tempK, tempV);
                        
                        next = next == _vector.size() - 1 ? 0 : next + 1;
                    }
                    // return
                    return;
                }
                counter++;
                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
            }
        }

        V & operator [](const K& key) {
            if (2 * _size > _vector.capacity()) {
                increaseCapacity(2 * _vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while(_vector[ind]._present && _vector[ind].key() != key) {
                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
            }

            if (!_vector[ind]._present) {
                _vector[ind] = hash_map_node<K, V>(key, V());
                _size++;
            }
            return _vector[ind].value();
        }

        inline size_t size() const { return _size; }
        inline bool empty() const { return _size == 0; }

        iterator find(const K& key) {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            
            while(_vector[ind]._present && counter < _vector.size()) {
                if (_vector[ind].key() == key) {
                    return iterator(&_vector[ind], _vector.end());
                }

                ind = ind == _vector.size() - 1 ? 0 : ind + 1;
                counter++;
            }
            return end();
        }

        class iterator {
        public:        
            iterator(hash_map_node<K, V>* current, hash_map_node<K, V>* end)
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

            hash_map_node<K, V>& operator*() { return *_current; }
            hash_map_node<K, V>* operator->() { return _current; }
        private:
            hash_map_node<K, V>* _current;
            hash_map_node<K, V>* _end;    
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
        vector<hash_map_node<K, V> > _vector;
        // number of key value pairs in table
        size_t _size;

        std::hash<K> hash_fn;
        // Todo: make distribution more uniform
        inline size_t hashIndex(const K& key) const { return hash_fn(key) % _vector.size(); }
    
        void increaseCapacity(size_t capacity) {
            prt::vector<hash_map_node<K, V> > temp;

            temp.resize(_size);

            size_t count = 0;
            for (auto it = begin(); it != end(); it++) {
                temp[count++] = *it;
            }
            _vector.clear();
            _vector.resize(capacity);

            for (size_t i = 0; i < temp.size(); i++) {
                size_t ind = hashIndex(temp[i].key());

                while (_vector[ind]._present) {
                    ind = ind == _vector.size() - 1 ? 0 : ind + 1;
                }
                _vector[ind] = temp[i];
            }
        }
    };
};

#endif