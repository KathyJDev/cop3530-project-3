import requests
import re
import os
import io
import gzip
import tempfile
from bs4 import BeautifulSoup
import ebooklib
from ebooklib import epub # Still needed here for epub processing after download

# Conditional import for gutenbergpy
try:
    import gutenbergpy.textget as gutenberg_textget
except ImportError:
    gutenberg_textget = None

# API endpoint
GUTENDEX_API = "https://gutendex.com/books"

def search_gutenberg_books(query):
    """
    Searches the Gutendex API for books matching a given query.

    Args:
        query (str): The search query (book title or author).

    Returns:
        list: A list of book dictionaries from the API response, or an empty list if none found.
    Raises:
        requests.exceptions.RequestException: If there's an issue with the API request.
    """
    response = requests.get(GUTENDEX_API, params={'search': query})
    response.raise_for_status() # Raise an exception for HTTP errors
    data = response.json()
    return data.get('results', [])

def clean_gutenberg_text(raw_text):
    """
    Cleans raw text downloaded from Project Gutenberg by stripping headers/footers.
    Attempts to use gutenbergpy.textget.strip_headers if available and content looks like raw bytes,
    otherwise uses regex for more general cleaning of common Project Gutenberg boilerplate.
    """
    # Attempt to use gutenbergpy's stripping if it looks like raw bytes (often gzipped)
    if gutenberg_textget:
        # Check if raw_text is bytes, if not, encode it for strip_headers
        processed_bytes = raw_text.encode("utf-8", errors='ignore') if isinstance(raw_text, str) else raw_text
        # Check for gzip header magic numbers
        if len(processed_bytes) >= 2 and processed_bytes[0] == 0x1f and processed_bytes[1] == 0x8b:
            try:
                with gzip.GzipFile(fileobj=io.BytesIO(processed_bytes)) as gzfile:
                    processed_bytes = gzfile.read()
            except Exception:
                pass # Fallback to raw bytes if gzip fails

        cleaned_bytes = gutenberg_textget.strip_headers(processed_bytes)
        try:
            decoded_text = cleaned_bytes.decode("utf-8")
            # If UTF-8 decode is too short or seems wrong, try latin1
            if len(decoded_text.strip()) < 100 or '\ufffd' in decoded_text:
                decoded_text = cleaned_bytes.decode("latin1", errors="ignore")
            return decoded_text
        except UnicodeDecodeError:
            decoded_text = cleaned_bytes.decode("latin1", errors="ignore")
            return decoded_text
        except Exception:
            pass # Fallback to regex cleaning if decoding fails

    # Fallback to regex cleaning if gutenbergpy is not available or fails
    start_re = re.compile(r'\*\*\* *START OF (THE|THIS) PROJECT GUTENBERG EBOOK.*\*\*\*', re.IGNORECASE | re.DOTALL)
    end_re = re.compile(r'\*\*\* *END OF (THE|THIS) PROJECT GUTENBERG EBOOK.*\*\*\*', re.IGNORECASE | re.DOTALL)

    start_match = start_re.search(raw_text)
    end_match = end_re.search(raw_text)

    if start_match and end_match:
        return raw_text[start_match.end():end_match.start()].strip()
    elif start_match:
        return raw_text[start_match.end():].strip()
    elif end_match:
        return raw_text[:end_match.start()].strip()
    else:
        # Heuristic: if no markers, try to cut common preamble/postamble
        # This is a bit risky but can work for many Gutenberg texts
        if len(raw_text) > 1200:
            return raw_text[600:-600].strip()
        return raw_text.strip()


