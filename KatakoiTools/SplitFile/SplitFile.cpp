#include <Windows.h>
#include <cstdint>

import std;
namespace fs = std::filesystem;

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

struct Sentence {
    size_t offset;
    std::string text;
};

struct File {
    size_t offsetBegin;
    size_t offsetEnd;
    std::string fileName;
    std::vector<Sentence> sentences;
};

std::string& replaceStrInplace(std::string& str, const std::string& org, const std::string& rep) {
    size_t pos = 0;
    while ((pos = str.find(org, pos)) != std::string::npos) {
        str = str.replace(pos, org.length(), rep);
        pos += rep.length();
    }
    return str;
}

int main(int argc, char* argv[])
{
    try{
        if (argc < 2) {
            return -1;
        }

        std::string mode = argv[1];
        if (mode == "split") {
            if (argc < 5) {
                return -11;
            }
            fs::path orgPath(argv[2]);
            fs::path czitPath(argv[3]);
            fs::path outputDir(argv[4]);

            fs::create_directories(outputDir);

            std::ifstream ifs(orgPath);
            std::string line;
            std::vector<Sentence> sentences;
            while (std::getline(ifs, line)) {
                Sentence sentence;
                if (size_t pos = line.find("[pre_unfinish]"); pos == 0)
                {
                    sentence.text = "[pre_unfinish]";
                    line = line.substr(14);
                }
                size_t pos = line.find(":::::");
                if (pos == std::string::npos) {
                    throw std::runtime_error("Invalid sentence format.");
                }
                sentence.offset = std::stoll(line.substr(0, pos));
                sentence.text += line.substr(pos + 5);
                sentences.push_back(sentence);
            }

            ifs.close();
            ifs.open(czitPath, std::ios::binary);
            std::vector<uint8_t> czitData(std::istreambuf_iterator<char>(ifs), {});
            ifs.close();

            uint32_t fileCount = read<uint32_t>(czitData.data());
            size_t currentOffset = 4;
            std::vector<File> files;
            for (uint32_t i = 0; i < fileCount; i++) {
                File file;
                file.fileName = (char*)&czitData[currentOffset];
                currentOffset += file.fileName.size() + 1;
                file.offsetBegin = read<uint32_t>(&czitData[currentOffset]);
                if (!files.empty()) {
                    files.back().offsetEnd = file.offsetBegin;
                }
                else if (files.size() == fileCount - 1) {
                    file.offsetEnd = czitData.size();
                }
                currentOffset += 4;
                files.push_back(file);
            }

            std::ofstream ofs;
            for (auto& file : files) {
                replaceStrInplace(file.fileName, "/", "__SLASH__");
                replaceStrInplace(file.fileName, ".", "__DOT__");
                file.sentences = sentences;
                std::erase_if(file.sentences, [&](const Sentence& s)
                    {
                        return s.offset < file.offsetBegin || s.offset >= file.offsetEnd;
                    });
                ofs.open(outputDir / file.fileName);
                for (const auto& sentence : file.sentences) {
                    ofs << sentence.text << "\n";
                }
                ofs.close();
            }
        }
        else if (mode == "merge") {
            if (argc < 5) {
                return -12;
            }
            fs::path inputDir(argv[2]);
            fs::path czitPath(argv[3]);
            fs::path outputPath(argv[4]);

            std::ifstream ifs(czitPath, std::ios::binary);
            std::vector<uint8_t> czitData(std::istreambuf_iterator<char>(ifs), {});
            ifs.close();

            uint32_t fileCount = read<uint32_t>(czitData.data());
            size_t currentOffset = 4;
            std::vector<std::string> fileNames;
            for (uint32_t i = 0; i < fileCount; i++) {
                std::string fileName = (char*)&czitData[currentOffset];
                currentOffset += fileName.size() + 1;
                replaceStrInplace(fileName, "/", "__SLASH__");
                replaceStrInplace(fileName, ".", "__DOT__");
                fileNames.push_back(fileName);
                currentOffset += 4;
            }

            std::vector<std::string> sentences;
            for (const auto& fileName : fileNames) {
                ifs.open(inputDir / fileName);
                std::string line;
                while (std::getline(ifs, line)) {
                    sentences.push_back(line);
                }
                ifs.close();
            }

            fs::create_directories(outputPath.parent_path());
            std::ofstream ofs(outputPath);
            for (const auto& sentence : sentences) {
                ofs << sentence << "\n";
            }
            ofs.close();
        }
        else {
            return -2;
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -3;
    }

    std::cout << "Done." << std::endl;
    return 0;
}
