#include "LocalDeduplicator.h"

void LocalDeduplicator::dedupSuperChunk(const SuperChunk& super_chunk) {
    int unique_size = 0;
    int dedup_size = 0;
    int total_size = 0;
    
    // 执行去重计算
    for (auto& chunk : super_chunk.chunk_list) {
        if (fingerprint_index.find(chunk.hash) == fingerprint_index.end()) {
            fingerprint_index.insert(chunk.hash);
            unique_size += chunk.size;
        } else {
            dedup_size += chunk.size;
        }
        total_size += chunk.size;
    }
    
    // 直接更新统计数据（不需要加锁，因为只有工作线程会修改）
    local_unique_data_size += unique_size;
    local_dedup_data_size += dedup_size;
    local_total_data_size += total_size;
    disk_usage_ratio = (float)local_unique_data_size / (float)disk_capacity;
}

void LocalDeduplicator::insertSuperChunkTask(const SuperChunk& super_chunk) {
    // 这里实际上没必要加锁了，因为我就一个router，真实部署多个router才需要加锁；
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        super_chunk_queue.push(super_chunk);
        // 立即释放锁，然后通知；
        // 不然消费者会消耗多余等待时间
    }
    // 通知工作线程有新任务
    queue_cv.notify_one();
}

void LocalDeduplicator::run() {
    while (is_running) {
        SuperChunk task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // 等待条件：队列不为空或线程需要停止
            queue_cv.wait(lock, [this]() {
                return !super_chunk_queue.empty() || !is_running;
            });
            
            // 检查是否要退出
            if (!is_running && super_chunk_queue.empty()) {
                break;
            }
            
            // 取出任务
            if (!super_chunk_queue.empty()) {
                task = super_chunk_queue.front();
                super_chunk_queue.pop();
            }
        }
        
        // 处理任务（不在锁内执行，避免阻塞其他线程）
        if (task.chunk_list.size() > 0) {
            dedupSuperChunk(task);
        }
    }
}

void LocalDeduplicator::stop() {
    is_running = false;
    queue_cv.notify_one();
    
    // 等待线程结束
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}