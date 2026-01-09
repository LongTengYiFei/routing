#!/bin/bash

para_path="/home/cyf/routing/para.json"
output_path="/home/cyf/routing/experiment/result.txt"

input_file=(
    # "/home/cyf/ssd0/Android"
    # "/home/cyf/ssd0/CHM"
    # "/home/cyf/ssd0/sfd_datasets/tar"
    "/home/cyf/ssd0/sfd_datasets/pdf"
    "/home/cyf/ssd0/sfd_datasets/apk_freefire"
    "/home/cyf/ssd0/sfd_datasets/apk_pubg"
)

routing_methods=(
    # "first_hash_all"
    "first_hash_64"
)

node_num=(1 2 4 8 16 32 64)

# 确保输出目录存在
mkdir -p "$(dirname "$output_path")"

# 清空或创建输出文件
> "$output_path"

# 遍历所有 input_file、routing_method 和 node_num 组合
for input in "${input_file[@]}"; do
    for routing_method in "${routing_methods[@]}"; do
        for nodes in "${node_num[@]}"; do
            {
                echo "正在测试: input_file=$input, routing_method=$routing_method, node_num=$nodes"
                
                # 使用 jq 修改 JSON 文件：设置 input_file, node_num, routing_method
                jq --arg input "$input" \
                   --arg routing_method "$routing_method" \
                   --argjson nodes "$nodes" \
                   '.input_file = $input | .node_num = $nodes | .routing_method = $routing_method' \
                   "$para_path" > tmp.json && mv tmp.json "$para_path"
                
                # 运行程序并将输出同时显示在控制台和保存到文件
                echo "=== 程序输出 ==="
                ../cRouting "$para_path"
                echo "=== 程序结束 ==="
                
                echo "完成: input_file=$input, routing_method=$routing_method, node_num=$nodes"
                echo "----------------------------------------"
            } | tee -a "$output_path"
        done
    done
done

echo "所有测试完成，结果已保存到: $output_path"