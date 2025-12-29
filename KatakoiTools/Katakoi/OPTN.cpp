#include <cstdint>

import std;
namespace fs = std::filesystem;

struct Sentence {
    size_t offset;
    uint32_t len;
    std::string text;
};

template<typename T>
T read(void* ptr)
{
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    return value;
}

template<typename T>
void write(void* ptr, const T& value)
{
    std::memcpy(ptr, &value, sizeof(T));
}

void dumpText(const fs::path& optnPath, const fs::path& outPath)
{
    std::ifstream ifs(optnPath, std::ios::binary);
    std::ofstream ofs(outPath);

    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(ifs), {});
    for (size_t i = 0; i < buffer.size() - 4; i++) {
        if (buffer[i] != 0x01) {
            continue;
        }
        if (buffer[i + 1] <= 0x02 || buffer[i + 4] != 0x00) {
            continue;
        }
        uint32_t len = read<uint32_t>(&buffer[i + 1]);
        ofs << i + 1 << ":::::";
        ofs << std::string_view((char*)&buffer[i + 5], len) << std::endl;
        i += 4 + len;
    }

    std::print("Text dump done.\n");
}

void injectText(const fs::path& orgOptnPath, const fs::path& transTextPath, const fs::path& outPath)
{
    std::ifstream orgOptnIfs(orgOptnPath, std::ios::binary);
    std::ifstream transTextIfs(transTextPath);
    std::ofstream outIfs(outPath, std::ios::binary);

    std::vector<uint8_t> orgOptnBuffer(std::istreambuf_iterator<char>(orgOptnIfs), {});
    std::vector<uint8_t> newBuffer;

    std::vector<Sentence> sentences;
    std::string line;
    while (std::getline(transTextIfs, line)) {
        size_t pos = line.find(":::::");
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid line format.");
        }
        size_t offset = std::stoull(line.substr(0, pos));
        std::string text = line.substr(pos + 5);
        sentences.emplace_back(Sentence{ offset, (uint32_t)text.size(), text });
    }

    size_t currentOffset = 0;
    for (const auto& sentence : sentences) {
        newBuffer.insert(newBuffer.end(), orgOptnBuffer.begin() + currentOffset, orgOptnBuffer.begin() + sentence.offset);
        newBuffer.insert(newBuffer.end(), (uint8_t*)&sentence.len, (uint8_t*)&sentence.len + 4);
        newBuffer.insert(newBuffer.end(), (uint8_t*)sentence.text.data(), (uint8_t*)sentence.text.data() + sentence.text.size());
        currentOffset = sentence.offset + 4 + read<uint32_t>(&orgOptnBuffer[sentence.offset]);
    }

    newBuffer.insert(newBuffer.end(), orgOptnBuffer.begin() + currentOffset, orgOptnBuffer.end());

    outIfs.write((char*)newBuffer.data(), newBuffer.size());
    std::print("Text injection done.\n");
}

int main(int argc, char* argv[])
{
    try {
        if (argc < 2) {
            return -1;
        }
        std::string mode = argv[1];
        if (mode == "dump") {
            if (argc < 4) {
                return -2;
            }
            fs::path optnPath = argv[2];
            fs::path outPath = argv[3];
            dumpText(optnPath, outPath);
        }
        else if (mode == "inject") {
            if (argc < 5) {
                return -3;
            }
            fs::path orgOptnPath = argv[2];
            fs::path transTextPath = argv[3];
            fs::path outPath = argv[4];
            injectText(orgOptnPath, transTextPath, outPath);
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
