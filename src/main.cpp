#include <string>

#include "config.h"
#include "Router.h"

using namespace std;

int main(int argc, char** argv){
    string para_json_file_path = argv[1];
    Config::getInstance().parse_argument(argc, argv);
    int node_nums = Config::getInstance().getNodeNums();

    Router router(node_nums);
    router.doBackup();
    
    return 0;
}