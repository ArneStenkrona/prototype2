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

        // ~hash_map_node() {
        //    if (_present) {
        //        key().~K();
        //        value().~V();
        //    }
        // }

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

        bool present() const { return _present; }
        
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
        class const_iterator;

        hash_map()
        : hash_map(ContainerAllocator::getDefaultContainerAllocator()) {}

        hash_map(ContainerAllocator& allocator) 
        : m_vector(allocator), m_size(0) {
            increaseCapacity(2);
        }

        hash_map(hash_map const & other)
        : m_vector(other.m_vector) {
            m_size = other.m_size;
        }

        hash_map& operator=(hash_map const & other) {
            if (this != &other) {
                m_vector = other.m_vector;
                m_size = other.m_size;                
            } 
            return *this;
        }

        hash_map(hash_map && other)
        : m_vector(std::move(other.m_vector)) {
            m_size = other.m_size;
            other.m_size = 0;
        }

        hash_map& operator=(hash_map && other) {
            if (this != &other) {
                m_vector = std::move(other.m_vector);
                m_size = other.m_size;
                
                other.m_size = 0;
            } 
            return *this;
        }
        
        void insert(const K& key, const V& value) {
            if (2 * m_size > m_vector.capacity()) {
                increaseCapacity(2 * m_vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while (m_vector[ind]._present && m_vector[ind].key() != key) {
                ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
            }

            if (!m_vector[ind]._present) {
                m_size++;
            }
            m_vector[ind] = hash_map_node<K, V>(key, value);
        }

        void erase(const K& key) {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            // Loop through until first gap.
            // Worst case is O(n), though average is O(1)
            while (m_vector[ind]._present) {
                if (m_vector[ind].key() == key) {
                    // remove the value and shift appropriate
                    // nodes to avoid invalidating future searches
                    m_size--;
                    m_vector[ind]._present = false;
                    size_t next = ind == m_vector.size() - 1 ? 0 : ind + 1;
                    // Loop through nodes to be shifted
                    // Worst case is O(n), though average is O(1)
                    while (m_vector[next]._present) {
                        // Store the value as temp and remove it
                        K& tempK = m_vector[next].key();
                        V& tempV = m_vector[next].value();
                        m_vector[next]._present = false;
                        size_t nextInd = hashIndex(tempK);
                        // Reinsert
                        // Worst case is O(n), though average is O(1)
                        while (m_vector[nextInd]._present) {
                            nextInd = nextInd == m_vector.size() - 1 ? 0 : nextInd + 1;
                        }
                        m_vector[nextInd] = hash_map_node<K, V>(tempK, tempV);
                        
                        next = next == m_vector.size() - 1 ? 0 : next + 1;
                    }
                    // return
                    return;
                }
                counter++;
                ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
            }
        }

        V & operator [](const K& key) {
            if (2 * m_size > m_vector.capacity()) {
                increaseCapacity(2 * m_vector.capacity());
            }
            
            size_t ind = hashIndex(key);

            while(m_vector[ind]._present && m_vector[ind].key() != key) {
                ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
            }

            if (!m_vector[ind]._present) {
                m_vector[ind] = hash_map_node<K, V>(key, V());
                m_size++;
            }
            return m_vector[ind].value();
        }

        V const & operator [](const K& key) const {
            if (m_size == 0) assert(false);

            size_t ind = hashIndex(key);

            while(m_vector[ind]._present && m_vector[ind].key() != key) {
                ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
            }

            return m_vector[ind].value();
        }

        inline size_t size() const { return m_size; }
        inline bool empty() const { return m_size == 0; }
        
        const_iterator find(const K& key) const {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            
            while(m_vector[ind]._present && counter < m_vector.size()) {
                if (m_vector[ind].key() == key) {
                    return const_iterator(&m_vector[ind], m_vector.end());
                }

                ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
                ++counter;
            }
            return end();
        }
        iterator find(const K& key) {
            size_t ind = hashIndex(key);
            size_t counter = 0;
            
            while(m_vector[ind]._present && counter < m_vector.size()) {
                if (m_vector[ind].key() == key) {
                    return iterator(&m_vector[ind], m_vector.end());
                }

                ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
                ++counter;
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
        
        class const_iterator {
        public:        
            const_iterator(const hash_map_node<K, V>* current, const hash_map_node<K, V>* end)
            : _current(current), _end(end) {}

            const const_iterator& operator++() {
                while (_current != _end) {
                    _current++;
                    if (_current == _end || _current->_present) {
                        return *this;
                    } 
                }
                return *this; 
            }

            const_iterator operator++(int) {
                iterator result = *this; 
                ++(*this); 
                return result;
            }

            bool operator==(const const_iterator& other) {
                return _current == other._current;
            }

            bool operator!=(const const_iterator& other) {
                return !(*this == other);
            }

            const hash_map_node<K, V>& operator*() { return *_current; }
            const hash_map_node<K, V>* operator->() { return _current; }
        private:
            const hash_map_node<K, V>* _current;
            const hash_map_node<K, V>* _end;    
        };

        const_iterator begin() const {
            for (auto const & node : m_vector) {
                if (node._present) {
                    return const_iterator(&node, m_vector.end());
                }
            }
            return end();
        }
        const_iterator end() const {
            return const_iterator(m_vector.end(), m_vector.end());
        }

        iterator begin() {
            for (auto & node : m_vector) {
                if (node._present) {
                    return iterator(&node, m_vector.end());
                }
            }
            return end();
        }
        iterator end() {
            return iterator(m_vector.end(), m_vector.end());
        }

    private:
        // vector to store the elements.
        vector<hash_map_node<K, V> > m_vector;
        // number of key value pairs in table
        size_t m_size;

        std::hash<K> m_hash_fn;
        // Todo: make distribution more uniform
        inline size_t hashIndex(const K& key) const { return m_hash_fn(key) % m_vector.size(); }
    
        void increaseCapacity(size_t capacity) {
            prt::vector<hash_map_node<K, V> > temp;

            temp.resize(m_size);

            size_t count = 0;
            for (auto const & node : *this) {
                temp[count++] = node;
            }
            m_vector.clear();
            m_vector.resize(capacity);

            for (auto const & node : temp) {
                size_t ind = hashIndex(node.key());

                while (m_vector[ind]._present) {
                    ind = ind == m_vector.size() - 1 ? 0 : ind + 1;
                }
                m_vector[ind] = node;
            }
        }
    };
};

#endif