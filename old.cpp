#include <iostream>
#include <fstream>
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

uint8_t read_uint8(ifstream& file) {
    uint8_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

void print_fat16_file_info(ifstream& file, uint16_t root_dir_start, uint16_t root_entries, uint16_t sector_size, uint8_t sectors_per_cluster) {
    file.seekg(root_dir_start * sector_size, ios::beg);

    cout << "Root Directory Content:" << endl;

    for (int i = 0; i < root_entries; ++i) {
        char entry[32];
        file.read(entry, sizeof(entry));

        if (entry[0] == 0x00) continue; // Unused entry
        if (entry[0] == 0xE5) continue; // Deleted entry

        string filename(entry, 8);
        string ext(entry + 8, 3);
        filename.erase(filename.find_last_not_of(' ') + 1);
        ext.erase(ext.find_last_not_of(' ') + 1);

        string full_name = filename;
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

        cout << "File: " << full_name << endl;
        cout << "Size: " << file_size << " bytes" << endl;
        cout << "Last Modified: " << year << "-" << month << "-" << day << " "
             << hour << ":" << minute << ":" << second << endl;

        cout << "Attributes: ";
        if (attributes & 0x01) cout << "Read-Only ";
        if (attributes & 0x02) cout << "Hidden ";
        if (attributes & 0x04) cout << "System ";
        if (attributes & 0x08) cout << "Volume Label ";
        if (attributes & 0x10) cout << "Directory ";
        if (attributes & 0x20) cout << "Archive ";

        cout << endl;
        cout << "First Cluster: " << first_cluster << endl;
        cout << "First Sector: " << first_sector << endl;
        cout << "----------------------------------" << endl;
    }
}

void read_fat16_image(const string& image_path) {
    ifstream file(image_path, ios::binary);

    if (!file) {
        cerr << "Could not open the file: " << image_path << endl;
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
    uint32_t fat_size_bytes = fat_size_sectors * sector_size;
    uint32_t root_dir_size = root_entries * 32;
    uint16_t signature = *reinterpret_cast<uint16_t*>(&boot_sector[510]);

    cout << "FAT16 File System Information:" << endl;
    cout << "Sector size: " << sector_size << " bytes" << endl;
    cout << "Sectors per cluster: " << (int)sectors_per_cluster << endl;
    cout << "Number of FAT copies: " << (int)fat_count << endl;
    cout << "FAT size (in sectors): " << fat_size_sectors << endl;
    cout << "FAT size (in bytes): " << fat_size_bytes << endl;
    cout << "Root directory entries: " << root_entries << endl;
    cout << "Root directory size: " << root_dir_size << " bytes" << endl;
    cout << "Reserved sectors: " << reserved_sectors << endl;
    cout << "Valid boot signature: " << (signature == 0xAA55 ? "Yes" : "No") << endl;

    cout << "----------------------------------" << endl;

    print_fat16_file_info(file, root_dir_start, root_entries, sector_size, sectors_per_cluster);

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
