#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include <atomic>

template<typename T>
struct Snapshot;

template<typename T>
struct WaitFreeAtomicSnapshotItem{
    T data;
    uint64_t version;
    std::shared_ptr<Snapshot<T>> last;

    WaitFreeAtomicSnapshotItem() = default;
    WaitFreeAtomicSnapshotItem(T data, uint64_t version, std::shared_ptr<Snapshot<T>> last): data(data), version(version), last(last) {}
    WaitFreeAtomicSnapshotItem(const WaitFreeAtomicSnapshotItem& other) = default;
    WaitFreeAtomicSnapshotItem(WaitFreeAtomicSnapshotItem&& other) = default;
    WaitFreeAtomicSnapshotItem& operator=(const WaitFreeAtomicSnapshotItem& other) = default;
    WaitFreeAtomicSnapshotItem& operator=(WaitFreeAtomicSnapshotItem&& other) = default;
    ~WaitFreeAtomicSnapshotItem() = default;
};

template<typename T>
struct Snapshot {
    std::vector<std::shared_ptr<WaitFreeAtomicSnapshotItem<T>>> items;
};

template<typename T>
class WaitFreeAtomicSnapshot {
private:
    std::vector<std::shared_ptr<WaitFreeAtomicSnapshotItem<T>>> items;
    // std::vector<std::atomic<WaitFreeAtomicSnapshotItem<T>*>> items;
    std::vector<size_t> num_updates;

    std::shared_ptr<Snapshot<T>> collect() const{
        std::shared_ptr<Snapshot<T>> snap = std::make_shared<Snapshot<T>>();
        snap->items.reserve(this->items.size());
        
        for(const auto& item : this->items){
            snap->items.push_back(item);
            // snap->items.push_back(std::make_shared<WaitFreeAtomicSnapshotItem<T>>(*item));
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
            // items[i] = new WaitFreeAtomicSnapshotItem<T>();
        }
    }

    ~WaitFreeAtomicSnapshot() noexcept {
        // Defered destruction
        for(size_t i = 0;i < items.size();i++){
            while(items[i]->last != nullptr){
                std::shared_ptr<Snapshot<T>> to_be_killed;
                std::swap(to_be_killed, items[i]->last);
                items[i]->last = std::move(to_be_killed->items[i]->last);
            }
            // auto lastSnap = items[i].load()->last;
            // while(lastSnap != nullptr){
            //     std::shared_ptr<Snapshot<T>> to_be_killed;
            //     std::swap(to_be_killed, lastSnap);
            //     lastSnap = std::move(to_be_killed->items[i]->last);
            // }
        }
    }

    void update(size_t tid, T new_data){
        // Take a scan
        auto collect = scan();
        
        // Build a new Wait-Free Atomic Snapshot Item
        auto item = std::make_shared<WaitFreeAtomicSnapshotItem<T>>(
            new_data, collect->items[tid]->version + 1, collect
        );
        // auto item = new WaitFreeAtomicSnapshotItem<T>(
        //     new_data, collect->items[tid]->version + 1, collect
        // );
        
        // Atomic Update
        this->items[tid] = std::move(item);
        // this->items[tid].exchange(item);
        // delete item;

        // this->items[tid]->last = nullptr;
    
        // Increment update count
        ++num_updates[tid];
    }

    std::shared_ptr<Snapshot<T>> scan() const {
        std::vector<bool> moved(items.size());
        size_t i;

        using CollectType = decltype(collect());
        CollectType old_collect = collect();
        CollectType new_collect;

        while(true){
            new_collect = collect();
            for(i = 0;i < items.size();i++){
                if(old_collect->items[i]->version != new_collect->items[i]->version){
                    if(moved[i]) {
                        return new_collect->items[i]->last;
                    }else{
                        moved[i] = true;
                        break;
                    }
                }
            }
            // old_collect == new_collect
            if(i == items.size())
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
inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Snapshot<T>>& snapshot) {
    size_t n = snapshot->items.size();
    
    os << "Snapshot(" << snapshot->items[0]->data;
    for(size_t i = 1;i < n;++i){
        os << ", " << snapshot->items[i]->data;
    }
    os << ")";

    return os;
}