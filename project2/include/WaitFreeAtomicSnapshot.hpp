#pragma once
#include <atomic>
#include <iostream>
#include <memory>
#include <vector>

template<typename T>
struct Snapshot;

template<typename T>
struct WaitFreeAtomicSnapshotItem{
    T data;
    uint64_t version;
    std::unique_ptr<Snapshot<T>> last;
};

template<typename T>
struct Snapshot {
    std::vector<std::shared_ptr<WaitFreeAtomicSnapshotItem<T>>> items;
};

template<typename T>
class WaitFreeAtomicSnapshot {
private:
    std::vector<std::shared_ptr<WaitFreeAtomicSnapshotItem<T>>> items;
    std::vector<size_t> num_updates;

    std::unique_ptr<Snapshot<T>> collect() const{
        std::unique_ptr<Snapshot<T>> snap = std::make_unique<Snapshot<T>>();
        snap->items.resize(this->items.size());
        
        for(size_t i = 0; i < this->items.size(); i++){
            snap->items[i] = std::atomic_load(&this->items[i]);
        }
        return snap;
    }

public:
    WaitFreeAtomicSnapshot(size_t n)
        :items(n), num_updates(n)
    {
        for (size_t i = 0; i < n; i++)
        {
            items[i] = std::make_shared<WaitFreeAtomicSnapshotItem<T>>();
        }
    }

    ~WaitFreeAtomicSnapshot() noexcept {
        // Defered destruction
        for(size_t i = 0;i < items.size();i++){
            while(items[i]->last != nullptr){
                std::unique_ptr<Snapshot<T>> to_be_killed;
                std::swap(to_be_killed, items[i]->last);
                items[i]->last = std::move(to_be_killed->items[i]->last);
            }
        }
    }

    void update(size_t tid, T new_data){
        // Take a scan
        auto collect = scan();

        // Build a new Wait-Free Atomic Snapshot Item
        auto item = std::make_shared<WaitFreeAtomicSnapshotItem<T>>();
        item->data = new_data;
        item->version = collect->items[tid]->version + 1;
        item->last = std::move(collect);
        
        // Atomic Update the WaitFreeAtomicSnapshotItem
        std::atomic_store(&this->items[tid], std::move(item));
    
        // Increment update count
        ++num_updates[tid];
    }

    std::unique_ptr<Snapshot<T>> scan() const {
        using CollectType = decltype(collect());
        CollectType new_collect;
        CollectType old_collect;
        
        const size_t item_size = items.size();
        std::vector<bool> moved(item_size);
        size_t i;
        
        old_collect = collect();
        while(true){
            new_collect = collect();
            for(i = 0;i < item_size;i++){
                if(old_collect->items[i]->version != new_collect->items[i]->version){
                    if(moved[i]) {
                        return std::make_unique<Snapshot<T>>(*new_collect->items[i]->last);
                    }else{
                        moved[i] = true;
                        break;
                    }
                }
            }

            if(i == item_size) // old_collect == new_collect (able to get an atomic snapshot)
                break;

            old_collect = std::move(new_collect);
        }
        return new_collect;
    }

    size_t getNumberOfUpdates(size_t tid) const {
        return this->num_updates[tid];
    }
};

template<typename T>
inline std::ostream& operator<<(std::ostream& os, const std::unique_ptr<Snapshot<T>>& snapshot) {
    size_t n = snapshot->items.size();
    
    os << "Snapshot(" << snapshot->items[0]->data;
    for(size_t i = 1;i < n;++i){
        os << ", " << snapshot->items[i]->data;
    }
    os << ")";

    return os;
}