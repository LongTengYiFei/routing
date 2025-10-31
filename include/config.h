#ifndef __CONFIG_H__
#define __CONFIG_H__
#include<iostream>
#include <string.h>
#include"cJSON.h"
using namespace std;

enum ROUTING_METHOD{
    STATEFUL,
    STATELESS_FIRST_HASH_64,
    STATELESS_FIRST_HASH_WHOLE_CHUNK,
    STATELESS_MIN_HASH_64,
    STATELESS_MIN_HASH_WHOLE_CHUNK,
};

class Config{
    public:
        static Config& getInstance() {
          static Config instance;
          return instance;
        }

        // getters
        string getInputFolder(){return this->input_file;}
        int getNodeNums(){return this->node_num;}
        int getAverageChunkSize(){return this->average_chunk_size;}
        int getSuperChunkWidth(){return this->super_chunk_width;}

        // setters
        void setInputFile(char* s){this->input_file = s;}
        void setNodeNums(int n){this->node_num = n;}
        void setAverageChunkSize(int n){this->average_chunk_size = n;}
        void setSuperChunkWidth(int n){this->super_chunk_width = n;}

        // you know
        void parse_argument(int argc, char **argv)
        {
            char source[2000 + 1];
            FILE *fp = fopen(argv[1], "r");
            if (fp != NULL) {
                size_t newLen = fread(source, sizeof(char), 1001, fp);
                if ( ferror( fp ) != 0 ) {
                    fputs("Error reading file", stderr);
                } else {
                    source[newLen++] = '\0'; /* Just to be safe. */
                }

                fclose(fp);
            }else{
                perror("open json file failed");
            }

            cJSON *config = cJSON_Parse(source);
            for (cJSON *param = config->child; param != nullptr; param = param->next) {
                char *name = param->string;
                char *valuestring = param->valuestring;
                int val_int = param->valueint;

                if (strcmp(name, "input_file") == 0) {
                    Config::getInstance().setInputFile(valuestring);
                }else if (strcmp(name, "node_num") == 0) {
                    Config::getInstance().setNodeNums(val_int);
                }else if (strcmp(name, "average_chunk_size") == 0) {
                    Config::getInstance().setAverageChunkSize(val_int);
                }else if (strcmp(name, "super_chunk_width") == 0) {
                    Config::getInstance().setSuperChunkWidth(val_int);
                }
            }
        }

private:
    string input_file;
    int node_num;
    int average_chunk_size;
    int super_chunk_width;

    int normal_level;
    enum ROUTING_METHOD routing_method;

    Config() {
        // 默认参数配置
        normal_level = 2;
        routing_method = STATELESS_FIRST_HASH_64;
    }
};
#endif