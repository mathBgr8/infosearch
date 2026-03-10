#include "stemmer.h"
#include <algorithm>
#include <cctype>


static const std::wstring vowels = L"aeiouy";
static const std::wstring doubleConsonants = L"bbddffggkkllmmnnpprrssttvvzz";

bool Stemmer::isVowel(const std::wstring& word, int pos) {
    wchar_t ch = std::towlower(word[pos]);
    return vowels.find(ch) != std::wstring::npos;
}

bool Stemmer::isConsonant(const std::wstring& word, int pos) {
    if (pos < 0 || pos >= static_cast<int>(word.length())) return false;
    wchar_t ch = std::towlower(word[pos]);
    return vowels.find(ch) == std::wstring::npos && ch >= L'a' && ch <= L'z';
}

// Проверка наличия гласной в диапазоне
bool Stemmer::hasVowel(const std::wstring& word, int start, int end) {
    for (int i = start; i <= end; i++) {
        if (isVowel(word, i)) return true;
    }
    return false;
}

bool Stemmer::endsWith(const std::wstring& word, const std::wstring& suffix) {
    if (word.length() < suffix.length()) return false;
    return word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::wstring Stemmer::replaceSuffix(std::wstring word, const std::wstring& suffix, const std::wstring& replacement) {
    if (endsWith(word, suffix)) {
        word.replace(word.length() - suffix.length(), suffix.length(), replacement);
    }
    return word;
}

std::wstring Stemmer::removeFirstLetterIfVowel(std::wstring word) {
    if (!word.empty() && isVowel(word, 0)) {
        word = word.substr(1);
    }
    return word;
}

// Вычисление меры (m) - количество последовательностей CVC
int Stemmer::measure(const std::wstring& word, int start, int end) {
    int m = 0;
    bool inVowelSeq = false;
    
    for (int i = start; i <= end; i++) {
        if (isConsonant(word, i)) {
            if (inVowelSeq) {
                inVowelSeq = false;
            }
        } else {
            if (!inVowelSeq) {
                inVowelSeq = true;
                if (i > start && isConsonant(word, i - 1)) {
                    m++;
                }
            }
        }
    }
    return m;
}

bool Stemmer::containsVowel(const std::wstring& word, int start, int end) {
    for (int i = start; i <= end; i++) {
        if (isVowel(word, i)) return true;
    }
    return false;
}

void Stemmer::step1a(std::wstring& word) {
    if (endsWith(word, L"sses")) {
        word = word.substr(0, word.length() - 2); // ss
    } else if (endsWith(word, L"ies")) {
        word = word.substr(0, word.length() - 3) + L"y";
    } else if (endsWith(word, L"ss")) {
        // оставляем как есть
    } else if (word.length() > 1 && endsWith(word, L"s")) {
        // Удаляем 's', если перед ним есть гласная+согласная (например, "dogs" -> "dog")
        if (word.length() >= 3 && hasVowel(word, 0, word.length() - 2)) {
            word = word.substr(0, word.length() - 1);
        }
    }
}

void Stemmer::step1b(std::wstring& word) {
    if (endsWith(word, L"eed")) {
        if (measure(word, 0, word.length() - 4) > 0) {
            word = word.substr(0, word.length() - 1); // eed -> ee
        }
    } else if (endsWith(word, L"ed")) {
        if (hasVowel(word, 0, word.length() - 3)) {
            word = word.substr(0, word.length() - 2);
            if (endsWith(word, L"at") || endsWith(word, L"bl") || endsWith(word, L"iz")) {
                word += L"e";
            } else if (word.length() > 1 && 
                       word[word.length() - 1] == word[word.length() - 2] &&
                       doubleConsonants.find(word[word.length() - 1]) != std::wstring::npos) {
                word = word.substr(0, word.length() - 1);
            } else if (word.length() == 1 || measure(word, 0, word.length() - 1) == 1) {
                if (!endsWith(word, L"a") && !endsWith(word, L"e") && !endsWith(word, L"i") &&
                    !endsWith(word, L"o") && !endsWith(word, L"u") && !endsWith(word, L"y")) {
                    word += L"e";
                }
            }
        }
    } else if (endsWith(word, L"ing")) {
        if (hasVowel(word, 0, word.length() - 4)) {
            word = word.substr(0, word.length() - 3);
            if (endsWith(word, L"at") || endsWith(word, L"bl") || endsWith(word, L"iz")) {
                word += L"e";
            } else if (word.length() > 1 && 
                       word[word.length() - 1] == word[word.length() - 2] &&
                       doubleConsonants.find(word[word.length() - 1]) != std::wstring::npos) {
                word = word.substr(0, word.length() - 1);
            } else if (word.length() == 1 || measure(word, 0, word.length() - 1) == 1) {
                if (!endsWith(word, L"a") && !endsWith(word, L"e") && !endsWith(word, L"i") &&
                    !endsWith(word, L"o") && !endsWith(word, L"u") && !endsWith(word, L"y")) {
                    word += L"e";
                }
            }
        }
    }
}

void Stemmer::step1c(std::wstring& word) {
    if (word.length() > 2 && endsWith(word, L"y")) {
        if (!endsWith(word, L"ay") && !endsWith(word, L"ey") && !endsWith(word, L"iy") &&
            !endsWith(word, L"oy") && !endsWith(word, L"uy")) {
            if (isConsonant(word, word.length() - 3)) {
                word = word.substr(0, word.length() - 1) + L"i";
            }
        }
    }
}

void Stemmer::step2(std::wstring& word) {
    // Список суффиксов для шага 2
    static const std::vector<std::pair<std::wstring, std::wstring>> suffixes = {
        {L"ational", L"ate"}, {L"tional", L"tion"}, {L"enci", L"ence"}, {L"anci", L"ance"},
        {L"izer", L"ize"}, {L"bli", L"ble"}, {L"alli", L"al"}, {L"entli", L"ent"},
        {L"eli", L"e"}, {L"ousli", L"ous"}, {L"ization", L"ize"}, {L"ation", L"ate"},
        {L"ator", L"ate"}, {L"alism", L"al"}, {L"iveness", L"ive"}, {L"fulness", L"ful"},
        {L"ousness", L"ous"}, {L"aliti", L"al"}, {L"iviti", L"ive"}, {L"biliti", L"ble"},
        {L"icate", L"ic"}, {L"iciti", L"ic"}, {L"ical", L"ic"}, {L"ful", L""},
        {L"ness", L""}, {L"ive", L""}, {L"ize", L""}, {L"al", L""}, {L"ance", L""},
        {L"ence", L""}, {L"er", L""}, {L"ic", L""}, {L"able", L""}, {L"ible", L""},
        {L"ant", L""}, {L"ement", L""}, {L"ment", L""}, {L"ent", L""}, {L"tion", L""},
        {L"ity", L""}, {L"ty", L""}, {L"ism", L""}, {L"ate", L""}, {L"iti", L""},
        {L"or", L""}, {L"ist", L""}, {L"ous", L""}, {L"ship", L""}, {L"age", L""},
        {L"ery", L""}
    };
    
    for (const auto& pair : suffixes) {
        if (endsWith(word, pair.first)) {
            int stemLen = word.length() - pair.first.length();
            if (measure(word, 0, stemLen - 1) > 0) {
                word = word.substr(0, stemLen) + pair.second;
                break;
            }
        }
    }
}

void Stemmer::step3(std::wstring& word) {
    static const std::vector<std::pair<std::wstring, std::wstring>> suffixes = {
        {L"icate", L"ic"}, {L"iciti", L"ic"}, {L"ical", L"ic"}, {L"tive", L""},
        {L"alize", L"al"}, {L"ize", L""}, {L"ative", L""}, {L"ative", L""},
        {L"al", L""}, {L"ance", L""}, {L"ence", L""}, {L"icate", L"ic"},
        {L"iciti", L"ic"}, {L"ical", L"ic"}, {L"ful", L""}, {L"ness", L""}
    };
    
    for (const auto& pair : suffixes) {
        if (endsWith(word, pair.first)) {
            int stemLen = word.length() - pair.first.length();
            if (measure(word, 0, stemLen - 1) > 0) {
                word = word.substr(0, stemLen) + pair.second;
                break;
            }
        }
    }
}

void Stemmer::step4(std::wstring& word) {
    static const std::vector<std::wstring> suffixes = {
        L"al", L"ance", L"ence", L"er", L"ic", L"able", L"ible", L"ant", L"ement",
        L"ment", L"ent", L"tion", L"ance", L"ence", L"ity", L"ty", L"ism", L"ate",
        L"iti", L"er", L"or", L"ism", L"ist", L"ous", L"ful", L"ness", L"less", L"ship", L"age", L"ery"
    };
    
    for (const auto& suffix : suffixes) {
        if (endsWith(word, suffix)) {
            int stemLen = word.length() - suffix.length();
            if (measure(word, 0, stemLen - 1) > 1) {
                word = word.substr(0, stemLen);
                return;
            }
        }
    }
    
    // Особые случаи: 'at', 'bl', 'iz'
    if (endsWith(word, L"at") || endsWith(word, L"bl") || endsWith(word, L"iz")) {
        word += L"e";
        return;
    }
    
    // Двойные согласные
    if (word.length() > 1) {
        wchar_t last = std::towlower(word[word.length() - 1]);
        if (word.length() > 2 && word[word.length() - 1] == word[word.length() - 2] &&
            doubleConsonants.find(last) != std::wstring::npos) {
            word = word.substr(0, word.length() - 1);
        }
    }
}

void Stemmer::step5a(std::wstring& word) {
    if (endsWith(word, L"e")) {
        int stemLen = word.length() - 1;
        int m = measure(word, 0, stemLen - 1);
        if (m > 1 || (m == 1 && !containsVowel(word, 0, stemLen - 1))) {
            word = word.substr(0, stemLen);
        }
    } else if (endsWith(word, L"le") && word.length() > 2) {
        if (isConsonant(word, word.length() - 3)) {
            // оставляем как есть
        } else {
            word = word.substr(0, word.length() - 1);
        }
    }
}

void Stemmer::step5b(std::wstring& word) {
    if (endsWith(word, L"e")) {
        int stemLen = word.length() - 1;
        int m = measure(word, 0, stemLen - 1);
        if (m > 1) {
            word = word.substr(0, stemLen);
        } else if (m == 1 && !containsVowel(word, 0, stemLen - 1)) {
            word = word.substr(0, stemLen);
        }
    }
}

std::wstring Stemmer::stem(const std::wstring& word) {
    if (word.length() <= 2) return word;
    
    std::wstring result = word;
    
    step1a(result);
    step1b(result);
    step1c(result);
    step2(result);
    step3(result);
    step4(result);
    step5a(result);
    step5b(result);
    
    return result.length() > 1 ? result : word;
}

// Определение языка слова (простая эвристика)
bool Stemmer::isRussian(const std::wstring& word) {
    // Проверка на кириллицу - возвращаем false, так как только английский
    return false;
}

bool Stemmer::isEnglish(const std::wstring& word) {
    for (wchar_t ch : word) {
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z')) {
            return true;
        }
    }
    return false;
}
