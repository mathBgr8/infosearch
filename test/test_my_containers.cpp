#include <iostream>
#include "my_hashmap.h"
#include "my_hashset.h"

int main() {
    std::cout << "=== Testing MyHashMap ===" << std::endl;
    
    // Test MyHashMap<int, int>
    MyHashMap<int, int> intMap;
    intMap[1] = 100;
    intMap[2] = 200;
    intMap[3] = 300;
    
    std::cout << "intMap[1] = " << intMap[1] << std::endl;
    std::cout << "intMap[2] = " << intMap[2] << std::endl;
    std::cout << "intMap size: " << intMap.size() << std::endl;
    
    // Test find
    auto it = intMap.find(2);
    if (it != intMap.end()) {
        std::cout << "Found key 2, value: " << it->second << std::endl;
    }
    
    // Test erase
    intMap.erase(2);
    std::cout << "After erase(2), size: " << intMap.size() << std::endl;
    
    std::cout << "\n=== Testing MyHashSet ===" << std::endl;
    
    // Test MyHashSet<std::wstring>
    MyHashSet<std::wstring> stringSet;
    stringSet.insert(L"hello");
    stringSet.insert(L"world");
    stringSet.insert(L"test");
    
    std::wcout << L"Set contains L\"hello\": " << stringSet.contains(L"hello") << std::endl;
    std::wcout << L"Set contains L\"missing\": " << stringSet.contains(L"missing") << std::endl;
    std::wcout << L"Set size: " << stringSet.size() << std::endl;
    
    // Test iteration
    std::wcout << L"Elements: ";
    for (const auto& elem : stringSet) {
        std::wcout << elem << L" ";
    }
    std::wcout << std::endl;
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    
    return 0;
}
