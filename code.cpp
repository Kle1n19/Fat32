#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

// Helper function to convert a directory entry to a readable filename.
std::string get_filename(const char* entry) {
    std::string filename(entry, 8);
    std::string ext(entry + 8, 3);

    // Remove padding spaces.
    filename.erase(filename.find_last_not_of(' ') + 1);
    ext.erase(ext.find_last_not_of(' ') + 1);

    if (!ext.empty()) {
        filename += "." + ext;
    }
    return filename;
}

// Recursive function to traverse directories and write all file paths.
void list_files_recursively(std::ifstream& file, uint16_t sector_size, uint16_t cluster_size, 
                            uint32_t current_cluster, const std::string& path, 
                            uint16_t root_dir_start, uint16_t data_region_start) {
    uint32_t cluster_start = data_region_start + (current_cluster - 2) * cluster_size;

    file.seekg(cluster_start * sector_size, std::ios::beg);

    for (uint16_t offset = 0; offset < cluster_size * sector_size; offset += 32) {
        char entry[32];
        file.read(entry, sizeof(entry));

        if (entry[0] == 0x00) break; // End of directory
        if (entry[0] == 0xE5) continue; // Deleted entry
        if (entry[11] & 0x08) continue; // Volume label

        std::string filename = get_filename(entry);

        uint8_t attributes = entry[11];
        uint16_t first_cluster = *reinterpret_cast<uint16_t*>(&entry[26]);

        if (attributes & 0x10) { // Directory
            if (filename != "." && filename != "..") {
                // Recursively explore the subdirectory.
                list_files_recursively(file, sector_size, cluster_size, first_cluster, 
                                       path + "/" + filename, root_dir_start, data_region_start);
            }
        } else { // File
            std::cout << path + "/" + filename << std::endl;
        }
    }
}

// Wrapper function to start the traversal from the root directory.
void traverse_fat16_filesystem(const std::string& image_path) {
    std::ifstream file(image_path, std::ios::binary);

    if (!file) {
        std::cerr << "Could not open the file: " << image_path << std::endl;
        return;
    }

    char boot_sector[512];
    file.read(boot_sector, sizeof(boot_sector));

    uint16_t sector_size = *reinterpret_cast<uint16_t*>(&boot_sector[11]);
    uint8_t sectors_per_cluster = boot_sector[13];
    uint16_t fat_count = boot_sector[16];
    uint16_t fat_size_sectors = *reinterpret_cast<uint16_t*>(&boot_sector[22]);
    uint16_t root_entries = *reinterpret_cast<uint16_t*>(&boot_sector[17]);
    uint16_t reserved_sectors = *reinterpret_cast<uint16_t*>(&boot_sector[14]);

    uint16_t root_dir_start = reserved_sectors + fat_count * fat_size_sectors;
    uint16_t data_region_start = root_dir_start + (root_entries * 32 + sector_size - 1) / sector_size;

    std::cout << "File Paths:" << std::endl;
    list_files_recursively(file, sector_size, sectors_per_cluster, 0, "", root_dir_start, data_region_start);

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <image-file-path>" << std::endl;
        return 1;
    }

    std::string image_path = argv[1];
    traverse_fat16_filesystem(image_path);

    return 0;
}
