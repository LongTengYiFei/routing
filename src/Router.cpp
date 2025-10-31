#include "Router.h"
#include "config.h"
#include "ramcdc.h"

#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <filesystem>

using namespace std;

Sha1Hash Router::getFeature(const SuperChunk& super_chunk, enum ROUTING_METHOD method){
    if(method == STATELESS_MIN_HASH_WHOLE_CHUNK){
        Sha1Hash min_hash = super_chunk.chunk_list.front().hash;
        for(auto & chunk: super_chunk.chunk_list){
            if(chunk.hash < min_hash){
                min_hash = chunk.hash;
            }
        }
        return min_hash;
    }else if(method == STATELESS_FIRST_HASH_WHOLE_CHUNK){
        return super_chunk.chunk_list.front().hash;
    }else{
        printf("Unknown routing method\n");
        exit(-1);
    }
}

int Router::decideStorageNode(const Sha1Hash& super_chunk_feature) {
    // 取前 8 字节 → 转为 uint64_t
    uint64_t hash_value = 0;
    for (int i = 0; i < 8; ++i) {
        // super_chunk_feature 的低位放到了 hash_value 的高位，但是路由只关心数据是否均匀；
        hash_value = (hash_value << 8) | super_chunk_feature[i];
    }

    return static_cast<int>(hash_value % this->node_nums);
}

bool Router::isSuperChunkBoundary(Sha1Hash chunk_hash, int temp_super_chunk_width){
    int dest_super_chunk_width = Config::getInstance().getSuperChunkWidth();

    if(temp_super_chunk_width >= 2 * dest_super_chunk_width){
        // 最大2倍默认宽度
        // 强制分super chunk
        return true;
    }

    if(dest_super_chunk_width == 256){
        // 每个字节最高位同时为 1
        // 概率是 1/256
        return (chunk_hash[0] & 0x80) != 0 && (chunk_hash[1] & 0x80) != 0 &&
               (chunk_hash[2] & 0x80) != 0 && (chunk_hash[3] & 0x80) != 0 &&
               (chunk_hash[4] & 0x80) != 0 && (chunk_hash[5] & 0x80) != 0 &&
               (chunk_hash[6] & 0x80) != 0 && (chunk_hash[7] & 0x80) != 0;
    
    }else{
        printf("Unknown super chunk width\n");
        exit(-1);
    }
}

void Router::writeFile(string file_path){
    // read file
    int fd = open(file_path.c_str(), O_RDONLY);
    if(fd == -1){
        perror("open file failed");
        exit(-1);
    }
    long n_read = read(fd, this->single_file_cache, FILE_CACHE_SIZE);
    close(fd);

    long file_offset = 0;

    SuperChunk tmp_super_chunk;
    Sha1Hash tmp_chunk_hash;
    int tmp_super_chunk_width = 0;
    while(file_offset < n_read){
        // cdc
        long chunk_length = ramcdc_avx_512(reinterpret_cast<const char*>(this->single_file_cache + file_offset), n_read - file_offset);
        
        // hash
        SHA1(this->single_file_cache + file_offset, chunk_length, tmp_chunk_hash.data());

        // update super chunk
        tmp_super_chunk.total_size += chunk_length;
        tmp_super_chunk.chunk_list.push_back(Chunk{
                                    this->single_file_cache + file_offset,
                                    static_cast<int>(chunk_length),
                                    tmp_chunk_hash});
        tmp_super_chunk.feature = tmp_chunk_hash;
        tmp_super_chunk_width ++;

        if(isSuperChunkBoundary(tmp_super_chunk.feature, tmp_super_chunk_width)){
            // boundary and routing
            tmp_super_chunk.feature = tmp_super_chunk.chunk_list.front().hash; // first hash
            int node_id = this->decideStorageNode(tmp_super_chunk.feature);
            this->local_dedup_list[node_id].insertSuperChunkTask(tmp_super_chunk);

            // clear tmp_super_chunk
            tmp_super_chunk.chunk_list.clear();
            tmp_super_chunk.total_size = 0;
            tmp_super_chunk.feature.fill(0);
        }

        file_offset += chunk_length;
    }  
}

void Router::doBackup(){
    vector<string> files;
    string input_folder = Config::getInstance().getInputFolder();

    // traverse folder
    for (const auto &entry : std::filesystem::directory_iterator(input_folder)) {
        files.push_back(entry.path().string());
    }
    sort(files.begin(), files.end());

    // cdc init
    ramcdc_init(Config::getInstance().getAverageChunkSize());

    // write file
    for(int i = 0; i < files.size(); i++){
        this->writeFile(files[i]);
    }
    
    // update metric
    this->total_size = 0;
    this->total_dedup_size = 0;
    this->total_unique_size = 0;

    float average_disk_usage = 0;
    float max_disk_usage = 0;

    for(auto & local_dedup: this->local_dedup_list){
        if(local_dedup.getDiskUsage() > max_disk_usage){
            max_disk_usage = local_dedup.getDiskUsage();
        }
        average_disk_usage += local_dedup.getDiskUsage();

        this->total_size += local_dedup.getTotalDataSize();
        this->total_dedup_size += local_dedup.getDedupDataSize();
        this->total_unique_size += local_dedup.getUniqueDataSize();
    }
    average_disk_usage /= this->node_nums;

    this->metric_Total_Deduplication = this->total_size / this->total_unique_size;
    this->metric_Data_Skew = this->total_size / this->total_dedup_size;
    this->metric_Effiective_Deduplication = this->metric_Total_Deduplication / this->metric_Data_Skew;

    // print metric
    printf("metric_Total_Deduplication: %.2f\n", this->metric_Total_Deduplication);
    printf("metric_Data_Skew: %.2f\n", this->metric_Data_Skew);
    printf("metric_Effiective_Deduplication: %.2f\n", this->metric_Effiective_Deduplication);
}
