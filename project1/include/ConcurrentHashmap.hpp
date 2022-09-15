#pragma once

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

#define USE_CUSTON_ALLOCATOR 1

template <typename T>
class concurrent_allocator {
    size_t max_cnt;
    std::atomic<size_t> alloc_cnt;
    std::vector<T> memory_pool;
public:
    concurrent_allocator(size_t sz)
        :max_cnt(sz), memory_pool{}, alloc_cnt() {
            memory_pool.reserve(sz);
        }
    ~concurrent_allocator() noexcept = default;

    void reserve(size_t sz) {
        memory_pool.reserve(sz);
    }

    template<typename ...Args>
    inline T* allocate(Args&&... args) {
        return &(memory_pool[alloc_cnt.fetch_add(1)] = T(std::forward<Args>(args)...));
    }
};

template<typename T>
struct node
{
    T data;
    node* next;
    node() = default;
    node(const T& data) : data(data), next(nullptr) {}
};

template<typename T>
class concurrent_linked_list
{
    #if USE_CUSTON_ALLOCATOR
    concurrent_allocator<node<T>> allocator;
    #endif
    std::atomic<size_t> _size;
    std::atomic<node<T>*> head;

public:
    #if USE_CUSTON_ALLOCATOR
    concurrent_linked_list()
        : _size(), head(), allocator(0) {}
    concurrent_linked_list(size_t max_size)
        : _size(), head(), allocator(max_size) {}
    #else
    concurrent_linked_list()
        : _size(), head() {}
    #endif

    ~concurrent_linked_list() {
        #if USE_CUSTON_ALLOCATOR
        #else
        node<T>* it = head.load();
        while(it) {
            node<T>* next = it->next;
            delete it;
            it = next;
        }
        #endif
    }

    concurrent_linked_list(const concurrent_linked_list&) = delete;
    concurrent_linked_list& operator=(const concurrent_linked_list&) = delete;
    concurrent_linked_list(concurrent_linked_list&&) = default;
    concurrent_linked_list& operator=(concurrent_linked_list&&) = default;
    
    void push(const T& data)
    {
        #if USE_CUSTON_ALLOCATOR
        node<T> *new_node = allocator.allocate(data);
        #else
        node<T> *new_node = new node<T>(data);
        #endif

        // put the current value of head into new_node->next
        new_node->next = head.load(std::memory_order_relaxed);
        node<T> *old_head = head.load(std::memory_order_relaxed);
        do {
            new_node->next = old_head;
        } while (!head.compare_exchange_weak(old_head, new_node,
                std::memory_order_release,std::memory_order_relaxed));
        
        // Increase the size
        _size.fetch_add(1);
    }

    const node<T>* get_head() const{
        return head.load();
    }

    size_t size() const {
        return _size.load();
    }

    #if USE_CUSTON_ALLOCATOR
    concurrent_allocator<node<T>>& get_allocator(){
        return this->allocator;
    }
    #endif
};

template<typename Key, typename Value>
class concurrent_hashmap
{
    size_t num_buckets;
    std::vector<concurrent_linked_list<std::pair<Key, Value>>> buckets;
public:
    #if USE_CUSTON_ALLOCATOR
    concurrent_hashmap(size_t num_buckets, size_t max_size)
    :num_buckets(num_buckets), buckets(num_buckets) {
        for(auto& bucket : buckets){
            bucket.get_allocator().reserve(max_size);
        }
    }
    #else
    concurrent_hashmap(size_t num_buckets)
    :num_buckets(num_buckets), buckets(num_buckets) {}
    #endif
    ~concurrent_hashmap() = default;
    
    static size_t hash(const Key& data){
        return std::hash<Key>()(data);
    }

    size_t get_bucket_size() const{
        return num_buckets;
    }
    
    void insert(const Key& key, const Value& value){
        buckets[hash(key) % num_buckets].push({key, value});
    }

    void insert(const Key& key, Value&& value){
        buckets[hash(key) % num_buckets].push({key, std::move(value)});
    }

    const concurrent_linked_list<std::pair<Key, Value>>& get_bucket(size_t idx) const{
        return buckets[idx];
    }

    const concurrent_linked_list<std::pair<Key, Value>>& get_bucket_by_data(const Key& key) const{
        return buckets[hash(key) % num_buckets];
    }

    size_t size() const {
        size_t result = 0;
        for(size_t i = 0; i < num_buckets; ++i){
            result += buckets[i].size();
        }
        return result;
    }
};