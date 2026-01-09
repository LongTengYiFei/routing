#pragma once

#include <string>
#include "SuperChunk.h"
#include "LocalDeduplicator.h"
#include "config.h"

using namespace std;

#define FILE_CACHE_SIZE (4ULL * 1024 * 1024 * 1024)

class Router {
public:
    Router(int _node_nums){
        metric_Total_Deduplication = 0;
        metric_Data_Skew = 0;
        metric_Effiective_Deduplication = 0;

        if (posix_memalign((void**)&single_file_cache, 4096, FILE_CACHE_SIZE) != 0) {
            perror("posix_memalign failed");
            exit(-1);
        }

        // init local deduplicator
        node_nums = _node_nums;
        this->local_dedup_list.reserve(node_nums);  // 预分配足够空间
        for(int i = 0; i < node_nums; i++){
            // 等价于：在vector内存中调用 LocalDeduplicator()
            // LocalDeduplicator含有mutex，不能拷贝；
            this->local_dedup_list.emplace_back();
        }
    }

    Sha1Hash getFeature(const SuperChunk& super_chunk, enum ROUTING_METHOD method);
    int decideStorageNode(const Sha1Hash& super_chunk_feature);
    void doBackup();
    void writeFile(string file_path);
    bool isSuperChunkBoundary(const Sha1Hash& chunk_hash, int super_chunk_width);

private:
    vector<LocalDeduplicator> local_dedup_list;

    double metric_Total_Deduplication;
    double metric_Data_Skew;
    double metric_Effiective_Deduplication;

    long long total_size;
    long long total_dedup_size;
    long long total_unique_size;

    double average_disk_usage;
    double max_disk_usage;

    int node_nums;

    uint8_t* single_file_cache;
};