#!/usr/bin/env python3


import xml.etree.ElementTree as ET
import time
import re
import json
import sys
from pathlib import Path
from urllib.parse import urlparse, unquote
import requests
from datetime import datetime
from bs4 import BeautifulSoup

BASE_URL = "https://starwars.fandom.com"
SITEMAP_INDEX_PATH = "siteinfo/indexes/sitemap-newsitemapxml-index.xml"
USER_AGENT = "ias_crawler"
OUTPUT_DIR = Path("pages")
LOG_FILE = Path("crawl_log.txt")
CHECKPOINT_FILE = Path("checkpoint.txt")
SLEEP_TIME = 1  # crawl-delay from robots.txt
TARGET_URLS = 50000


FOLLOW_LINKS = False  
MAX_DEPTH = 2  
MAX_URLS_PER_PAGE = 50

def load_robots_rules():
    robots_path = "siteinfo/robots.txt"
    try:
        with open(robots_path, 'r', encoding='utf-8') as f:
            text = f.read()

        rules = {'Disallow': [], 'Allow': []}
        current_agent = None

        for line in text.split('\n'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            if ':' in line:
                key, value = line.split(':', 1)
                key = key.strip()
                value = value.strip()

                if key.lower() == 'user-agent':
                    current_agent = value
                elif current_agent == '*':
                    if key in ['Disallow', 'Allow']:
                        rules[key].append(value)

        log_message(f"Loaded robots.txt: {len(rules['Disallow'])} Disallow, {len(rules['Allow'])} Allow")
        return rules
    except Exception as e:
        log_message(f"Could not load robots.txt: {e}")
        return {'Disallow': [], 'Allow': []}

def is_allowed(url, rules):
    path = urlparse(url).path

    for allow_pattern in rules['Allow']:
        pattern = allow_pattern.replace('*', '.*').replace('?', '.')
        if re.match(pattern, path):
            return True

    for disallow_pattern in rules['Disallow']:
        pattern = disallow_pattern.replace('*', '.*').replace('?', '.')
        if re.match(pattern, path):
            return False

    return True

def get_all_sitemap_paths():
    sitemaps = []
    try:
        tree = ET.parse(SITEMAP_INDEX_PATH)
        root = tree.getroot()
        ns = {'sm': 'http://www.sitemaps.org/schemas/sitemap/0.9'}

        for sm in root.findall('.//sm:sitemap', ns):
            loc = sm.find('sm:loc', ns).text
            if 'NS_0' in loc:
                filename = loc.split('/')[-1]
                local_path = f"siteinfo/{filename}"
                if Path(local_path).exists():
                    sitemaps.append(local_path)
                else:
                    sitemaps.append(loc)

        log_message(f"Found {len(sitemaps)} sitemap files with NS_0")
        return sitemaps
    except Exception as e:
        log_message(f"Error parsing sitemap-index: {e}")
        return []

def extract_urls_from_sitemap(sitemap_paths, target_count=50000):
    all_urls = []

    for sitemap in sitemap_paths[:10]:
        try:
            log_message(f"Processing sitemap: {sitemap}")

            if str(sitemap).startswith('http'):
                resp = requests.get(sitemap, timeout=30)
                resp.raise_for_status()
                content = resp.content
            else:
                with open(sitemap, 'rb') as f:
                    content = f.read()

            root = ET.fromstring(content)
            ns = {'sm': 'http://www.sitemaps.org/schemas/sitemap/0.9'}

            for url_elem in root.findall('.//sm:url', ns):
                loc = url_elem.find('sm:loc', ns).text
                lastmod = url_elem.find('sm:lastmod', ns)
                priority = url_elem.find('sm:priority', ns)

                all_urls.append({
                    'url': loc,
                    'lastmod': lastmod.text if lastmod is not None else None,
                    'priority': float(priority.text) if priority is not None else 0.0
                })

                if len(all_urls) >= target_count:
                    break

            log_message(f"  Extracted {len(all_urls)} URLs so far")

            if len(all_urls) >= target_count:
                break

        except Exception as e:
            log_message(f"Error parsing sitemap {sitemap}: {e}")
            continue

    log_message(f"Total URLs extracted: {len(all_urls)}")
    return all_urls[:target_count]

def fetch_page_via_api(url):
    title = urlparse(url).path.split('/')[-1]
    title = unquote(title)

    api_url = f"{BASE_URL}/api.php"
    params = {
        'action': 'query',
        'titles': title,
        'prop': 'revisions',
        'rvprop': 'content',
        'format': 'json',
        'redirects': 'true'
    }

    headers = {
        'User-Agent': USER_AGENT,
        'Accept': 'application/json',
    }

    try:
        log_message(f"Fetching: {title}", level='debug')
        time.sleep(SLEEP_TIME)

        resp = requests.get(api_url, params=params, headers=headers, timeout=30)
        resp.raise_for_status()

        data = resp.json()

        pages = data.get('query', {}).get('pages', {})
        if not pages:
            log_message(f"ERROR: No pages in API response for {url}")
            return None

        page_id = list(pages.keys())[0]
        page = pages[page_id]

        if page.get('pageid') == -1:
            log_message(f"ERROR: Page does not exist: {url}")
            return None

        revisions = page.get('revisions', [])
        if not revisions:
            log_message(f"ERROR: No revisions found for {url}")
            return None

        content = revisions[0].get('*', '')

        result = {
            'url': url,
            'title': page.get('title', ''),
            'pageid': page.get('pageid', 0),
            'content': content,
            'length': len(content)
        }

        return result
    except requests.exceptions.RequestException as e:
        log_message(f"Network error fetching {url}: {e}")
        return None
    except json.JSONDecodeError as e:
        log_message(f"JSON decode error for {url}: {e}")
        return None
    except Exception as e:
        log_message(f"Unexpected error fetching {url}: {e}")
        return None

def fetch_page_wikitext(url):
    title = unquote(title)

    api_url = f"{BASE_URL}/api.php"
    params = {
        'action': 'query',
        'titles': title,
        'prop': 'revisions',
        'rvprop': 'content',
        'format': 'json',
        'redirects': 'true'
    }

    headers = {
        'User-Agent': web-spider,
        'Accept': 'application/json',
    }

    try:
        log_message(f"Fetching wikitext: {title}", level='debug')
        time.sleep(SLEEP_TIME)

        resp = requests.get(api_url, params=params, headers=headers, timeout=30)
        resp.raise_for_status()

        data = resp.json()
        pages = data.get('query', {}).get('pages', {})
        if not pages:
            log_message(f"ERROR: No pages in API response for {url}")
            return None

        page_id = list(pages.keys())[0]
        page = pages[page_id]

        if page.get('pageid') == -1:
            log_message(f"ERROR: Page does not exist: {url}")
            return None

        revisions = page.get('revisions', [])
        if not revisions:
            log_message(f"ERROR: No revisions found for {url}")
            return None

        content = revisions[0].get('*', '')
        return content
    except requests.exceptions.RequestException as e:
        log_message(f"Network error fetching wikitext {url}: {e}")
        return None
    except json.JSONDecodeError as e:
        log_message(f"JSON decode error for {url}: {e}")
        return None
    except Exception as e:
        log_message(f"Unexpected error fetching wikitext {url}: {e}")
        return None

def extract_links_from_html(html, base_url, rules, current_depth=0):
    links = []
    try:
        soup = BeautifulSoup(html, 'html.parser')
        
        for a in soup.find_all('a', href=True):
            href = a['href']
            
            if not href.startswith('/wiki/'):
                continue
            if ':' in href.split('/wiki/')[1]:  # Special:, User:, File: и т.д.
                continue
            if href.startswith('/wiki/Special:') or href.startswith('/wiki/User:'):
                continue
            if '#' in href:
                continue
            
            # Собираем полный URL
            full_url = base_url.rstrip('/') + href
            
            # Проверяем robots.txt
            if not is_allowed(full_url, rules):
                continue
            
            # Избегаем дубликатов
            if full_url in links:
                continue
            
            links.append({
                'url': full_url,
                'lastmod': None,  # неизвестно
                'priority': 0.0,  # дефолтный приоритет
                'depth': current_depth + 1
            })
            
            if len(links) >= MAX_URLS_PER_PAGE:
                break
                
    except Exception as e:
        log_message(f"Error extracting links: {e}", level='debug')
    
    return links

def load_checkpoint():
    if CHECKPOINT_FILE.exists():
        try:
            with open(str(CHECKPOINT_FILE), 'r', encoding='utf-8') as f:
                processed = set(line.strip() for line in f if line.strip())
            log_message(f"Loaded checkpoint: {len(processed)} URLs already processed")
            return processed
        except Exception as e:
            log_message(f"Error loading checkpoint: {e}")
    return set()

def save_checkpoint(processed_urls):
    try:
        with open(str(CHECKPOINT_FILE), 'w', encoding='utf-8') as f:
            for url in sorted(processed_urls):
                f.write(url + '\n')
    except Exception as e:
        log_message(f"Error saving checkpoint: {e}")

def save_page(page_data):
    try:
        pageid = page_data['pageid']
        output_path = OUTPUT_DIR / f"{pageid}.json"
        with open(str(output_path), 'w', encoding='utf-8') as f:
            json.dump(page_data, f, indent=2, ensure_ascii=False)
        return True
    except Exception as e:
        log_message(f"Error saving page {page_data.get('pageid')}: {e}")
        return False

def log_message(message, level='info'):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_entry = f"[{timestamp}] {message}"

    print(log_entry)

    try:
        with open(str(LOG_FILE), 'a', encoding='utf-8') as f:
            f.write(log_entry + '\n')
    except Exception as e:
        print(f"Error writing to log: {e}")

def main():
    log_message("=" * 60)
    log_message("CRAWLER START: Massive crawl of starwars.fandom.com")

    OUTPUT_DIR.mkdir(exist_ok=True)

    log_message("\n1. Loading robots.txt...")
    rules = load_robots_rules()

    log_message("\n2. Getting sitemap list...")
    global sitemap_paths
    sitemap_paths = get_all_sitemap_paths()
    if not sitemap_paths:
        log_message("ERROR: No sitemaps found!")
        return

    log_message("\n3. Loading checkpoint...")
    processed_urls = load_checkpoint()
    log_message(f"Already processed: {len(processed_urls)} URLs")

    if FOLLOW_LINKS:
        # BFS queue: list of (url, lastmod, priority, depth)
        # Start with a single seed page
        url_queue = [("https://starwars.fandom.com/wiki/Star_Wars", None, 0.0, 0)]
    else:
        # Sitemap mode: extract from sitemaps
        log_message("\n4. Extracting URLs from sitemaps...")
        all_urls = extract_urls_from_sitemap(sitemap_paths, TARGET_URLS * 2)  # Извлекаем с запасом
        if not all_urls:
            log_message("ERROR: No URLs extracted!")
            return
        
        # Filter by robots.txt
        log_message("\n5. Filtering URLs by robots.txt...")
        filtered_urls = []
        for item in all_urls:
            if is_allowed(item['url'], rules):
                filtered_urls.append(item)
            else:
                log_message(f"  Disallowed: {item['url']}", level='debug')
        log_message(f"URLs after filtering: {len(filtered_urls)}/{len(all_urls)}")
        
        # Convert to queue format (depth=0)
        url_queue = [(item['url'], item.get('lastmod'), item.get('priority', 0.0), 0)
                     for item in filtered_urls
                     if item['url'] not in processed_urls]
    
    log_message(f"URL queue size: {len(url_queue)}")
    log_message(f"Processed URLs: {len(processed_urls)}")
    log_message(f"Total target: {TARGET_URLS} (already done + to do)")

    if not url_queue and not processed_urls:
        log_message("ERROR: No URLs to process!")
        return

    log_message("\n6. Starting crawl...")
    log_message(f"Checkpoint: {CHECKPOINT_FILE}")
    log_message(f"Output dir: {OUTPUT_DIR}")
    log_message(f"Log: {LOG_FILE}")
    log_message("-" * 60)

    start_time = time.time()
    success_count = 0
    error_count = 0
    total_processed = 0

    try:
        while url_queue and len(processed_urls) < TARGET_URLS:
            url, lastmod, priority, depth = url_queue.pop(0)
            
            if url in processed_urls:
                continue
            
            total_processed += 1
            log_message(f"[{len(processed_urls)+1}/{TARGET_URLS}] Processing (depth={depth}): {url}")

            # Fetch page based on mode
            if FOLLOW_LINKS:
                # Link following mode: fetch wikitext via API
                content = fetch_page_wikitext(url)
                if content:
                    # Generate pageid from URL (since API may not return it reliably)
                    pageid = abs(hash(url)) % (10**10)
                    # Extract title from URL
                    title = url.split('/')[-1].replace('_', ' ')
                    
                    page_data = {
                        'url': url,
                        'title': title,
                        'pageid': pageid,
                        'content': content,
                        'length': len(content)
                    }
                else:
                    page_data = None
            else:
                # Sitemap mode: fetch via API
                page_data = fetch_page_via_api(url)

            if page_data:
                # Add metadata
                page_data['lastmod'] = lastmod
                page_data['priority'] = priority
                page_data['depth'] = depth

                if save_page(page_data):
                    success_count += 1
                    processed_urls.add(url)
                    
                    if FOLLOW_LINKS and depth < MAX_DEPTH:
                        new_links = extract_links_from_html(page_data['content'], BASE_URL, rules, depth)
                        log_message(f"  Extracted {len(new_links)} links from page", level='debug')
                        
                        # Add new links to queue (if not processed and not already in queue)
                        for link in new_links:
                            link_url = link['url']
                            if link_url not in processed_urls and not any(item[0] == link_url for item in url_queue):
                                url_queue.append((link_url, link['lastmod'], link['priority'], link['depth']))
                    
                    if success_count % 10 == 0:
                        save_checkpoint(processed_urls)
                        log_message(f"  Checkpoint saved ({len(processed_urls)} pages)")
                else:
                    error_count += 1
            else:
                error_count += 1

            if total_processed % 100 == 0:
                elapsed = time.time() - start_time
                avg_time = elapsed / total_processed if total_processed else 0
                log_message(f"Progress: {len(processed_urls)} saved, queue={len(url_queue)}, {success_count} success, {error_count} errors")
                log_message(f"Elapsed: {elapsed:.1f}s, avg: {avg_time:.2f}s/page")

            if not FOLLOW_LINKS and not url_queue and len(processed_urls) < TARGET_URLS:
                log_message("Sitemap exhausted, extracting more URLs...")
                more_urls = extract_urls_from_sitemap(sitemap_paths, TARGET_URLS * 2)
                for item in more_urls:
                    if item['url'] not in processed_urls and is_allowed(item['url'], rules):
                        url_queue.append((item['url'], item.get('lastmod'), item.get('priority', 0.0), 0))
                if not url_queue:
                    log_message("No more URLs available in sitemaps.")
                    break

    except KeyboardInterrupt:
        log_message("\n\nINTERRUPTED: Saving checkpoint...")
        save_checkpoint(processed_urls)
        log_message("Checkpoint saved. You can resume later.")
        sys.exit(0)
    except Exception as e:
        log_message(f"\nUnexpected error: {e}")
        save_checkpoint(processed_urls)
        raise

    save_checkpoint(processed_urls)

    total_time = time.time() - start_time
    avg_time = total_time / success_count if success_count else 0

    log_message("\n" + "=" * 60)
    log_message("CRAWL COMPLETE")
    log_message("=" * 60)
    log_message(f"Total processed: {total_processed} requests")
    log_message(f"Success: {success_count}")
    log_message(f"Errors: {error_count}")
    log_message(f"Total time: {total_time:.1f}s ({total_time/60:.1f} minutes)")
    log_message(f"Average time per page: {avg_time:.2f}s")
    log_message(f"Pages saved in: {OUTPUT_DIR}")
    log_message(f"Final queue size: {len(url_queue)}")
    log_message("=" * 60)

if __name__ == "__main__":
    main()
