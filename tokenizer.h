#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <regex>
#include "my_hashset.h"

class Tokenizer {
public:
    static std::vector<std::wstring> tokenize(const std::wstring& text);
    static std::vector<std::wstring> tokenizeUTF8(const std::string& text);

    static std::wstring normalize(const std::wstring& token);

    static bool isValidToken(const std::wstring& token);

    static std::vector<std::wstring> filterStopWords(const std::vector<std::wstring>& tokens);

    // Получение набора стоп-слов (для отладки/информации)
    static const MyHashSet<std::wstring>& getStopWords();

private:
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_converter;
    static std::locale ru_locale;
    static std::locale en_locale;
    static const MyHashSet<std::wstring> stopWords;
};

#endif // TOKENIZER_H
