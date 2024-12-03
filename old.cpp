#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>

// Helper to read 16-bit and 8-bit values from a file stream
uint16_t read_uint16(std::ifstream& file, uint32_t offset) {
    file.seekg(offset, std::ios::beg);
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

// Helper to safely construct a filename
std::string construct_filename(const char* entry) {
    std::string filename(entry, 8);
    std::string extension(entry + 8, 3);

    // Remove padding spaces and ensure characters are valid ASCII
    filename.erase(filename.find_last_not_of(' ') + 1);
    extension.erase(extension.find_last_not_of(' ') + 1);

    if (!std::all_of(filename.begin(), filename.end(), [](char c) { return std::isalnum(c) || c == '_'; })) {
        return ""; // Ignore corrupted filenames
    }

    if (!extension.empty()) {
        return filename + "." + extension;
    }
    return filename;
}

// Read the next cluster in the chain from the FAT table
uint16_t get_next_cluster(std::ifstream& file, uint16_t current_cluster, uint32_t fat_start) {
    uint32_t fat_offset = fat_start * 512 + current_cluster * 2; // Each FAT16 entry is 2 bytes
    return read_uint16(file, fat_offset);
}

// Recursively list files in FAT16 directories
void list_files_recursively(std::ifstream& file, uint16_t sector_size, uint16_t cluster_size, 
                            uint32_t cluster, const std::string& path, uint32_t fat_start, 
                            uint32_t data_start) {
    while (cluster < 0xFFF8 && cluster >= 2) { // FAT16 end-of-chain markers: 0xFFF8–0xFFFF
        uint32_t cluster_offset = data_start + (cluster - 2) * cluster_size * sector_size;
        file.seekg(cluster_offset, std::ios::beg);

        for (uint16_t offset = 0; offset < cluster_size * sector_size; offset += 32) {
            char entry[32];
            file.read(entry, sizeof(entry));

            if (entry[0] == 0x00) return; // End of directory
            if (entry[0] == 0xE5) continue; // Deleted entry
            if (entry[11] & 0x08) continue; // Volume label

            std::string filename = construct_filename(entry);
            if (filename.empty()) continue; // Skip corrupted filenames

            uint8_t attributes = entry[11];
            uint16_t first_cluster = *reinterpret_cast<uint16_t*>(&entry[26]);

            if (attributes & 0x10) { // Directory
                if (filename != "." && filename != "..") {
                    list_files_recursively(file, sector_size, cluster_size, first_cluster, 
                                           path + "/" + filename, fat_start, data_start);
                }
            } else { // File
                std::cout << path + "/" + filename << std::endl;
            }
        }

        // Move to the next cluster in the chain
        cluster = get_next_cluster(file, cluster, fat_start);
    }
}

// Start traversal from the root directory
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

    uint32_t fat_start = reserved_sectors;
    uint32_t root_dir_start = fat_start + fat_count * fat_size_sectors;
    uint32_t data_start = root_dir_start + (root_entries * 32 + sector_size - 1) / sector_size;

    uint32_t root_dir_offset = root_dir_start * sector_size;

    std::cout << "File Paths:" << std::endl;

    for (uint32_t offset = 0; offset < root_entries * 32; offset += 32) {
        file.seekg(root_dir_offset + offset, std::ios::beg);

        char entry[32];
        file.read(entry, sizeof(entry));

        if (entry[0] == 0x00) break; // End of directory
        if (entry[0] == 0xE5) continue; // Deleted entry
        if (entry[11] & 0x08) continue; // Volume label

        std::string filename = construct_filename(entry);
        if (filename.empty()) continue; // Skip corrupted filenames

        uint8_t attributes = entry[11];
        uint16_t first_cluster = *reinterpret_cast<uint16_t*>(&entry[26]);

        if (attributes & 0x10) { // Directory
            if (filename != "." && filename != "..") {
                list_files_recursively(file, sector_size, sectors_per_cluster, first_cluster, 
                                       "/" + filename, fat_start, data_start);
            }
        } else { // File
            std::cout << "/" + filename << std::endl;
        }
    }

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
