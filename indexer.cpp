#include "indexer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <queue>
#include <codecvt>
#include <cmath>

// Реализация QueryNode

std::vector<int> TermNode::evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const {
    auto it = index.find(term);
    if (it != index.end()) {
        std::vector<int> result;
        result.reserve(it->second.size());
        for (const auto& posting : it->second) {
            result.push_back(posting.docId);
        }
        return result;
    }
    return {};
}

void TermNode::print() const {
    std::wcout << L"Термин: " << term << std::endl;
}

void TermNode::collectTerms(std::vector<std::wstring>& terms) const {
    terms.push_back(term);
}

std::vector<int> AndNode::evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const {
    std::vector<int> leftResult = left->evaluate(index, allDocs);
    std::vector<int> rightResult = right->evaluate(index, allDocs);
    
    if (leftResult.empty() || rightResult.empty()) {
        return {};
    }
    
    return intersect(leftResult, rightResult);
}

void AndNode::print() const {
    std::wcout << L"(AND ";
    left->print();
    std::wcout << L" ";
    right->print();
    std::wcout << L")" << std::endl;
}

void AndNode::collectTerms(std::vector<std::wstring>& terms) const {
    left->collectTerms(terms);
    right->collectTerms(terms);
}

std::vector<int> OrNode::evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const {
    std::vector<int> leftResult = left->evaluate(index, allDocs);
    std::vector<int> rightResult = right->evaluate(index, allDocs);
    
    if (leftResult.empty()) return rightResult;
    if (rightResult.empty()) return leftResult;
    
    return unite(leftResult, rightResult);
}

void OrNode::print() const {
    std::wcout << L"(OR ";
    left->print();
    std::wcout << L" ";
    right->print();
    std::wcout << L")" << std::endl;
}

void OrNode::collectTerms(std::vector<std::wstring>& terms) const {
    left->collectTerms(terms);
    right->collectTerms(terms);
}

std::vector<int> NotNode::evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const {
    std::vector<int> childResult = child->evaluate(index, allDocs);
    return complement(childResult, allDocs);
}

void NotNode::print() const {
    std::wcout << L"(NOT ";
    child->print();
    std::wcout << L")" << std::endl;
}

void NotNode::collectTerms(std::vector<std::wstring>& terms) const {
    child->collectTerms(terms);
}

// Реализация QueryParser

// Вспомогательный метод: вставка неявных AND между терминами
std::vector<QueryParser::Token> QueryParser::insertImplicitAnd(const std::vector<Token>& tokens) {
    std::vector<Token> result;
    result.reserve(tokens.size() * 2);
    
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& current = tokens[i];
        result.push_back(current);
        
        // Если текущий токен может быть слева от неявного AND
        // А следующий токен может быть справа (TERM или LPAREN)
        if (i + 1 < tokens.size()) {
            const Token& next = tokens[i + 1];
            // Текущий: TERM, RPAREN
            // Следующий: TERM, LPAREN, NOT
            bool currentCanHaveAnd = (current.type == Token::TERM || current.type == Token::RPAREN);
            bool nextNeedsAnd = (next.type == Token::TERM || next.type == Token::LPAREN || next.type == Token::NOT);
            
            if (currentCanHaveAnd && nextNeedsAnd) {
                result.push_back({Token::AND, L""});
            }
        }
    }
    
    return result;
}

