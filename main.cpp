#include "indexer.h"
#include "tokenizer.h"
#include "stemmer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

// Загрузка документа из простого текстового формата
// Формат: id|url|title|length
// Затем идет пустая строка и содержимое документа
bool loadDocument(const std::string& filename, Document& doc) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    // Читаем метаданные
    if (!std::getline(file, line)) return false;
    doc.id = std::stoi(line);

    if (!std::getline(file, line)) return false;
    doc.url = line;

    if (!std::getline(file, line)) return false;
    doc.title = line;

    if (!std::getline(file, line)) return false;
    doc.length = std::stoi(line);

    // Пропускаем пустую строку
    if (!std::getline(file, line)) return false;

    // Читаем все оставшееся как содержимое
    std::ostringstream contentStream;
    contentStream << file.rdbuf();
    doc.content = contentStream.str();

    return true;
}

void buildIndexFromDirectory(const std::string& dirPath, RankedBooleanIndex& index) {
    std::vector<Document> documents;
    std::cout << "Загрузка документов из: " << dirPath << std::endl;

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.path().extension() == ".txt") {
            Document doc;
            if (loadDocument(entry.path().string(), doc)) {
                // Используем ID из файла (уже установлен в loadDocument)
                documents.push_back(doc);

                if (documents.size() % 1000 == 0) {
                    std::cout << "Загружено документов: " << documents.size() << std::endl;
                }
            }
        }
    }

    std::cout << "Всего загружено: " << documents.size() << " документов" << std::endl;
    index.buildIndex(documents);
}

void interactiveSearch(RankedBooleanIndex& index) {
    std::string query;
    std::cout << "\n=== ПОИСК С BM25 РАНЖИРОВАНИЕМ ===" << std::endl;
    std::cout << "Поддерживаемые операторы: AND, OR, NOT, скобки ()" << std::endl;
    std::cout << "Примеры запросов:" << std::endl;
    std::cout << "  jedi AND sith" << std::endl;
    std::cout << "  (jedi OR sith) AND laser" << std::endl;
    std::cout << "  NOT jedi" << std::endl;
    std::cout << "Для выхода введите 'exit'" << std::endl;

    while (true) {
        std::cout << "\nЗапрос: ";
        std::getline(std::cin, query);

        if (query == "exit") {
            break;
        }

        if (query.empty()) {
            continue;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto results = index.search(query);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end - start;

        std::cout << "Найдено документов: " << results.size() << std::endl;
        std::cout << "Время поиска: " << elapsed.count() * 1000 << " мс" << std::endl;

        if (!results.empty()) {
            std::cout << "\nРезультаты (сортировка по BM25 score):" << std::endl;
            std::cout << std::fixed << std::setprecision(6);
            for (size_t i = 0; i < std::min(results.size(), size_t(20)); i++) {
                std::cout << "  [" << i + 1 << "] Документ ID: " << results[i].docId
                          << ", BM25 score: " << results[i].score << std::endl;
            }
            if (results.size() > 20) {
                std::cout << "  ... и еще " << (results.size() - 20) << " документов" << std::endl;
            }
        }
    }
}

void printIndexStats(const RankedBooleanIndex& index) {
    std::cout << "\n=== СТАТИСТИКА ИНДЕКСА ===" << std::endl;
    std::cout << "Количество документов: " << index.getTotalDocuments() << std::endl;
    std::cout << "Количество уникальных терминов: " << index.getIndexSize() << std::endl;
    std::cout << "Средняя длина документа: " << std::fixed << std::setprecision(2)
              << index.getAvgDocLength() << " токенов" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование:" << std::endl;
        std::cerr << "  " << argv[0] << " <папка_с_документами> [индекс.bin]" << std::endl;
        std::cerr << "  " << argv[0] << " <индекс.bin> --search" << std::endl;
        return 1;
    }

    std::string indexPath = "index.bin";
    std::string documentsPath;
    bool useRanking = true; // По умолчанию используем ранжирование

    // Парсим аргументы командной строки
    if (argc >= 3) {
        if (std::string(argv[2]) == "--search") {
            indexPath = argv[1];
        } else if (std::string(argv[2]) == "--no-rank") {
            documentsPath = argv[1];
            useRanking = false;
            if (argc >= 4) {
                indexPath = argv[3];
            }
        } else {
            documentsPath = argv[1];
            indexPath = argv[2];
            if (argc >= 4 && std::string(argv[3]) == "--no-rank") {
                useRanking = false;
            }
        }
    } else if (argc == 2 && std::string(argv[1]) != "--search") {
        documentsPath = argv[1];
    } else {
        std::cerr << "Укажите папку с документами" << std::endl;
        return 1;
    }

    RankedBooleanIndex index;

    // Проверяем, нужно ли строить индекс или загружать существующий
    if (!documentsPath.empty() && fs::exists(documentsPath)) {
        std::cout << "Построение индекса..." << std::endl;
        buildIndexFromDirectory(documentsPath, index);
        index.save(indexPath);
    } else if (fs::exists(indexPath)) {
        std::cout << "Загрузка индекса из: " << indexPath << std::endl;
        index.load(indexPath);
    } else {
        std::cerr << "Не найден индекс и не указана папка с документами" << std::endl;
        return 1;
    }

    printIndexStats(index);

    interactiveSearch(index);

    return 0;
}
