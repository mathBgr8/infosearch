#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Конвертация JSON документов в текстовый формат для C++ индексатора
"""

import json
import os
import glob
import re
from pathlib import Path

def clean_wikitext(text):
    if not text:
        return ""
    
    # Удаляем HTML/XML теги
    text = re.sub(r'<[^>]+>', '', text)
    
    # Удаляем вики-разметку [[...]] - оставляем только текст внутри
    # [[Ссылка|Текст]] -> Текст, [[Ссылка]] -> Ссылка
    def replace_wikilink(match):
        content = match.group(1)
        # Если есть вертикальная черта, берем часть после нее
        if '|' in content:
            return content.split('|', 1)[1]
        return content
    
    text = re.sub(r'\[\[([^\]|]+(?:\|[^\]]+)?)\]\]', replace_wikilink, text)
    
    # Удаляем шаблоны {{...}} (включая многострочные)
    text = re.sub(r'\{\{[^}]*\}\}', '', text, flags=re.DOTALL)
    
    # Удаляем комментарии <!-- ... -->
    text = re.sub(r'<!--.*?-->', '', text, flags=re.DOTALL)
    
    # Удаляем заголовки == Заголовок ==
    text = re.sub(r'==+.*?==+', '', text)
    
    # Удаляем маркированные списки (* и #)
    text = re.sub(r'^\s*[\*#]\s+', '', text, flags=re.MULTILINE)
    
    # Удаляем форматирование '' и '''
    text = re.sub(r"'''", '', text)
    text = re.sub(r"''", '', text)
    
    # Удаляем ссылки [http://...]
    text = re.sub(r'\[https?://[^\]]+\]', '', text)
    
    # Заменяем несколько пробелов на один
    text = re.sub(r'\s+', ' ', text)
    
    return text.strip()

def convert_json_to_txt(json_dir, output_dir):
    os.makedirs(output_dir, exist_ok=True)

    json_files = sorted(glob.glob(os.path.join(json_dir, "*.json")))

    for i, json_file in enumerate(json_files):
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                doc = json.load(f)

            pageid = doc.get('pageid', i)
            txt_file = os.path.join(output_dir, f"{pageid:06d}.txt")

            with open(txt_file, 'w', encoding='utf-8') as f:
                # Метаданные
                f.write(f"{pageid}\n")
                f.write(f"{doc.get('url', '')}\n")
                f.write(f"{doc.get('title', '')}\n")
                f.write(f"{doc.get('length', 0)}\n")
                f.write("\n")
                # Содержимое: очищаем вики-разметку
                content = doc.get('content', '')
                cleaned_content = clean_wikitext(content)
                f.write(cleaned_content)

            if (i + 1) % 1000 == 0:
                print(f"Конвертировано {i + 1} документов...")

        except Exception as e:
            print(f"Ошибка при обработке {json_file}: {e}")

    print(f"Готово! Сконвертировано {len(json_files)} документов в {output_dir}")

if __name__ == "__main__":
    import sys

    if len(sys.argv) < 3:
        print("Использование: python convert_json.py <папка_с_json> <выходная_папка>")
        sys.exit(1)

    json_dir = sys.argv[1]
    output_dir = sys.argv[2]

    convert_json_to_txt(json_dir, output_dir)
