#ifndef INDEXER_H
#define INDEXER_H

#include <string>
#include <vector>
#include <set>
#include <memory>
#include "tokenizer.h"
#include "stemmer.h"
#include "my_hashmap.h"

struct Document {
    int id;
    std::string url;
    std::string title;
    std::string content;
    int length;
};

struct Posting {
    int docId;
    int tf; // term frequency в документе
};

// Abstract Search Tree для запросов
class QueryNode {
public:
    virtual ~QueryNode() = default;
    virtual std::vector<int> evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const = 0;
    virtual void print() const = 0;
    virtual void collectTerms(std::vector<std::wstring>& terms) const = 0;
};

class TermNode : public QueryNode {
public:
    explicit TermNode(std::wstring term) : term(std::move(term)) {}
    
    std::vector<int> evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const override;
    void print() const override;
    void collectTerms(std::vector<std::wstring>& terms) const override;
    
    const std::wstring& getTerm() const { return term; }
    
private:
    std::wstring term;
};

class AndNode : public QueryNode {
public:
    AndNode(std::unique_ptr<QueryNode> left, std::unique_ptr<QueryNode> right)
        : left(std::move(left)), right(std::move(right)) {}
    
    std::vector<int> evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const override;
    void print() const override;
    void collectTerms(std::vector<std::wstring>& terms) const override;
    
private:
    std::unique_ptr<QueryNode> left;
    std::unique_ptr<QueryNode> right;
};

class OrNode : public QueryNode {
public:
    OrNode(std::unique_ptr<QueryNode> left, std::unique_ptr<QueryNode> right)
        : left(std::move(left)), right(std::move(right)) {}
    
    std::vector<int> evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const override;
    void print() const override;
    void collectTerms(std::vector<std::wstring>& terms) const override;
    
private:
    std::unique_ptr<QueryNode> left;
    std::unique_ptr<QueryNode> right;
};

class NotNode : public QueryNode {
public:
    explicit NotNode(std::unique_ptr<QueryNode> child) : child(std::move(child)) {}
    
    std::vector<int> evaluate(const MyHashMap<std::wstring, std::vector<Posting>>& index, const std::vector<int>& allDocs) const override;
    void print() const override;
    void collectTerms(std::vector<std::wstring>& terms) const override;
    
private:
    std::unique_ptr<QueryNode> child;
};

// Парсер запросов
class QueryParser {
public:
    // Парсинг запроса в AST
    static std::unique_ptr<QueryNode> parse(const std::string& query);
    
    struct BM25Score {
        int docId;
        double score;
    };
    
    // Лексический анализ
    struct Token {
        enum Type { TERM, AND, OR, NOT, LPAREN, RPAREN, END };
        Type type;
        std::wstring value;
    };
    
private:
    // Вспомогательные методы
    static std::vector<Token> insertImplicitAnd(const std::vector<Token>& tokens);
    
public:
    static std::vector<BM25Score> rankWithBM25(
        const std::unique_ptr<QueryNode>& ast,
        const MyHashMap<std::wstring, std::vector<Posting>>& index,
        const MyHashMap<int, int>& docLengths,
        const std::vector<int>& allDocs,
        double avgDocLength,
        double k1 = 1.2,
        double b = 0.75
    );
    
    static std::vector<Token> tokenizeQuery(const std::wstring& query);
    
    // Синтаксический анализ (рекурсивный спуск)
    static std::unique_ptr<QueryNode> parseExpression(std::vector<Token>& tokens, size_t& pos);
    static std::unique_ptr<QueryNode> parseTerm(std::vector<Token>& tokens, size_t& pos);
    static std::unique_ptr<QueryNode> parseFactor(std::vector<Token>& tokens, size_t& pos);
    
    // Вычисление IDF для термина
    static double computeIDF(int docFreq, int totalDocs);
    
    // Вычисление TF для термина в документе (упрощённо - 1 если термин присутствует)
    static double computeTF(int tf, int docLength, double k1);
};

// Расширенный индекс
class RankedBooleanIndex {
public:
    RankedBooleanIndex();
    ~RankedBooleanIndex();
    
    void addDocument(int docId, const std::string& content);
    
    void buildIndex(const std::vector<Document>& documents);
    
    std::vector<QueryParser::BM25Score> search(const std::string& query);
    
    void save(const std::string& filename);
    
    void load(const std::string& filename);
    
    // Получение статистики
    size_t getIndexSize() const { return index.size(); }
    size_t getTotalDocuments() const { return docCount; }
    double getAvgDocLength() const { return avgDocLength; }
    
private:
    // Обратный индекс: термин -> posting list (docId, tf)
    MyHashMap<std::wstring, std::vector<Posting>> index;
    
    // Длины документов: docId -> длина в токенах
    MyHashMap<int, int> docLengths;
    
    double avgDocLength;
    
    size_t docCount;
    
    // Список всех существующих ID документов
    std::vector<int> allDocs;
    
    // Вспомогательные методы
    void processDocument(int docId, const std::string& content);
};

// Глобальные вспомогательные функции (используются в QueryNode)
inline std::vector<int> intersect(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    size_t i = 0, j = 0;
    
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            i++;
            j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }
    
    return result;
}

inline std::vector<int> unite(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    result.reserve(a.size() + b.size());
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

inline std::vector<int> complement(const std::vector<int>& a, const std::vector<int>& allDocs) {
    std::vector<int> result;
    result.reserve(allDocs.size() - a.size());
    
    size_t i = 0;
    for (int docId : allDocs) {
        if (i < a.size() && a[i] == docId) {
            i++;
        } else {
            result.push_back(docId);
        }
    }
    
    return result;
}

#endif // INDEXER_H
