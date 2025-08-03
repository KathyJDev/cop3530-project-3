import os
import re
from bs4 import BeautifulSoup

# Conditional imports for external libraries
try:
    import PyPDF2
except ImportError:
    PyPDF2 = None
try:
    import docx
except ImportError:
    docx = None
try:
    import ebooklib
    from ebooklib import epub
except ImportError:
    epub = None

def convert_all_non_txt(folder):
    """
    Scans the specified folder for supported non-text document types (.pdf, .docx, .epub, .html)
    and converts their content into plain .txt files in the same directory.
    """
    for fname in os.listdir(folder):
        full_path = os.path.join(folder, fname)
        txt = None
        if fname.lower().endswith(".txt"):
            continue # Skip already text files
        elif fname.lower().endswith(".pdf") and PyPDF2:
            txt = extract_pdf_text(full_path)
        elif fname.lower().endswith(".docx") and docx:
            txt = extract_docx_text(full_path)
        elif fname.lower().endswith(".epub") and epub:
            txt = extract_epub_text(full_path)
        elif fname.lower().endswith((".html", ".htm")):
            txt = extract_html_text(full_path)
        else:
            continue # Skip unsupported file types

        if txt:
            base_name = os.path.splitext(fname)[0]
            txt_path = os.path.join(folder, base_name + ".txt")
            try:
                with open(txt_path, "w", encoding="utf-8") as f:
                    f.write(txt)
            except IOError as e:
                print(f"Warning: Could not write converted text for '{fname}': {e}")


def extract_pdf_text(path):
    """Extracts text from a PDF file."""
    if not PyPDF2:
        return ""
    try:
        with open(path, "rb") as f:
            reader = PyPDF2.PdfReader(f)
            return "\n".join(page.extract_text() or "" for page in reader.pages)
    except Exception as e:
        print(f"Error extracting PDF text from '{os.path.basename(path)}': {e}")
        return ""

def extract_docx_text(path):
    """Extracts text from a DOCX file."""
    if not docx:
        return ""
    try:
        doc = docx.Document(path)
        return "\n".join(paragraph.text for paragraph in doc.paragraphs)
    except Exception as e:
        print(f"Error extracting DOCX text from '{os.path.basename(path)}': {e}")
        return ""

def extract_epub_text(path):
    """Extracts text from an EPUB file."""
    if not epub:
        return ""
    try:
        book = epub.read_epub(path)
        text_content = []
        for item in book.get_items():
            if item.get_type() == ebooklib.ITEM_DOCUMENT:
                # Use BeautifulSoup to parse HTML content from EPUB items
                soup = BeautifulSoup(item.get_content(), "html.parser")
                text_content.append(soup.get_text())
        return "\n".join(text_content)
    except Exception as e:
        print(f"Error extracting EPUB text from '{os.path.basename(path)}': {e}")
        return ""

def extract_html_text(path):
    """Extracts plain text from an HTML file."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            soup = BeautifulSoup(f, "html.parser")
            return soup.get_text()
    except Exception as e:
        print(f"Error extracting HTML text from '{os.path.basename(path)}': {e}")
        return ""