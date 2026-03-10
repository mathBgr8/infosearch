#ifndef MY_HASHMAP_H
#define MY_HASHMAP_H

#include <vector>
#include <string>
#include <functional>
#include <cstddef>

// Простая реализация hash map для совместимости со std::unordered_map
template<typename Key, typename Value>
class MyHashMap {
private:
    // Элемент хэш-таблицы
    struct Entry {
        Key first;
        Value second;
        bool occupied;
        
        Entry() : first(), second(), occupied(false) {}
        Entry(const Key& k, const Value& v) : first(k), second(v), occupied(true) {}
    };
    
    std::vector<Entry> buckets;
    size_t bucket_count;
    size_t m_size;
    std::hash<Key> hasher;
    
    // Простая хэш-функция для std::wstring (если Key - это wstring)
    size_t hashWString(const std::wstring& str) const {
        // Используем стандартную хэш-функцию для wstring
        return std::hash<std::wstring>{}(str);
    }
    
    size_t getBucketIndex(const Key& key) const {
        if constexpr (std::is_same_v<Key, std::wstring>) {
            return hashWString(key) % bucket_count;
        } else {
            return hasher(key) % bucket_count;
        }
    }
    
    std::pair<size_t, size_t> findEntry(const Key& key) const {
        size_t bucket_idx = getBucketIndex(key);
        size_t start = bucket_idx;
        size_t entry_idx = static_cast<size_t>(-1);
        
        do {
            if (buckets[bucket_idx].occupied) {
                if (buckets[bucket_idx].first == key) {
                    entry_idx = bucket_idx;
                    break;
                }
            }
            bucket_idx = (bucket_idx + 1) % bucket_count;
        } while (bucket_idx != start);
        
        return {bucket_idx, entry_idx};
    }
    
    size_t findEmptySlot(const Key& key) {
        size_t bucket_idx = getBucketIndex(key);
        size_t start = bucket_idx;
        
        do {
            if (!buckets[bucket_idx].occupied) {
                return bucket_idx;
            }
            if (buckets[bucket_idx].first == key) {
                return bucket_idx; // Обновляем существующий
            }
            bucket_idx = (bucket_idx + 1) % bucket_count;
        } while (bucket_idx != start);
        
        // Таблица заполнена - нужно расширение
        rehash();
        // После рехэша ищем заново
        return findEmptySlot(key);
    }
    
    void rehash() {
        size_t old_count = bucket_count;
        std::vector<Entry> old_buckets = std::move(buckets);
        m_size = 0;
        
        bucket_count = old_count == 0 ? 8 : old_count * 2;
        buckets = std::vector<Entry>(bucket_count);
        
        // Переinsert все элементы
        for (size_t i = 0; i < old_count; ++i) {
            if (old_buckets[i].occupied) {
                size_t new_idx = findEmptySlot(old_buckets[i].first);
                buckets[new_idx] = std::move(old_buckets[i]);
                ++m_size;
            }
        }
    }
    
public:
    // Итератор
    class iterator {
    private:
        MyHashMap* map;
        size_t index;
        
    public:
        iterator(MyHashMap* m, size_t idx) : map(m), index(idx) {}
        
        iterator& operator++() {
            if (map) {
                do {
                    ++index;
                } while (index < map->bucket_count && !(map->buckets[index].occupied));
            }
            return *this;
        }
        
        Entry& operator*() const {
            return map->buckets[index];
        }
        
        Entry* operator->() const {
            return &map->buckets[index];
        }
        
        bool operator==(const iterator& other) const {
            return map == other.map && index == other.index;
        }
        
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };
    
    MyHashMap() : bucket_count(8), m_size(0) {
        buckets.resize(bucket_count);
    }
    
    MyHashMap(size_t initial_buckets) : bucket_count(initial_buckets), m_size(0) {
        if (bucket_count == 0) bucket_count = 8;
        buckets.resize(bucket_count);
    }
    
    ~MyHashMap() = default;
    
    // Вставка или обновление
    Value& operator[](const Key& key) {
        size_t idx = findEmptySlot(key);
        
        if (!buckets[idx].occupied) {
            buckets[idx] = Entry(key, Value{});
            ++m_size;
            // Проверяем коэффициент загрузки
            if (m_size * 2 >= bucket_count) {
                rehash();
            }
        }
        
        return buckets[idx].second;
    }
    
    // Вставка (возвращает true если вставлен новый)
    std::pair<iterator, bool> insert(const std::pair<Key, Value>& pair) {
        size_t idx = findEmptySlot(pair.first);
        
        if (buckets[idx].occupied) {
            // Ключ уже существует, обновляем
            buckets[idx].second = pair.second;
            return {iterator(this, idx), false};
        } else {
            buckets[idx] = Entry(pair.first, pair.second);
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
        return iterator(const_cast<MyHashMap*>(this), bucket_idx);
    }
    
    bool contains(const Key& key) const {
        auto [_, entry_idx] = const_cast<MyHashMap*>(this)->findEntry(key);
        return entry_idx != static_cast<size_t>(-1);
    }
    
    bool erase(const Key& key) {
        auto [bucket_idx, entry_idx] = findEntry(key);
        if (entry_idx == static_cast<size_t>(-1)) {
            return false;
        }
        
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
    
    iterator begin() const {
        for (size_t i = 0; i < bucket_count; ++i) {
            if (buckets[i].occupied) {
                return iterator(const_cast<MyHashMap*>(this), i);
            }
        }
        return end();
    }
    
    iterator end() {
        return iterator(this, bucket_count);
    }
    
    iterator end() const {
        return iterator(const_cast<MyHashMap*>(this), bucket_count);
    }
    
    
    // Доступ по ключу
    const Value& at(const Key& key) const {
        auto it = const_cast<MyHashMap*>(this)->find(key);
        if (it == end()) {
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }
};

#endif // MY_HASHMAP_H
