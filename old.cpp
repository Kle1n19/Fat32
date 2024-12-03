#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>

uint16_t read_uint16(std::ifstream& file) {
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint32_t read_uint32(std::ifstream& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}
uint8_t read_uint8(std::ifstream& file) {
    uint8_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}
void print_fat16_file_info(std::ifstream& file, uint16_t root_dir_start, uint16_t root_entries, uint16_t sector_size, uint8_t sectors_per_cluster) {
    file.seekg(root_dir_start * sector_size, std::ios::beg);

    std::cout << "Root Directory Content:" << std::endl;

    for (int i = 0; i < root_entries; ++i) {
        char entry[32];
        file.read(entry, sizeof(entry));

        if (entry[0] == 0x00) continue; // Unused entry
        if (entry[0] == 0xE5) continue; // Deleted entry

        std::string filename(entry, 8);
        std::string ext(entry + 8, 3);
        filename.erase(filename.find_last_not_of(' ') + 1);
        ext.erase(ext.find_last_not_of(' ') + 1);

        std::string full_name = filename;
        if (!ext.empty()) {
            full_name += "." + ext;
        }

        uint32_t file_size = *reinterpret_cast<uint32_t*>(&entry[28]);
        uint8_t attributes = entry[11];
        uint16_t date = *reinterpret_cast<uint16_t*>(&entry[24]);
        uint16_t time = *reinterpret_cast<uint16_t*>(&entry[22]);
        uint16_t first_cluster = *reinterpret_cast<uint16_t*>(&entry[26]);
        int day = date & 0x1F;
        int month = (date >> 5) & 0x0F;
        int year = ((date >> 9) & 0x7F) + 1980;
        int second = (time & 0x1F) * 2;
        int minute = (time >> 5) & 0x3F;
        int hour = (time >> 11) & 0x1F;
        uint32_t first_sector = root_dir_start + (first_cluster - 2) * sectors_per_cluster;
        std::cout << "File: " << full_name << std::endl;
        std::cout << "Size: " << file_size << " bytes" << std::endl;
        std::cout << "Last Modified: " << year << "-" << month << "-" << day << " "
                  << hour << ":" << minute << ":" << second << std::endl;
        std::cout << "Attributes: ";
        if (attributes & 0x01) std::cout << "Read-Only ";
        if (attributes & 0x02) std::cout << "Hidden ";
        if (attributes & 0x04) std::cout << "System ";
        if (attributes & 0x08) std::cout << "Volume Label ";
        if (attributes & 0x10) std::cout << "Directory ";
        if (attributes & 0x20) std::cout << "Archive ";
        std::cout << std::endl;
        std::cout << "First Cluster: " << first_cluster << std::endl;
        std::cout << "First Sector: " << first_sector << std::endl;
        std::cout << "----------------------------------" << std::endl;
    }
}

void read_fat16_image(const std::string& image_path) {
    std::ifstream file(image_path, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open the file: " << image_path << std::endl;
        return;
@@ -97,25 +92,39 @@ void read_fat16_image(const std::string& image_path) {
    uint16_t root_entries = *reinterpret_cast<uint16_t*>(&boot_sector[17]);
    uint16_t reserved_sectors = *reinterpret_cast<uint16_t*>(&boot_sector[14]);

    uint16_t root_dir_start = reserved_sectors + fat_count * fat_size_sectors;
    uint32_t fat_size_bytes = fat_size_sectors * sector_size;
    uint32_t root_dir_size = root_entries * 32;
    uint16_t signature = *reinterpret_cast<uint16_t*>(&boot_sector[510]);

    std::cout << "FAT16 File System Information:" << std::endl;
    std::cout << "Sector size: " << sector_size << " bytes" << std::endl;
    std::cout << "Sectors per cluster: " << static_cast<int>(sectors_per_cluster) << std::endl;
    std::cout << "Number of FAT copies: " << static_cast<int>(fat_count) << std::endl;
    std::cout << "FAT size (in sectors): " << fat_size_sectors << std::endl;
    std::cout << "FAT size (in bytes): " << fat_size_bytes << std::endl;
    std::cout << "Root directory entries: " << root_entries << std::endl;
    std::cout << "Root directory size: " << root_dir_size << " bytes" << std::endl;
    std::cout << "Reserved sectors: " << reserved_sectors << std::endl;
    std::cout << "Valid boot signature: " << (signature == 0xAA55 ? "Yes" : "No") << std::endl;

    std::cout << "----------------------------------" << std::endl;

    print_fat16_file_info(file, root_dir_start, root_entries, sector_size, sectors_per_cluster);

    file.close();
}
@@ -127,7 +136,7 @@ int main(int argc, char* argv[]) {
    }

    std::string image_path = argv[1];
    read_fat16_image(image_path);

    return 0;
}
