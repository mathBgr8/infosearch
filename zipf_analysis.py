#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Анализ закона Ципфа на коллекции документов
"""

import os
import json
import glob
import re
from collections import Counter
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def tokenize_text(text):
    """
    Токенизация текста: извлечение слов (русские/английские)
    """
    # Приведение к нижнему регистру и извлечение слов
    words = re.findall(r'\b[a-zA-Zа-яА-ЯёЁ]{2,}\b', text.lower())
    return words

def simple_stem(word):
    """
    Простой стемминг (упрощенный)
    """
    if len(word) <= 3:
        return word

    # Английские окончания
    if word.endswith('ing'):
        return word[:-3]
    elif word.endswith('ed'):
        return word[:-2]
    elif word.endswith('ly'):
        return word[:-2]
    elif word.endswith('s') and len(word) > 3:
        return word[:-1]

    # Русские окончания
    if word.endswith('ть') or word.endswith('ться'):
        return word[:-3] if word.endswith('ться') else word[:-2]
    elif word.endswith('ет') or word.endswith('ут') or word.endswith('ют'):
        return word[:-2]
    elif word.endswith('а') or word.endswith('я') or word.endswith('о') or word.endswith('е'):
        return word[:-1]
    elif word.endswith('ых') or word.endswith('их') or word.endswith('ов') or word.endswith('ев'):
        return word[:-2]

    return word

def analyze_zipf(json_dir, sample_size=None):
    """
    Анализ распределения частот слов и проверка закона Ципфа
    """
    print("Загрузка документов и анализ частот...")

    json_files = sorted(glob.glob(os.path.join(json_dir, "*.json")))

    if sample_size:
        json_files = json_files[:sample_size]

    all_words = []
    doc_count = 0

    for json_file in json_files:
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                doc = json.load(f)
                content = doc.get('content', '')

                words = tokenize_text(content)
                all_words.extend(words)
                doc_count += 1

                if doc_count % 1000 == 0:
                    print(f"Обработано {doc_count} документов, слов: {len(all_words)}")

        except Exception as e:
            print(f"Ошибка при обработке {json_file}: {e}")

    print(f"\nВсего документов: {doc_count}")
    print(f"Всего слов (токенов): {len(all_words):,}")

    # Подсчет частот
    word_freq = Counter(all_words)

    print(f"Уникальных слов (до стемминга): {len(word_freq):,}")

    # Применяем стемминг
    stemmed_words = [simple_stem(word) for word in all_words]
    stemmed_freq = Counter(stemmed_words)

    print(f"Уникальных слов (после стемминга): {len(stemmed_freq):,}")

    # Получаем отсортированный список по убыванию частоты
    sorted_freq = sorted(stemmed_freq.items(), key=lambda x: x[1], reverse=True)

    # Создаем данные для графика Ципфа
    ranks = np.arange(1, len(sorted_freq) + 1)
    frequencies = np.array([freq for word, freq in sorted_freq])

    # Логарифмируем для проверки линейности в log-log масштабе
    log_ranks = np.log10(ranks)
    log_freq = np.log10(frequencies)

    # Линейная регрессия для оценки коэффициента Ципфа
    coeffs = np.polyfit(log_ranks[:min(1000, len(log_ranks))], log_freq[:min(1000, len(log_freq))], 1)
    slope = coeffs[0]
    intercept = coeffs[1]

    print("\n=== ЗАКОН ЦИПФА ===")
    print(f"Коэффициент Ципфа (s): {slope:.4f}")
    print(f"Теоретически ожидается около -1.0 для естественных языков")
    print(f"R² (коэффициент детерминации) можно вычислить отдельно")

    # Топ-20 слов
    print("\nТоп-20 слов по частоте:")
    for i, (word, freq) in enumerate(sorted_freq[:20], 1):
        print(f"{i:2}. {word}: {freq:,}")

    # Построение графика
    plt.figure(figsize=(12, 5))

    # График 1: log-log
    plt.subplot(1, 2, 1)
    plt.scatter(log_ranks[:1000], log_freq[:1000], alpha=0.5, s=10)
    plt.plot(log_ranks[:1000], slope * log_ranks[:1000] + intercept,
             'r-', linewidth=2, label=f'log(f) = {slope:.3f}·log(r) + {intercept:.3f}')
    plt.xlabel('log(ранг)')
    plt.ylabel('log(частота)')
    plt.title('Закон Ципфа (log-log масштаб)')
    plt.legend()
    plt.grid(True, alpha=0.3)

    # График 2: Топ-50 слов
    plt.subplot(1, 2, 2)
    top_n = 50
    top_words = [word for word, freq in sorted_freq[:top_n]]
    top_freqs = [freq for word, freq in sorted_freq[:top_n]]
    y_pos = np.arange(top_n)
    plt.barh(y_pos, top_freqs)
    plt.yticks(y_pos, top_words, fontsize=8)
    plt.xlabel('Частота')
    plt.title(f'Топ-{top_n} слов')
    plt.gca().invert_yaxis()

    plt.tight_layout()
    plt.savefig('zipf_analysis.png', dpi=150, bbox_inches='tight')
    print("\nГрафик сохранен в: zipf_analysis.png")
    plt.show()

    # Дополнительная статистика
    total_occurrences = sum(freq for word, freq in sorted_freq)
    print(f"\nОбщее количество вхождений слов: {total_occurrences:,}")

    # Проверка на соответствие закону Ципфа (H1: slope ≈ -1)
    print(f"\nВывод:")
    if abs(slope + 1) < 0.2:
        print("Закон Ципфа выполняется (коэффициент близок к -1)")
    else:
        print("Отклонение от закона Ципфа (коэффициент = {:.3f})".format(slope))

    return {
        'doc_count': doc_count,
        'total_words': len(all_words),
        'unique_words_before': len(word_freq),
        'unique_words_after': len(stemmed_freq),
        'zipf_coefficient': slope,
        'top_words': sorted_freq[:100]
    }

if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Использование: python zipf_analysis.py <папка_с_json> [sample_size]")
        print("Пример: python zipf_analysis.py pages/ 10000")
        sys.exit(1)

    json_dir = sys.argv[1]
    sample_size = int(sys.argv[2]) if len(sys.argv) > 2 else None

    if not os.path.exists(json_dir):
        print(f"Папка не найдена: {json_dir}")
        sys.exit(1)

    stats = analyze_zipf(json_dir, sample_size)

    # Сохраняем статистику в JSON
    with open('zipf_stats.json', 'w', encoding='utf-8') as f:
        json.dump(stats, f, ensure_ascii=False, indent=2)
    print("\nСтатистика сохранена в: zipf_stats.json")
