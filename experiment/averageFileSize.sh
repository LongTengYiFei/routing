input_file=(
    "/home/cyf/ssd0/Android"
    "/home/cyf/ssd0/CHM"
    "/home/cyf/ssd0/sfd_datasets/tar"
    "/home/cyf/ssd0/sfd_datasets/pdf"
    "/home/cyf/ssd0/sfd_datasets/apk_freefire"
    "/home/cyf/ssd0/sfd_datasets/apk_pubg"
)

for dir in "${input_file[@]}"; do
    du -sh $dir
    if [[ -d "$dir" ]]; then
        # Find all files, get their sizes in bytes, and calculate the average
        total_size_bytes=$(find "$dir" -type f -exec stat -c%s {} + | awk '{sum += $1} END {print sum+0}')
        file_count=$(find "$dir" -type f | wc -l)
        
        if [[ $file_count -gt 0 ]]; then
            # Calculate average size in MB
            average_size_mb=$(awk "BEGIN {printf \"%.2f\", $total_size_bytes / $file_count / 1024 / 1024}")
            echo "Average file size in $dir: $average_size_mb MB"
        else
            echo "No files found in $dir"
        fi
    else
        echo "Directory $dir does not exist"
    fi
done