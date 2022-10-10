#pragma once
#include <atomic>
#include <vector>
#include <memory>

template<typename T>
struct LockFreeAtomicSnapshotItem{
    T data;
    uint64_t version;

    LockFreeAtomicSnapshotItem() = default;
    ~LockFreeAtomicSnapshotItem() = default;
    LockFreeAtomicSnapshotItem(T data, uint64_t version) : data(data), version(version) {}
    LockFreeAtomicSnapshotItem(const LockFreeAtomicSnapshotItem&) = default;
    LockFreeAtomicSnapshotItem& operator=(const LockFreeAtomicSnapshotItem&) = default;
    LockFreeAtomicSnapshotItem(LockFreeAtomicSnapshotItem&&) = default;
    LockFreeAtomicSnapshotItem& operator=(LockFreeAtomicSnapshotItem&&) = default;
};

template<typename T>
class LockFreeAtomicSnapshot {
private:
    std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>> items;
    std::vector<size_t> num_updates;

    std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>> collect() const{
        std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>> result;
        result.reserve(this->items.size());
        for(auto& item : this->items){
            result.push_back(item);
        }
        return result;
    }

    bool isSameCollect(const std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>>& lhs, const std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>>& rhs) const{
        for(size_t i = 0; i < items.size(); i++){
            if(lhs[i]->version != rhs[i]->version){
                return false;
            }
        }
        return true;
    }
public:
    LockFreeAtomicSnapshot(size_t n)
    {
        num_updates.resize(n);
        items.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            items.push_back(std::make_shared<LockFreeAtomicSnapshotItem<T>>());
        }
    }

    ~LockFreeAtomicSnapshot() = default;

    void update(size_t tid, uint32_t data){
        auto new_item = std::make_shared<LockFreeAtomicSnapshotItem<T>>();
        new_item->data = data;
        new_item->version = this->items[tid]->version + 1;
        this->items[tid] = new_item;
        ++num_updates[tid];
    }

    std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>> scan() const {
        std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>> old_collect = collect();
        std::vector<std::shared_ptr<LockFreeAtomicSnapshotItem<T>>> new_collect;
        while(true){
            new_collect = collect();
            if(isSameCollect(old_collect, new_collect)){
                break;
            }
            old_collect = std::move(new_collect);
        }
        return old_collect;
    }

    size_t getNumberOfUpdates(size_t tid) const {
        return this->num_updates[tid];
    }
};