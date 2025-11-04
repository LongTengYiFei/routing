#pragma once

#include <ramcdc.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <queue>
#include <string.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "SuperChunk.h"

using std::vector;
using std::string;

#define GB (1024ULL * 1024ULL * 1024ULL)

class LocalDeduplicator {
public:
    LocalDeduplicator(){
        this->disk_capacity = 64 * GB;
        this->disk_usage_ratio = 0;
        this->local_total_data_size = 0;
        this->local_dedup_data_size = 0;
        this->local_unique_data_size = 0;
        this->is_running = true;
        
        // 启动工作线程
        worker_thread = std::thread(&LocalDeduplicator::run, this);
    }

    /*
        移动构造函数
        用不上，但是为了编译器不报错
        vector扩容时会寻找移动构造函数
        如果没有移动构造函数，会报错
    */
    LocalDeduplicator(LocalDeduplicator&& other) noexcept 
        : local_total_data_size(other.local_total_data_size),
          local_dedup_data_size(other.local_dedup_data_size),
          local_unique_data_size(other.local_unique_data_size),
          disk_capacity(other.disk_capacity),
          disk_usage_ratio(other.disk_usage_ratio),
          fingerprint_index(std::move(other.fingerprint_index)),
          is_running(other.is_running.load()),
          super_chunk_queue(std::move(other.super_chunk_queue))
    {
        other.local_total_data_size = 0;
    }
    
    ~LocalDeduplicator() {
        stop();
    }

    // 删除拷贝构造函数和拷贝赋值运算符
    // 因为该类包含mutex，而mutex不能拷贝；
    LocalDeduplicator(const LocalDeduplicator&) = delete;
    LocalDeduplicator& operator=(const LocalDeduplicator&) = delete;

    void insertSuperChunkTask(const SuperChunk& super_chunk);
    void stop();
    
    // getters
    double getDiskUsage(){ 
        std::lock_guard<std::mutex> lock(stats_mutex);
        return disk_usage_ratio;
    }
    long long getTotalDataSize(){ 
        std::lock_guard<std::mutex> lock(stats_mutex);
        return local_total_data_size;
    }
    long long getDedupDataSize(){ 
        std::lock_guard<std::mutex> lock(stats_mutex);
        return local_dedup_data_size;
    }
    long long getUniqueDataSize(){ 
        std::lock_guard<std::mutex> lock(stats_mutex);
        return local_unique_data_size;
    }

private:
    void run(); 
    void dedupSuperChunk(const SuperChunk& super_chunk);

    // 统计数据
    long long local_total_data_size;
    long long local_dedup_data_size;
    long long local_unique_data_size;
    std::size_t disk_capacity;
    double disk_usage_ratio;
    
    // 指纹索引
    std::unordered_set<Sha1Hash> fingerprint_index;
    
    // 任务队列和同步
    std::queue<SuperChunk> super_chunk_queue;
    std::condition_variable queue_cv;

    // 队列锁
    std::mutex queue_mutex; 
    
    // 统计数据锁
    std::mutex stats_mutex;
    
    // 线程控制
    std::atomic<bool> is_running;
    std::thread worker_thread;
};