std::vector<QueryParser::Token> QueryParser::tokenizeQuery(const std::wstring& query) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    while (i < query.length()) {
        // Пропускаем пробелы
        if (std::iswspace(query[i])) {
            i++;
            continue;
        }
        
        // Проверяем операторы и скобки (регистронезависимо)
        if (i + 3 <= query.length()) {
            std::wstring substr = query.substr(i, 3);
            std::transform(substr.begin(), substr.end(), substr.begin(), ::towlower);
            if (substr == L"and") {
                tokens.push_back({Token::AND, L""});
                i += 3;
                continue;
            }
            if (substr == L"not") {
                tokens.push_back({Token::NOT, L""});
                i += 3;
                continue;
            }
        }
        
        if (i + 2 <= query.length()) {
            std::wstring substr = query.substr(i, 2);
            std::transform(substr.begin(), substr.end(), substr.begin(), ::towlower);
            if (substr == L"or") {
                tokens.push_back({Token::OR, L""});
                i += 2;
                continue;
            }
        }
        
        if (query[i] == L'(') {
            tokens.push_back({Token::LPAREN, L""});
            i++;
            continue;
        }
        
        if (query[i] == L')') {
            tokens.push_back({Token::RPAREN, L""});
            i++;
            continue;
        }
        
        // Проверяем кавычки для фраз
        if (query[i] == L'"' || query[i] == L'\"') {
            i++;
            size_t start = i;
            while (i < query.length() && query[i] != L'"' && query[i] != L'\"') {
                i++;
            }
            std::wstring phrase = query.substr(start, i - start);
            if (!phrase.empty()) {
                // Фразы пока обрабатываем как один термин (заглушка)
                tokens.push_back({Token::TERM, phrase});
            }
            if (i < query.length() && (query[i] == L'"' || query[i] == L'\"')) {
                i++;
            }
            continue;
        }
        
        // Читаем термин (до пробела или оператора)
        size_t start = i;
        while (i < query.length() && !std::iswspace(query[i])) {
            // Проверяем, не начинается ли оператор
            if (i + 3 <= query.length()) {
                std::wstring substr = query.substr(i, 3);
                std::transform(substr.begin(), substr.end(), substr.begin(), ::towlower);
                if (substr == L"and" || substr == L"not") break;
            }
            if (i + 2 <= query.length()) {
                std::wstring substr = query.substr(i, 2);
                std::transform(substr.begin(), substr.end(), substr.begin(), ::towlower);
                if (substr == L"or") break;
            }
            if (query[i] == L'(' || query[i] == L')' || query[i] == L'"' || query[i] == L'\"') break;
            i++;
        }
        
        std::wstring term = query.substr(start, i - start);
        if (!term.empty()) {
            tokens.push_back({Token::TERM, term});
        }
    }
    
    tokens.push_back({Token::END, L""});
    return tokens;
}

std::unique_ptr<QueryNode> QueryParser::parseExpression(std::vector<Token>& tokens, size_t& pos) {
    std::unique_ptr<QueryNode> node = parseTerm(tokens, pos);
    
    // Только OR на этом уровне (низший приоритет)
    while (pos < tokens.size() && tokens[pos].type == Token::OR) {
        pos++;
        std::unique_ptr<QueryNode> right = parseTerm(tokens, pos);
        node = std::make_unique<OrNode>(std::move(node), std::move(right));
    }
    
    return node;
}

std::unique_ptr<QueryNode> QueryParser::parseTerm(std::vector<Token>& tokens, size_t& pos) {
    std::unique_ptr<QueryNode> node = parseFactor(tokens, pos);
    
    // Только AND на этом уровне (средний приоритет)
    while (pos < tokens.size() && tokens[pos].type == Token::AND) {
        pos++;
        std::unique_ptr<QueryNode> right = parseFactor(tokens, pos);
        node = std::make_unique<AndNode>(std::move(node), std::move(right));
    }
    
    return node;
}

std::unique_ptr<QueryNode> QueryParser::parseFactor(std::vector<Token>& tokens, size_t& pos) {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Неожиданный конец запроса");
    }
    
    Token token = tokens[pos];
    
    if (token.type == Token::NOT) {
        pos++;
        std::unique_ptr<QueryNode> child = parseFactor(tokens, pos);
        return std::make_unique<NotNode>(std::move(child));
    } else if (token.type == Token::LPAREN) {
        pos++;
        std::unique_ptr<QueryNode> expr = parseExpression(tokens, pos);
        
        if (pos >= tokens.size() || tokens[pos].type != Token::RPAREN) {
            throw std::runtime_error("Ожидалась закрывающая скобка");
        }
        pos++;
        return expr;
    } else if (token.type == Token::TERM) {
        pos++;
        // Нормализуем термин: приводим к нижнему регистру и стемминг
        std::wstring normalized = Tokenizer::normalize(token.value);
        std::wstring stemmed = Stemmer::stem(normalized);
        return std::make_unique<TermNode>(stemmed);
    } else {
        throw std::runtime_error("Неожиданный токен в запросе: ожидался термин, NOT или '('");
    }
}

