#ifndef MY_HASHSET_H
#define MY_HASHSET_H

#include <vector>
#include <string>
#include <functional>
#include <cstddef>

// Простая реализация hash set для совместимости со std::unordered_set
template<typename Key>
class MyHashSet {
private:
    // Элемент хэш-таблицы
    struct Entry {
        Key key;
        bool occupied;
        
        Entry() : occupied(false) {}
        Entry(const Key& k) : key(k), occupied(true) {}
    };
    
    std::vector<Entry> buckets;
    size_t bucket_count;
    size_t m_size;
    std::hash<Key> hasher;
    
    // Простая хэш-функция для std::wstring (если Key - это wstring)
    size_t hashWString(const std::wstring& str) const {
        return std::hash<std::wstring>{}(str);
    }
    
    size_t getBucketIndex(const Key& key) const {
        if constexpr (std::is_same_v<Key, std::wstring>) {
            return hashWString(key) % bucket_count;
        } else {
            return hasher(key) % bucket_count;
        }
    }
    
    // Найти запись по ключу (возвращает индекс или -1 если не найден)
    std::pair<size_t, size_t> findEntry(const Key& key) const {
        size_t bucket_idx = getBucketIndex(key);
        size_t start = bucket_idx;
        size_t entry_idx = static_cast<size_t>(-1);
        
        do {
            if (buckets[bucket_idx].occupied) {
                if (buckets[bucket_idx].key == key) {
                    entry_idx = bucket_idx;
                    break;
                }
            }
            bucket_idx = (bucket_idx + 1) % bucket_count;
        } while (bucket_idx != start);
        
        return {bucket_idx, entry_idx};
    }
    
    // Найти свободную позицию для вставки (с линейным пробированием)
    size_t findEmptySlot(const Key& key) {
        size_t bucket_idx = getBucketIndex(key);
        size_t start = bucket_idx;
        
        do {
            if (!buckets[bucket_idx].occupied) {
                return bucket_idx;
            }
            if (buckets[bucket_idx].key == key) {
                return bucket_idx; // Обновляем существующий
            }
            bucket_idx = (bucket_idx + 1) % bucket_count;
        } while (bucket_idx != start);
        
        // Таблица заполнена - нужно расширение
        rehash();
        // После рехэша ищем заново
        return findEmptySlot(key);
    }
    
    // Реаллокация (увеличиваем размер в 2 раза)
    void rehash() {
        size_t old_count = bucket_count;
        std::vector<Entry> old_buckets = std::move(buckets);
        m_size = 0;
        
        bucket_count = old_count == 0 ? 8 : old_count * 2;
        buckets = std::vector<Entry>(bucket_count);
        
        // Перевставляем все элементы
        for (size_t i = 0; i < old_count; ++i) {
            if (old_buckets[i].occupied) {
                size_t new_idx = findEmptySlot(old_buckets[i].key);
                buckets[new_idx] = std::move(old_buckets[i]);
                ++m_size;
            }
        }
    }
    
public:
    // Итератор
    class iterator {
    private:
        MyHashSet* set;
        size_t index;
        
    public:
        iterator(MyHashSet* s, size_t idx) : set(s), index(idx) {}
        
        iterator& operator++() {
            if (set) {
                do {
                    ++index;
                } while (index < set->bucket_count && !(set->buckets[index].occupied));
            }
            return *this;
        }
        
        const Key& operator*() const {
            return set->buckets[index].key;
        }
        
        const Key* operator->() const {
            return &set->buckets[index].key;
        }
        
        bool operator==(const iterator& other) const {
            return set == other.set && index == other.index;
        }
        
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };
    
    MyHashSet() : bucket_count(8), m_size(0) {
        buckets.resize(bucket_count);
    }
    
    MyHashSet(size_t initial_buckets) : bucket_count(initial_buckets), m_size(0) {
        if (bucket_count == 0) bucket_count = 8;
        buckets.resize(bucket_count);
    }
    
    MyHashSet(std::initializer_list<Key> init) : MyHashSet() {
        for (const auto& key : init) {
            insert(key);
        }
    }
    
    ~MyHashSet() = default;
    
    std::pair<iterator, bool> insert(const Key& key) {
        size_t idx = findEmptySlot(key);
        
        if (buckets[idx].occupied) {
            // Ключ уже существует
            return {iterator(this, idx), false};
        } else {
            buckets[idx] = Entry(key);
            ++m_size;
            if (m_size * 2 >= bucket_count) {
                rehash();
            }
            return {iterator(this, idx), true};
        }
    }
    
    iterator find(const Key& key) {
        auto [bucket_idx, entry_idx] = findEntry(key);
        if (entry_idx == static_cast<size_t>(-1)) {
            return end();
        }
        return iterator(this, bucket_idx);
    }
    
    iterator find(const Key& key) const {
        auto [bucket_idx, entry_idx] = findEntry(key);
        if (entry_idx == static_cast<size_t>(-1)) {
            return end();
        }
        return iterator(const_cast<MyHashSet*>(this), bucket_idx);
    }
    
    bool contains(const Key& key) const {
        auto [_, entry_idx] = const_cast<MyHashSet*>(this)->findEntry(key);
        return entry_idx != static_cast<size_t>(-1);
    }
    
    bool erase(const Key& key) {
        auto [bucket_idx, entry_idx] = findEntry(key);
        if (entry_idx == static_cast<size_t>(-1)) {
            return false;
        }
        
        // Помечаем как удаленный (просто сбрасываем occupied)
        buckets[bucket_idx].occupied = false;
        --m_size;
        return true;
    }
    
    void clear() {
        for (size_t i = 0; i < bucket_count; ++i) {
            buckets[i].occupied = false;
        }
        m_size = 0;
    }
    
    size_t size() const {
        return m_size;
    }
    
    bool empty() const {
        return size() == 0;
    }
    
    iterator begin() {
        for (size_t i = 0; i < bucket_count; ++i) {
            if (buckets[i].occupied) {
                return iterator(this, i);
            }
        }
        return end();
    }
    
    iterator end() {
        return iterator(this, bucket_count);
    }
    
    iterator begin() const {
        for (size_t i = 0; i < bucket_count; ++i) {
            if (buckets[i].occupied) {
                return iterator(const_cast<MyHashSet*>(this), i);
            }
        }
        return end();
    }
    
    iterator end() const {
        return iterator(const_cast<MyHashSet*>(this), bucket_count);
    }
};

#endif // MY_HASHSET_H