def download_gutenberg_content(book_info):
    """
    Attempts to download the plain text content for a given Gutenberg book.
    Prioritizes text, then HTML, then EPUB, trying various common URLs.

    Args:
        book_info (dict): A dictionary containing book details, including 'id' and 'formats'.

    Returns:
        tuple: (content_string, error_message).
               content_string will be None if download fails.
               error_message will be None if successful, otherwise a string.
    """
    book_id = book_info.get("id")
    formats = book_info.get("formats", {})
    content = None

    # 1. Try gutenbergpy.textget
    if gutenberg_textget and book_id:
        try:
            raw_bytes = gutenberg_textget.get_text_by_id(book_id)
            if raw_bytes:
                temp_content = clean_gutenberg_text(raw_bytes)
                if temp_content and len(temp_content.strip()) > 1000:
                    content = temp_content
        except Exception as e:
            print(f"Warning: gutenbergpy_download failed for ID {book_id}: {e}")
            pass # Continue to other methods

    # 2. Try direct text/plain URL from Gutendex API
    if content is None:
        text_url = formats.get("text/plain; charset=utf-8") or formats.get("text/plain")
        if text_url:
            try:
                response = requests.get(text_url)
                response.raise_for_status()
                temp_content = clean_gutenberg_text(response.text)
                if temp_content and len(temp_content.strip()) > 1000:
                    content = temp_content
            except Exception as e:
                print(f"Warning: Direct text URL download failed for {text_url}: {e}")
                pass

    # 3. Try alternative text/plain URLs based on book_id (common pattern)
    if content is None and book_id:
        for suffix in ["-0.txt", ".txt"]: # Common Gutenberg text file suffixes
            try:
                alt_url = f"https://www.gutenberg.org/files/{book_id}/{book_id}{suffix}"
                response = requests.get(alt_url)
                if response.status_code == 200:
                    temp_content = clean_gutenberg_text(response.text)
                    if temp_content and len(temp_content.strip()) > 1000:
                        content = temp_content
                        break
            except Exception as e:
                print(f"Warning: Alt text URL download failed for {alt_url}: {e}")
                continue

    # 4. Try direct HTML URL from Gutendex API
    if content is None:
        html_url = formats.get("text/html; charset=utf-8") or formats.get("text/html")
        if html_url:
            try:
                response = requests.get(html_url)
                response.raise_for_status()
                soup = BeautifulSoup(response.text, "html.parser")
                temp_content = soup.get_text()
                # HTML often has less boilerplate, so maybe a slightly lower threshold
                if temp_content and len(temp_content.strip()) > 500:
                    content = temp_content
            except Exception as e:
                print(f"Warning: HTML URL download failed for {html_url}: {e}")
                pass

    # 5. Try alternative HTML URLs (common pattern)
    if content is None and book_id:
        html_patterns = [
            f"https://www.gutenberg.org/files/{book_id}/{book_id}-h/{book_id}-h.htm",
            f"https://www.gutenberg.org/files/{book_id}/{book_id}-h/{book_id}-h.html",
            f"https://www.gutenberg.org/files/{book_id}/{book_id}.htm",
            f"https://www.gutenberg.org/files/{book_id}/{book_id}.html",
        ]
        for html_url_canonical in html_patterns:
            try:
                response = requests.get(html_url_canonical)
                if response.status_code == 200:
                    soup = BeautifulSoup(response.text, "html.parser")
                    temp_content = soup.get_text()
                    if temp_content and len(temp_content.strip()) > 500:
                        content = temp_content
                        break
            except Exception as e:
                print(f"Warning: Alt HTML URL download failed for {html_url_canonical}: {e}")
                continue

    # 6. Try EPUB (requires ebooklib and temporary file handling)
    if content is None:
        epub_url = formats.get("application/epub+zip")
        if epub_url:
            tmpf_name = None
            try:
                response = requests.get(epub_url)
                response.raise_for_status()
                with tempfile.NamedTemporaryFile(delete=False, suffix=".epub") as tmpf:
                    tmpf.write(response.content)
                    tmpf.flush()
                    tmpf_name = tmpf.name
                
                # Directly extract text from the temporary EPUB file
                # This part needs to be careful about not relying on self.extract_epub from GUI class
                extracted_text = ""
                if epub: # Check if ebooklib is actually available
                    book = epub.read_epub(tmpf.name)
                    text_parts = []
                    for item in book.get_items():
                        if item.get_type() == ebooklib.ITEM_DOCUMENT:
                            soup = BeautifulSoup(item.get_content(), "html.parser")
                            text_parts.append(soup.get_text())
                    extracted_text = "\n".join(text_parts)

                if extracted_text and len(extracted_text.strip()) > 1000:
                    content = extracted_text
            except Exception as e:
                print(f"Warning: EPUB download/extraction failed for {epub_url}: {e}")
                content = None # Ensure content is None if EPUB fails completely
            finally:
                if tmpf_name and os.path.exists(tmpf_name):
                    try:
                        os.remove(tmpf_name) # Clean up temp file
                    except OSError as remove_e:
                        print(f"Warning: Could not delete temporary EPUB file {tmpf_name}: {remove_e}")


    if not content or len(content.strip()) < 1000:
        return None, "Could not download a suitable plain text version for this book, or text was too short."

    return content, None

def save_book_to_file(book_info, content, save_dir):
    """
    Saves the downloaded book content to a .txt file in the specified directory.
    Also prepends the filename to the content for C++ backend.

    Args:
        book_info (dict): The dictionary containing book details (e.g., 'title', 'id').
        content (str): The plain text content of the book.
        save_dir (str): The directory where the book file should be saved.

    Returns:
        tuple: (full_path_to_saved_file, error_message).
               full_path_to_saved_file will be None if save fails.
               error_message will be None if successful, otherwise a string.
    """
    title = book_info.get("title", f"gutenberg_book_{book_info.get('id', 'unknown')}").replace(" ", "_")
    # Sanitize title for filename
    invalid_chars_re = re.compile(r'[<>:"/\\|?*\x00-\x1f]')
    title = invalid_chars_re.sub('', title)
    title = title[:60] # Limit length

    book_id = book_info.get("id")
    filename = f"{title}_{book_id}.txt" if book_id else f"{title}.txt"
    full_path = os.path.join(save_dir, filename)

    # Prepend filename to content for C++ backend
    final_content = f"Document: {filename}\n\n{content}"

    if not os.access(save_dir, os.W_OK):
        return None, f"Cannot write to directory '{save_dir}'. Please check permissions."

    try:
        with open(full_path, "w", encoding="utf-8") as f:
            f.write(final_content)
        return full_path, None
    except Exception as e:
        return None, f"Could not save book to '{full_path}': {e}"