std::unique_ptr<QueryNode> QueryParser::parse(const std::string& query) {
    try {
        // Конвертируем UTF-8 в wide string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wquery = converter.from_bytes(query);
        
        auto tokens = tokenizeQuery(wquery);
        // Вставляем неявные AND между терминами
        tokens = insertImplicitAnd(tokens);
        size_t pos = 0;
        return parseExpression(tokens, pos);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Ошибка парсинга запроса: ") + e.what());
    }
}

double QueryParser::computeIDF(int docFreq, int totalDocs) {
    if (docFreq == 0) return 0.0;
    return std::log(static_cast<double>(totalDocs) / docFreq);
}

double QueryParser::computeTF(int tf, int docLength, double k1) {
    return static_cast<double>(tf) * (k1 + 1.0) / (tf + k1 * docLength);
}

std::vector<QueryParser::BM25Score> QueryParser::rankWithBM25(
    const std::unique_ptr<QueryNode>& ast,
    const MyHashMap<std::wstring, std::vector<Posting>>& index,
    const MyHashMap<int, int>& docLengths,
    const std::vector<int>& allDocs,
    double avgDocLength,
    double k1,
    double b
) {
    int totalDocs = static_cast<int>(allDocs.size());
    
    // 1. Выполняем булев запрос через AST (AND, OR, NOT)
    std::vector<int> matchingDocs = ast->evaluate(index, allDocs);
    
    if (matchingDocs.empty()) {
        return {};
    }
    
    // 2. Собираем все уникальные термины из запроса
    std::vector<std::wstring> queryTerms;
    ast->collectTerms(queryTerms);
    std::sort(queryTerms.begin(), queryTerms.end());
    queryTerms.erase(std::unique(queryTerms.begin(), queryTerms.end()), queryTerms.end());
    
    // 3. Вычисляем BM25 score для каждого документа, прошедшего булеву фильтрацию
    std::vector<BM25Score> results;
    results.reserve(matchingDocs.size());
    
    for (int docId : matchingDocs) {
        double docScore = 0.0;
        int docLength = docLengths.at(docId);
        
        for (const auto& term : queryTerms) {
            auto it = index.find(term);
            if (it == index.end()) continue;
            
            // Находим TF для этого документа
            int tf = 0;
            for (const auto& posting : it->second) {
                if (posting.docId == docId) {
                    tf = posting.tf;
                    break;
                }
            }
            
            if (tf > 0) {
                int docFreq = static_cast<int>(it->second.size());
                double idf = computeIDF(docFreq, totalDocs);
                double score = idf * computeTF(tf, docLength, k1);
                docScore += score;
            }
        }
        
        results.push_back({docId, docScore});
    }
    
    // 4. Сортируем по убыванию score
    std::sort(results.begin(), results.end(),
              [](const BM25Score& a, const BM25Score& b) { return a.score > b.score; });
    
    return results;
}

// Реализация RankedBooleanIndex

RankedBooleanIndex::RankedBooleanIndex() : avgDocLength(0.0), docCount(0) {}

RankedBooleanIndex::~RankedBooleanIndex() {}

void RankedBooleanIndex::addDocument(int docId, const std::string& content) {
    processDocument(docId, content);
    docCount++;
    allDocs.push_back(docId);
}

