#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>

using namespace std;

uint16_t read_uint16(ifstream& file) {
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint32_t read_uint32(ifstream& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

class FAT16 {
public:
    uint16_t sector_size;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t fat_size_sectors;
    uint16_t root_dir_start;
    uint16_t data_start;
    vector<uint16_t> fat_table;

    FAT16(ifstream& file) {
        file.seekg(11, ios::beg);
        sector_size = read_uint16(file);
        sectors_per_cluster = file.get();
        reserved_sectors = read_uint16(file);
        fat_count = file.get();
        root_entries = read_uint16(file);
        fat_size_sectors = read_uint16(file);

        root_dir_start = reserved_sectors + fat_count * fat_size_sectors;
        data_start = root_dir_start + (root_entries * 32 + sector_size - 1) / sector_size;

        file.seekg(reserved_sectors * sector_size, ios::beg);
        for (int i = 0; i < fat_size_sectors * sector_size / 2; ++i) {
            fat_table.push_back(read_uint16(file));
        }
    }

    uint16_t get_next_cluster(uint16_t cluster) const {
        return fat_table[cluster];
    }

    uint32_t cluster_to_sector(uint16_t cluster) const {
        return data_start + (cluster - 2) * sectors_per_cluster;
    }
};

void print_file_info(const string& path, const string& name, uint32_t size, bool is_directory) {
    cout << path << (path.empty() ? "" : "/") << name << (is_directory ? "/" : "") 
         << " (Size: " << size << " bytes, Type: " 
         << (is_directory ? "Directory" : "File") << ")" << endl;
}
void read_directory(ifstream& file, const FAT16& fat, uint16_t cluster, const string& path) {
    uint32_t sector = cluster == 0 
                        ? fat.root_dir_start 
                        : fat.cluster_to_sector(cluster);
    uint32_t max_entries = cluster == 0 
                             ? fat.root_entries 
                             : fat.sector_size * fat.sectors_per_cluster / 32;

    for (uint32_t i = 0; i < max_entries; ++i) {
        file.seekg(sector * fat.sector_size + i * 32, ios::beg);

        char entry[32];
        file.read(entry, sizeof(entry));
        if (entry[0] == 0x00) break;
        if (entry[0] == 0xE5) continue;

        string name(entry, 8);
        string ext(entry + 8, 3);
        uint8_t attributes = entry[11];
        uint16_t start_cluster = *reinterpret_cast<uint16_t*>(&entry[26]);
        uint32_t file_size = *reinterpret_cast<uint32_t*>(&entry[28]);

        name.erase(name.find_last_not_of(' ') + 1);
        ext.erase(ext.find_last_not_of(' ') + 1);
        string full_name = name + (ext.empty() ? "" : "." + ext);
        bool is_directory = attributes & 0x10;

        print_file_info(path, full_name, file_size, is_directory);

        if (is_directory && full_name != "." && full_name != "..") {
            uint16_t next_cluster = start_cluster;
            while (next_cluster < 0xFFF8) { // FAT16 cluster chain ends at 0xFFF8
                read_directory(file, fat, next_cluster, path + "/" + full_name);
                next_cluster = fat.get_next_cluster(next_cluster);
            }
        }
    }
}

void read_fat16_image(const string& image_path) {
    ifstream file(image_path, ios::binary);
    if (!file) {
        cerr << "Could not open the file: " << image_path << endl;
        return;
    }
    FAT16 fat(file);
    cout << "FAT16 File System Information:" << endl;
    cout << "Sector size: " << fat.sector_size << " bytes" << endl;
    cout << "Sectors per cluster: " << (int)fat.sectors_per_cluster << endl;
    cout << "Number of FAT copies: " << (int)fat.fat_count << endl;
    cout << "FAT size (in sectors): " << fat.fat_size_sectors << endl;
    cout << "Root directory entries: " << fat.root_entries << endl;
    cout << "Reserved sectors: " << fat.reserved_sectors << endl;
    cout << "\nRoot Directory:" << endl;
    read_directory(file, fat, 0, "");

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <image-file-path>" << endl;
        return 1;
    }

    string image_path = argv[1];
    read_fat16_image(image_path);

    return 0;
}
