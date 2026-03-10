#include "tokenizer.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cwctype>

std::wstring_convert<std::codecvt_utf8<wchar_t>> Tokenizer::utf8_converter;

// Инициализация статического набора стоп-слов
const MyHashSet<std::wstring> Tokenizer::stopWords = {
    L"the", L"a", L"an", L"of", L"to", L"in", L"for", L"on", L"with", L"at",
    L"by", L"from", L"up", L"about", L"into", L"through", L"during", L"before",
    L"after", L"above", L"below", L"between", L"under", L"again", L"further",
    L"then", L"once", L"here", L"there", L"when", L"where", L"why", L"how",
    L"all", L"each", L"few", L"more", L"most", L"other", L"some", L"such",
    L"no", L"nor", L"not", L"only", L"own", L"same", L"so", L"than", L"too",
    L"very", L"just", L"can", L"will", L"don", L"should", L"now", L"and",
    L"or", L"but", L"if", L"because", L"as", L"until", L"while", L"of", L"at",
    L"by", L"for", L"with", L"about", L"against", L"between", L"into",
    L"through", L"during", L"before", L"after", L"above", L"below", L"to",
    L"from", L"up", L"down", L"in", L"out", L"on", L"off", L"over", L"under",
    L"again", L"further", L"then", L"once", L"here", L"there", L"when",
    L"where", L"why", L"how", L"all", L"any", L"both", L"each", L"few",
    L"more", L"most", L"other", L"some", L"such", L"no", L"nor", L"not",
    L"only", L"own", L"same", L"so", L"than", L"too", L"very", L"will",
    L"just", L"don", L"should", L"now", L"am", L"is", L"are", L"was", L"were",
    L"be", L"been", L"being", L"have", L"has", L"had", L"having", L"do",
    L"does", L"did", L"doing", L"would", L"could", L"should", L"might",
    L"must", L"shall", L"may", L"might", L"must", L"shall", L"can", L"could",
    L"may", L"might", L"must", L"shall", L"will", L"would", L"should", L"ought",
    L"need", L"dare", L"let's", L"that's", L"who's", L"what's", L"where's",
    L"when's", L"why's", L"how's", L"i'm", L"you're", L"he's", L"she's",
    L"it's", L"we're", L"they're", L"i've", L"you've", L"we've", L"they've",
    L"i'd", L"you'd", L"he'd", L"she'd", L"we'd", L"they'd", L"i'll",
    L"you'll", L"he'll", L"she'll", L"we'll", L"they'll", L"isn't", L"aren't",
    L"wasn't", L"weren't", L"hasn't", L"haven't", L"hadn't", L"doesn't",
    L"don't", L"didn't", L"won't", L"wouldn't", L"shouldn't", L"can't",
    L"couldn't", L"mustn't", L"mightn't", L"needn't", L"oughtn't", L"daren't"
};

std::vector<std::wstring> Tokenizer::tokenize(const std::wstring& text) {
    std::vector<std::wstring> tokens;
    std::wstring token;

    for (wchar_t ch : text) {
        if (std::iswalnum(ch)) {
            token += ch;
        } else {
            if (!token.empty()) {
                if (isValidToken(token)) {
                    std::wstring normalized = normalize(token);
                    tokens.push_back(normalized);
                }
                token.clear();
            }
        }
    }

    if (!token.empty() && isValidToken(token)) {
        std::wstring normalized = normalize(token);
        tokens.push_back(normalized);
    }

    return filterStopWords(tokens);
}

std::vector<std::wstring> Tokenizer::tokenizeUTF8(const std::string& text) {
    try {
        std::wstring wtext = utf8_converter.from_bytes(text);
        return tokenize(wtext);
    } catch (...) {
        return {};
    }
}

std::wstring Tokenizer::normalize(const std::wstring& token) {
    std::wstring result;
    for (wchar_t ch : token) {
        result += std::towlower(ch);
    }
    return result;
}

bool Tokenizer::isValidToken(const std::wstring& token) {
    if (token.length() < 2) return false;
    bool allDigits = true;
    for (wchar_t ch : token) {
        if (!std::iswdigit(ch)) {
            allDigits = false;
            break;
        }
    }
    return !allDigits;
}

std::vector<std::wstring> Tokenizer::filterStopWords(const std::vector<std::wstring>& tokens) {
    std::vector<std::wstring> filtered;
    filtered.reserve(tokens.size());
    
    for (const auto& token : tokens) {
        if (stopWords.find(token) == stopWords.end()) {
            filtered.push_back(token);
        }
    }
    
    return filtered;
}

const MyHashSet<std::wstring>& Tokenizer::getStopWords() {
    return stopWords;
}