void RankedBooleanIndex::processDocument(int docId, const std::string& content) {
    // Токенизация
    std::vector<std::wstring> tokens = Tokenizer::tokenizeUTF8(content);
    
    // Сохраняем длину документа
    int docLength = static_cast<int>(tokens.size());
    docLengths[docId] = docLength;
    
    // Стемминг и сбор уникальных терминов с частотами
    MyHashMap<std::wstring, int> termFreqs;
    
    for (const auto& token : tokens) {
        std::wstring stemmed = Stemmer::stem(token);
        if (stemmed.length() >= 2) {
            termFreqs[stemmed]++;
        }
    }
    
    // Добавление в обратный индекс с частотами
    for (const auto& pair : termFreqs) {
        index[pair.first].push_back({docId, pair.second});
    }
}

void RankedBooleanIndex::buildIndex(const std::vector<Document>& documents) {
    index.clear();
    docLengths.clear();
    allDocs.clear();
    docCount = 0;
    
    for (const auto& doc : documents) {
        addDocument(doc.id, doc.content);
    }
    
    // Сортировка posting lists
    for (auto& pair : index) {
        std::sort(pair.second.begin(), pair.second.end(), 
                  [](const Posting& a, const Posting& b) { return a.docId < b.docId; });
    }
    
    // Вычисление средней длины документа
    if (!docLengths.empty()) {
        double sum = 0.0;
        for (const auto& pair : docLengths) {
            sum += pair.second;
        }
        avgDocLength = sum / docLengths.size();
    }
    
    std::cout << "Индекс построен: " << index.size() << " уникальных терминов, "
              << docCount << " документов, avgDocLength = " << avgDocLength << std::endl;
}

std::vector<QueryParser::BM25Score> RankedBooleanIndex::search(const std::string& query) {
    try {
        auto ast = QueryParser::parse(query);
        return QueryParser::rankWithBM25(ast, index, docLengths, allDocs, avgDocLength);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка поиска: " << e.what() << std::endl;
        return {};
    }
}

void RankedBooleanIndex::save(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Ошибка открытия файла для сохранения: " << filename << std::endl;
        return;
    }
    
    // Записываем версию формата
    uint32_t version = 2; // Версия 2: с docLengths и avgDocLength
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    // Записываем количество документов
    out.write(reinterpret_cast<const char*>(&docCount), sizeof(docCount));
    
    // Записываем среднюю длину документа
    out.write(reinterpret_cast<const char*>(&avgDocLength), sizeof(avgDocLength));
    
    // Записываем количество документов с длинами
    uint32_t docLengthsSize = static_cast<uint32_t>(docLengths.size());
    out.write(reinterpret_cast<const char*>(&docLengthsSize), sizeof(docLengthsSize));
    
    // Записываем длины документов
    for (const auto& pair : docLengths) {
        out.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
        out.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
    }
    
    // Записываем размер индекса
    uint64_t indexSize = index.size();
    out.write(reinterpret_cast<const char*>(&indexSize), sizeof(indexSize));
    
    // Записываем индекс
    for (const auto& pair : index) {
        // Сохраняем термин (как UTF-8)
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string termUTF8 = converter.to_bytes(pair.first);
        
        uint32_t termLen = static_cast<uint32_t>(termUTF8.size());
        out.write(reinterpret_cast<const char*>(&termLen), sizeof(termLen));
        out.write(termUTF8.c_str(), termLen);
        
        // Сохраняем posting list
        uint32_t postingsSize = static_cast<uint32_t>(pair.second.size());
        out.write(reinterpret_cast<const char*>(&postingsSize), sizeof(postingsSize));
        out.write(reinterpret_cast<const char*>(pair.second.data()),
                  postingsSize * sizeof(Posting));
    }
    
    out.close();
    std::cout << "Индекс сохранен в: " << filename << " (версия " << version << ")" << std::endl;
}

