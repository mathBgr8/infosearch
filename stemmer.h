#ifndef STEMMER_H
#define STEMMER_H

#include <string>
#include <vector>
#include <locale>
#include <cctype>
#include "my_hashset.h"

class Stemmer {
public:
    static std::wstring stem(const std::wstring& word);

    static bool isRussian(const std::wstring& word);
    static bool isEnglish(const std::wstring& word);

    static std::wstring porterStem(const std::wstring& word);
    
    static std::wstring russianStem(const std::wstring& word);

private:
    // Вспомогательные функции для Porter Stemmer
    static bool isConsonant(const std::wstring& word, int pos);
    static bool isVowel(const std::wstring& word, int pos);
    static bool hasVowel(const std::wstring& word, int start, int end);
    static bool endsWith(const std::wstring& word, const std::wstring& suffix);
    static std::wstring replaceSuffix(std::wstring word, const std::wstring& suffix, const std::wstring& replacement);
    static std::wstring removeFirstLetterIfVowel(std::wstring word);
    
    // Porter Stemmer шаги
    static void step1a(std::wstring& word);
    static void step1b(std::wstring& word);
    static void step1c(std::wstring& word);
    static void step2(std::wstring& word);
    static void step3(std::wstring& word);
    static void step4(std::wstring& word);
    static void step5a(std::wstring& word);
    static void step5b(std::wstring& word);
    
    // Проверка меры (m) - количество последовательностей согласная-гласная-согласная
    static int measure(const std::wstring& word, int start, int end);
    static bool containsVowel(const std::wstring& word, int start, int end);
    
};

#endif // STEMMER_H