void RankedBooleanIndex::load(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Ошибка открытия файла для загрузки: " << filename << std::endl;
        return;
    }
    
    index.clear();
    docLengths.clear();
    allDocs.clear();
    
    // Читаем версию формата
    uint32_t version;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (version == 1) {
        // Старый формат (простой BooleanIndex)
        std::cout << "Загрузка индекса в старом формате (версия 1), конвертация..." << std::endl;
        
        // Читаем старый формат
        in.read(reinterpret_cast<char*>(&docCount), sizeof(docCount));
        
        uint64_t indexSize;
        in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));
        
        // Для совместимости: создаём docLengths и allDocs на основе docCount
        avgDocLength = 10.0; // Значение по умолчанию
        for (int i = 0; i < static_cast<int>(docCount); i++) {
            docLengths[i] = 10; // Фиктивная длина
            allDocs.push_back(i);
        }
        
        // Загружаем индекс (старый формат: только docId)
        for (uint64_t i = 0; i < indexSize; i++) {
            uint32_t termLen;
            in.read(reinterpret_cast<char*>(&termLen), sizeof(termLen));
            std::string termUTF8(termLen, '\0');
            in.read(&termUTF8[0], termLen);
            
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::wstring term = converter.from_bytes(termUTF8);
            
            uint32_t postingsSize;
            in.read(reinterpret_cast<char*>(&postingsSize), sizeof(postingsSize));
            std::vector<int> postings(postingsSize);
            in.read(reinterpret_cast<char*>(postings.data()), postingsSize * sizeof(int));
            
            // Конвертируем в новый формат с tf=1
            std::vector<Posting> newPostings;
            newPostings.reserve(postingsSize);
            for (int docId : postings) {
                newPostings.push_back({docId, 1});
            }
            index[term] = std::move(newPostings);
        }
        
        std::cout << "Индекс конвертирован в новый формат" << std::endl;
    } else if (version == 2) {
        // Новый формат
        in.read(reinterpret_cast<char*>(&docCount), sizeof(docCount));
        in.read(reinterpret_cast<char*>(&avgDocLength), sizeof(avgDocLength));
        
        uint32_t docLengthsSize;
        in.read(reinterpret_cast<char*>(&docLengthsSize), sizeof(docLengthsSize));
        
        for (uint32_t i = 0; i < docLengthsSize; i++) {
            int docId, length;
            in.read(reinterpret_cast<char*>(&docId), sizeof(docId));
            in.read(reinterpret_cast<char*>(&length), sizeof(length));
            docLengths[docId] = length;
            allDocs.push_back(docId);
        }
        // Проверяем соответствие docCount и docLengths/allDocs
        if (docLengths.size() != static_cast<size_t>(docCount) || allDocs.size() != static_cast<size_t>(docCount)) {
            std::cerr << "Предупреждение: docCount (" << docCount << ") не совпадает с docLengths (" << docLengths.size() << ") или allDocs (" << allDocs.size() << "). Исправляем docCount." << std::endl;
            docCount = static_cast<int>(docLengths.size());
        }
        
        uint64_t indexSize;
        in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));
        
        for (uint64_t i = 0; i < indexSize; i++) {
            uint32_t termLen;
            in.read(reinterpret_cast<char*>(&termLen), sizeof(termLen));
            std::string termUTF8(termLen, '\0');
            in.read(&termUTF8[0], termLen);
            
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::wstring term = converter.from_bytes(termUTF8);
            
            uint32_t postingsSize;
            in.read(reinterpret_cast<char*>(&postingsSize), sizeof(postingsSize));
            std::vector<Posting> postings(postingsSize);
            in.read(reinterpret_cast<char*>(postings.data()), postingsSize * sizeof(Posting));
            
            index[term] = std::move(postings);
        }
    } else {
        std::cerr << "Неизвестная версия формата индекса: " << version << std::endl;
        in.close();
        return;
    }
    
    in.close();
    std::cout << "Индекс загружен из: " << filename << std::endl;
    std::cout << "Документов: " << docCount << ", терминов: " << index.size() << std::endl;
}

