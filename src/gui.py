import sys
import subprocess
import requests
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLineEdit, QPushButton,
    QLabel, QListWidget, QListWidgetItem, QMessageBox, QInputDialog, QTextEdit,
    QFileDialog, QComboBox, QDialog, QSizePolicy, QButtonGroup, QAction
)
from PyQt5.QtGui import QPixmap, QFont, QIcon
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QSize
import os
import re

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
    from bs4 import BeautifulSoup
except ImportError:
    epub = None

try:
    import gutenbergpy.textget as gutenberg_textget
except ImportError:
    gutenberg_textget = None

import gzip
import io

# from generate_summary import DocSummary

GUTENDEX_API = "https://gutendex.com/books"

class WorkerThread(QThread):
    finished = pyqtSignal(object)
    error = pyqtSignal(str)

    def __init__(self, command_args, is_add_file=False):
        super().__init__()
        self.command_args = command_args
        self.is_add_file = is_add_file

    def run(self):
        try:
            if self.is_add_file:
                result = subprocess.run(self.command_args, capture_output=True, text=True, encoding="utf-8", errors='replace')
            else:
                result = subprocess.run(self.command_args, capture_output=True, text=True, encoding="utf-8", errors='replace')

            self.finished.emit(result)
        except FileNotFoundError:
            self.error.emit(f"Error: search_engine executable not found. Make sure it's compiled and in the same directory.")
        except Exception as e:
            self.error.emit(f"An unexpected error occurred: {e}")

"""
class SummarizerWorker(QThread):
    finished = pyqtSignal(str)
    error = pyqtSignal(str)
    def __init__(self, text_to_summarize):
        super().__init__()
        self.text = text_to_summarize
    def run(self):
        try:
            words = self.text.split()
            reduced_content = " ".join(words[:1000])
            summarizer = DocSummary()
            summary_list = summarizer.summarize(reduced_content)
            self.finished.emit(summary_list[0]['summary_text'])

        except Exception as e:
            self.error.emit(f"Error Summary unable to generate:{e}")
"""

class SearchWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Simple Search Engine with Gutenberg")
        self.setFixedSize(750, 540)
        self.docs_indexed = False
        self.indexed_folder = "."
        self.last_query = ""
        self.doc_id_map = {}
        self.doc_map = {}
        self.init_ui()
        self.current_worker = None
       #self.summarizer_worker = None

    def init_ui(self):
        layout = QVBoxLayout()

        # Load logo image if available, otherwise display title text
        self.logo_label = QLabel()
        pixmap = QPixmap("logo.png")
        if pixmap.isNull():
            self.logo_label.setText("Search Engine")
            self.setStyleSheet("background-color: #202020; color: white;")
            self.logo_label.setFont(QFont("Arial", 24))
            self.logo_label.setStyleSheet("font-weight: 600;")
        else:
            self.logo_label.setPixmap(pixmap.scaled(200, 70, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        self.logo_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(self.logo_label)

        # Toggle Buttons
        btn_font = QFont("Arial", 10)
        toggle_layout = QHBoxLayout()
        self.inverted_btn = QPushButton("Inverted Index")
        self.suffix_btn = QPushButton("Suffix Array")
        self.inverted_btn.setFont(btn_font)
        self.suffix_btn.setFont(btn_font)
        self.inverted_btn.setCursor(Qt.PointingHandCursor)
        self.suffix_btn.setCursor(Qt.PointingHandCursor)

        for btn in (self.inverted_btn, self.suffix_btn):
            btn.setCheckable(True)
            btn.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
            btn.setStyleSheet("""
                QPushButton { 
                    padding: 6px 12px; 
                    border-radius: 4px;
                    background-color: #202020;
                    color: #8E8E8E;
                    font-weight: 500;
                } 
                QPushButton:checked { 
                    background-color: #414141;
                    color: white;
                }
                QPushButton:hover {
                    background-color: #414141;
                    color: white;
                }
            """)
        toggle_group = QButtonGroup(self)
        toggle_group.setExclusive(True)
        toggle_group.addButton(self.inverted_btn)
        toggle_group.addButton(self.suffix_btn)
        self.inverted_btn.setChecked(True)
        toggle_layout.addStretch()
        toggle_layout.addWidget(self.inverted_btn)
        toggle_layout.addWidget(self.suffix_btn)
        toggle_layout.addStretch()
        toggle_layout.setContentsMargins(0, 6, 0, 6)
        layout.addLayout(toggle_layout)

        # Search Input
        search_layout = QHBoxLayout()
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("type keywords or phrases...")
        self.search_input.setFont(QFont("Inter", 11))
        self.search_input.setMinimumHeight(32)
        self.search_input.setStyleSheet("""
            QLineEdit {
                padding: 8px 24px;
                border-radius: 16px;
                background-color: #D9D9D9;
                color: #202020;
                font-weight: 400;
            }
        """)
        self.search_icon = QPushButton()
        self.search_icon.setIcon(QIcon("../resources/images/search.svg"))
        self.search_icon.setIconSize(QSize(16, 16))
        self.search_icon.setFixedSize(40, 40)
        self.search_icon.setStyleSheet("""
            QPushButton {
                background-color: #414141;
                border-radius: 20px;
            }
        """)
        self.search_icon.setCursor(Qt.PointingHandCursor)
        search_layout.addWidget(self.search_input)
        search_layout.addWidget(self.search_icon)
        search_layout.setContentsMargins(48, 0, 48, 0)
        layout.addLayout(search_layout)

        # btn_layout = QHBoxLayout()
        # self.search_btn = QPushButton("Search")
        # # --- REMOVED: self.lucky_btn = QPushButton("First Matching Document") ---
        # btn_layout.addStretch() # Add stretch to the left
        # btn_layout.addWidget(self.search_btn)
        # btn_layout.addStretch() # Add stretch to the right
        # layout.addLayout(btn_layout)

        # ds_layout = QHBoxLayout()
        # ds_label = QLabel("Search Engine:")
        # self.ds_combo = QComboBox()
        # self.ds_combo.addItems(["Inverted Index", "Suffix Array"])
        # ds_layout.addWidget(ds_label)
        # ds_layout.addWidget(self.ds_combo)
        # ds_layout.addStretch()
        # layout.addLayout(ds_layout)


        # Document Indexing and Online Search Buttons
        ds_layout = QVBoxLayout()
        self.index_btn = QPushButton("Index Local Documents")
        self.online_btn = QPushButton("Search Online (Gutenberg)")
        self.index_btn.setFont(QFont("Arial", 10))
        self.online_btn.setFont(QFont("Arial", 10))
        for btn in (self.index_btn, self.online_btn):
            btn.setStyleSheet("""
                QPushButton { 
                    padding: 6px; 
                    border-radius: 4px; 
                    background-color: #202020; 
                    color: white;
                    font-weight: 500;
                } 
                QPushButton:hover { 
                    background-color: #414141;
                }
            """)
        btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        ds_layout.addWidget(self.index_btn)
        ds_layout.addWidget(self.online_btn)
        layout.addLayout(ds_layout)

        # Document Results Box
        self.results = QListWidget()
        self.results.setMinimumHeight(150)
        self.results.setFont(QFont("Arial", 11))
        self.results.setStyleSheet("""
            QListWidget {
                border-radius: 8px; 
                color: black;
                background-color: #D9D9D9;
            }
            """)
        layout.addWidget(self.results)

        self.setLayout(layout)

        self.index_btn.clicked.connect(self.index_documents)
        self.search_icon.clicked.connect(self.search)
        # --- REMOVED: self.lucky_btn.clicked.connect(self.feeling_lucky) ---
        self.online_btn.clicked.connect(self.search_online)
        self.results.itemDoubleClicked.connect(self.show_document)

        self.setStyleSheet("""
            QWidget { background: white; }
            QLineEdit { font-size: 18px; padding: 8px; border-radius: 16px; border: 1px solid #ddd; }
            QPushButton { font-size: 16px; padding: 8px 16px; border-radius: 10px; }
            QPushButton:hover { background: #f8f8f8; }
        """)

    def get_ds_flag(self):
        return "--ds", "suffix" if self.suffix_btn.isChecked() else "inverted"

    def index_documents(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Documents Directory")
        if not folder:
            return
        self.indexed_folder = folder

        QApplication.setOverrideCursor(Qt.WaitCursor)

        self.convert_all_non_txt(folder)

        self.current_worker = WorkerThread(['./search_engine', '--index', folder])
        self.current_worker.finished.connect(self.on_indexing_finished)
        self.current_worker.error.connect(self.on_indexing_error)
        self.current_worker.start()

    def on_indexing_finished(self, result):
        QApplication.restoreOverrideCursor()
        if result.returncode == 0:
            self.docs_indexed = True
            QMessageBox.information(self, "Indexing Complete", "Documents indexed successfully!")
        else:
            QMessageBox.warning(self, "Indexing Failed", result.stderr or "Failed to index documents.")
        self.current_worker = None

    def on_indexing_error(self, error_message):
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None

    def convert_all_non_txt(self, folder):
        for fname in os.listdir(folder):
            full_path = os.path.join(folder, fname)
            if fname.lower().endswith(".txt"):
                continue
            elif fname.lower().endswith(".pdf") and PyPDF2:
                txt = self.extract_pdf(full_path)
            elif fname.lower().endswith(".docx") and docx:
                txt = self.extract_docx(full_path)
            elif fname.lower().endswith(".epub") and epub:
                txt = self.extract_epub(full_path)
            elif fname.lower().endswith(".html") or fname.lower().endswith(".htm"):
                txt = self.extract_html(full_path)
            else:
                continue
            if txt:
                base = os.path.splitext(fname)[0]
                txt_path = os.path.join(folder, base + ".txt")
                with open(txt_path, "w", encoding="utf-8") as f:
                    f.write(txt)

    def extract_pdf(self, path):
        try:
            with open(path, "rb") as f:
                reader = PyPDF2.PdfReader(f)
                return "\n".join(page.extract_text() or "" for page in reader.pages)
        except Exception as e:
            return ""

    def extract_docx(self, path):
        try:
            doc = docx.Document(path)
            return "\n".join(paragraph.text for paragraph in doc.paragraphs)
        except Exception as e:
            return ""

    def extract_epub(self, path):
        try:
            book = epub.read_epub(path)
            text = []
            for item in book.get_items():
                if item.get_type() == ebooklib.ITEM_DOCUMENT:
                    soup = BeautifulSoup(item.get_content(), "html.parser")
                    text.append(soup.get_text())
            return "\n".join(text)
        except Exception as e:
            return ""

    def extract_html(self, path):
        try:
            from bs4 import BeautifulSoup
            with open(path, "r", encoding="utf-8") as f:
                soup = BeautifulSoup(f, "html.parser")
                return soup.get_text()
        except Exception as e:
            return ""

    def clean_gutenberg_text(self, raw):
        start_re = re.compile(r'\*\*\* *START OF (THE|THIS) PROJECT GUTENBERG EBOOK.*\*\*\*', re.IGNORECASE)
        end_re = re.compile(r'\*\*\* *END OF (THE|THIS) PROJECT GUTENBERG EBOOK.*\*\*\*', re.IGNORECASE)
        start = start_re.search(raw)
        end = end_re.search(raw)
        if start and end:
            return raw[start.end():end.start()].strip()
        elif start:
            return raw[start.end():].strip()
        elif end:
            return raw[:end.start()].strip()
        else:
            return raw[600:-600].strip() if len(raw) > 1200 else raw

    def gutenbergpy_download(self, book_id):
        if not gutenberg_textget:
            return None
        try:
            raw_bytes = gutenberg_textget.get_text_by_id(book_id)
            if raw_bytes is None:
                return None
            processed_bytes = raw_bytes
            if len(raw_bytes) >= 2 and raw_bytes[0] == 0x1f and raw_bytes[1] == 0x8b:
                try:
                    with gzip.GzipFile(fileobj=io.BytesIO(raw_bytes)) as gzfile:
                        processed_bytes = gzfile.read()
                except Exception:
                    processed_bytes = raw_bytes
            clean_bytes = gutenberg_textget.strip_headers(processed_bytes)
            try:
                decoded_text = clean_bytes.decode("utf-8")
                if len(decoded_text.strip()) < 100:
                    decoded_text = clean_bytes.decode("latin1", errors="ignore")
            except UnicodeDecodeError:
                decoded_text = clean_bytes.decode("latin1", errors="ignore")
            except Exception:
                decoded_text = ""
            return decoded_text
        except Exception as e:
            return None

    def search(self):
        if not self.docs_indexed:
            QMessageBox.warning(self, "Error", "Please index documents first.")
            return
        query = self.search_input.text().strip()
        if not query:
            QMessageBox.warning(self, "Input Error", "Please enter a search query.")
            return
        self.last_query = query
        ds_flag, ds_value = self.get_ds_flag()

        QApplication.setOverrideCursor(Qt.WaitCursor)
        self.current_worker = WorkerThread(['./search_engine', '--search', query, self.indexed_folder, ds_flag, ds_value])
        self.current_worker.finished.connect(self.on_search_finished)
        self.current_worker.error.connect(self.on_search_error)
        self.current_worker.start()

    def on_search_finished(self, result):
        QApplication.restoreOverrideCursor()
        if result.returncode != 0 or not result.stdout.strip():
            QMessageBox.information(self, "No Results", "No documents found containing your search.")
            self.results.clear()
        else:
            self.display_results(result.stdout)
        self.current_worker = None

    def on_search_error(self, error_message):
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None

    # --- REMOVED: feeling_lucky method and its associated on_lucky_finished/on_lucky_error ---

    def select_book_dialog(self, book_list):
        dialog = QDialog(self)
        dialog.setWindowTitle("Choose a book to download")
        dialog.setFixedSize(self.width(), 400)

        layout = QVBoxLayout(dialog)
        list_widget = QListWidget()
        list_widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        max_len = 70
        for title in book_list:
            display_title = (title[:max_len] + "...") if len(title) > max_len else title
            item = QListWidgetItem(display_title)
            if len(title) > max_len:
                item.setToolTip(title)
            list_widget.addItem(item)

        layout.addWidget(list_widget)

        btn_layout = QHBoxLayout()
        ok_btn = QPushButton("OK")
        cancel_btn = QPushButton("Cancel")
        btn_layout.addStretch()
        btn_layout.addWidget(ok_btn)
        btn_layout.addWidget(cancel_btn)
        layout.addLayout(btn_layout)

        selected = [None]

        def accept():
            if list_widget.currentRow() >= 0:
                selected[0] = list_widget.currentRow()
                dialog.accept()

        ok_btn.clicked.connect(accept)
        cancel_btn.clicked.connect(dialog.reject)
        list_widget.itemDoubleClicked.connect(lambda _: accept())

        dialog.exec_()
        return selected[0]

    def search_online(self):
        query, ok = QInputDialog.getText(self, "Search Online", "Enter book title or author:")
        if not ok or not query.strip():
            return

        QApplication.setOverrideCursor(Qt.WaitCursor)

        try:
            response = requests.get(GUTENDEX_API, params={'search': query})
            response.raise_for_status()
            data = response.json()
            books = data.get('results', [])
            if not books:
                QApplication.restoreOverrideCursor()
                QMessageBox.information(self, "No Results", "No books found for your query.")
                return
        except Exception as e:
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "Error", f"Failed to fetch from Gutenberg: {e}")
            return

        book_list = []
        for book in books:
            title = book.get("title", "Unknown Title")
            authors = book.get("authors", [])
            author_names = ", ".join(a.get("name", "") for a in authors) if authors else "Unknown Author"
            book_list.append(f"{title} by {author_names}")

        idx = self.select_book_dialog(book_list)
        if idx is None:
            QApplication.restoreOverrideCursor()
            return

        book = books[idx]
        formats = book.get("formats", {})
        text_url = formats.get("text/plain; charset=utf-8") or formats.get("text/plain")
        html_url = formats.get("text/html; charset=utf-8") or formats.get("text/html")
        epub_url = formats.get("application/epub+zip")
        book_id = book.get("id")
        content = None

        if gutenberg_textget and book_id:
            temp_content = self.gutenbergpy_download(book_id)
            if temp_content and len(temp_content.strip()) > 1000:
                content = temp_content

        if content is None and text_url:
            try:
                text_response = requests.get(text_url)
                text_response.raise_for_status()
                temp_content = text_response.text
                temp_content = self.clean_gutenberg_text(temp_content)
                if len(temp_content.strip()) > 1000:
                    content = temp_content
            except Exception:
                content = None

        if content is None and book_id:
            for suffix in ["-0.txt", ".txt"]:
                try:
                    alt_url = f"https://www.gutenberg.org/files/{book_id}/{book_id}{suffix}"
                    text_response = requests.get(alt_url)
                    if text_response.status_code == 200:
                        temp_content = self.clean_gutenberg_text(text_response.text)
                        if len(temp_content.strip()) > 1000:
                            content = temp_content
                            break
                except Exception:
                    continue

        if content is None and html_url:
            try:
                html_response = requests.get(html_url)
                html_response.raise_for_status()
                soup = BeautifulSoup(html_response.text, "html.parser")
                temp_content = soup.get_text()
                if len(temp_content.strip()) > 1000:
                    content = temp_content
            except Exception:
                content = None

        if content is None and book_id:
            html_patterns = [
                f"https://www.gutenberg.org/files/{book_id}/{book_id}-h/{book_id}-h.htm",
                f"https://www.gutenberg.org/files/{book_id}/{book_id}-h/{book_id}-h.html",
                f"https://www.gutenberg.org/files/{book_id}/{book_id}-h.htm",
                f"https://www.gutenberg.org/files/{book_id}/{book_id}-h.html",
                f"https://www.gutenberg.org/files/{book_id}/{book_id}.htm",
                f"https://www.gutenberg.org/files/{book_id}/{book_id}.html",
            ]
            for html_url_canonical in html_patterns:
                try:
                    html_response = requests.get(html_url_canonical)
                    if html_response.status_code == 200:
                        soup = BeautifulSoup(html_response.text, "html.parser")
                        temp_content = soup.get_text()
                        if len(temp_content.strip()) > 1000:
                            content = temp_content
                            break
                except Exception:
                    continue

        if content is None and epub_url and epub:
            try:
                import tempfile
                epub_response = requests.get(epub_url)
                epub_response.raise_for_status()
                with tempfile.NamedTemporaryFile(delete=False, suffix=".epub") as tmpf:
                    tmpf.write(epub_response.content)
                    tmpf.flush()
                txt = self.extract_epub(tmpf.name)
                if txt and len(txt) > 1000:
                    content = txt
            except Exception:
                content = None

        if not content or len(content.strip()) < 1000:
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "No Text", "Could not download a plain text, HTML, or EPUB version for this book, or text was too short.")
            return

        title = book.get("title", "gutenberg_book").replace(" ", "_")
        invalid_chars = r'[<>:"/\\|?*\x00-\x1f]'
        title = re.sub(invalid_chars, '', title)
        title = title[:60]

        filename = f"{title}_{book_id}.txt"
        full_path = os.path.join(self.indexed_folder, filename)

        target_dir = os.path.dirname(full_path)
        if not target_dir:
            target_dir = ".."

        if not os.path.exists(target_dir):
            try:
                os.makedirs(target_dir)
            except OSError as e:
                QApplication.restoreOverrideCursor()
                QMessageBox.warning(self, "File System Error", f"Could not create directory '{target_dir}': {e}. Please ensure you have write permissions.")
                return

        if not os.access(target_dir, os.W_OK):
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "Permissions Error", f"Cannot write to directory '{target_dir}'. Please check your folder permissions.")
            return

        try:
            with open(full_path, "w", encoding="utf-8") as f:
                f.write(content)
        except Exception as e:
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "File Error", f"Could not save book to '{full_path}': {e}")
            return

        self.current_worker = WorkerThread(['./search_engine', '--add-file', full_path, self.indexed_folder], is_add_file=True)
        self.current_worker.finished.connect(lambda res: self.on_add_file_finished(res, filename))
        self.current_worker.error.connect(self.on_add_file_error)
        self.current_worker.start()

    def on_add_file_finished(self, result, filename):
        QApplication.restoreOverrideCursor()
        if result.returncode == 0:
            self.docs_indexed = True
            QMessageBox.information(self, "Book Added", f"Book '{filename}' downloaded and indexed! You can now search it locally.")
        else:
            QMessageBox.warning(self, "Indexing Failed", result.stderr or f"Failed to index downloaded book '{filename}'.")
        self.current_worker = None

    def on_add_file_error(self, error_message):
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None

    def display_results(self, text):
        self.results.clear()
        self.doc_map.clear()
        self.doc_id_map.clear()
        for line in text.strip().splitlines():
            m = re.match(r"Document (\d+):", line)
            if not m:
                continue
            doc_id = int(m.group(1))
            item = QListWidgetItem(line)
            self.results.addItem(item)
            row = self.results.count() - 1
            self.doc_id_map[row] = doc_id
            self.doc_map[line] = line

    def show_document(self, item):
        row = self.results.row(item)
        doc_id = self.doc_id_map.get(row, None)
        if doc_id is not None and self.last_query:
            ds_flag, ds_value = self.get_ds_flag()

            QApplication.setOverrideCursor(Qt.WaitCursor)
            self.current_worker = WorkerThread(
                ['./search_engine', '--snippets', self.last_query, str(doc_id), self.indexed_folder, ds_flag, ds_value]
            )
            self.current_worker.finished.connect(self.on_show_document_finished)
            self.current_worker.error.connect(self.on_show_document_error)
            self.current_worker.start()
        else:
            content = self.doc_map.get(item.text(), "")
            self.show_document_content(content, highlight=False)
    """
        dialog = QMessageBox(self);
        dialog.setWindowTitle("Action")
        dialog.setText(f"Document {doc_id}")
        dialog.setInformativeText("Pick action you would like to perform.")

        snippets_btn = dialog.addButton("View Snippets", QMessageBox.ActionRole)
        summary_btn = dialog.addButton("Generate Summary", QMessageBox.ActionRole)
        dialog.addButton(QMessageBox.Cancel)

        dialog.exec_()
        clicked_btn = dialog.clickedButton()

        if clicked_btn == snippets_btn:
            self.show_snippets_for_document(doc_id)
        elif clicked_btn == summary_btn:
            self.start_summarization(doc_id)
    
    def show_snippets_for_document(self, doc_id):
        ds_flag, ds_value = self.get_ds_flag()
        QApplication.setOverrideCursor(Qt.WaitCursor)
        self.current_worker = WorkerThread(
            ['./search_engine', '--snippets', self.last_query, str(doc_id),
             self.indexed_folder, ds_flag, ds_value]
        )

        self.current_worker.finished.connect(self.on_show_document_finished)
        self.current_worker.error.connect(self.on_search_error)
        self.current_worker.start()

    def start_summarization(self, doc_id):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        self.current_worker = WorkerThread(
            ['./search_engine', '--get-content', str(doc_id), self.indexed_folder]
        )
        self.current_worker.finished.connect(self.summary_recieved)
        self.current_worker.error.connect(self.on_worker_error)
        self.current_worker.start()
    
    def summary_recieved(self, result):
        doc_content = result.stdout
        self.summarizer_worker = SummarizerWorker(doc_content)
        self.summarizer_worker.finished.connect(self.summary_displayed)
        self.summarizer_worker.error.connect(self.on_worker_error)
        self.summarizer_worker.start()

    def summary_displayed(self, summary_text):
        QApplication.restoreOverrideCursor()
        self.current_worker = None
        self.summarizer_worker = None

        viewer = QTextEdit()
        viewer.setReadOnly(True)
        viewer.setPlainText(summary_text)
        viewer.setWindowTitle("Document Summary")
        viewer.resize(600, 250)
        viewer.show()
        self._last_view = viewer
    """

    def on_show_document_finished(self, result):
        QApplication.restoreOverrideCursor()
        snippets = result.stdout if result.stdout is not None else "No snippets found."
        if result.returncode != 0:
            snippets += f"\n(Search engine error: {result.stderr})"
        self.show_document_content(snippets, highlight=True)
        self.current_worker = None

    def on_show_document_error(self, error_message):
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None

    """
    def on_worker_error(self, error_message):
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Worker Error", error_message)
        self.current_worker = None
        self.summarizer_worker = None
    """
    def show_document_content(self, content, highlight=True):
        if content is None:
            content = ""
        viewer = QTextEdit()
        viewer.setReadOnly(True)
        if highlight and self.last_query:
            html = self.highlight_text(content, self.last_query)
            viewer.setHtml(html)
        else:
            viewer.setPlainText(content)
        viewer.setWindowTitle("Document Viewer")
        viewer.resize(600, 400)
        viewer.show()
        self._last_viewer = viewer

    def highlight_text(self, text, query):
        if not text:
            return ""
        def html_escape(s):
            return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        q = re.escape(query)
        def replacer(match):
            return f'<span style="background-color: yellow">{html_escape(match.group(0))}</span>'
        highlighted = re.sub(q, replacer, html_escape(text), flags=re.IGNORECASE)
        return f"<pre style='font-family:monospace'>{highlighted}</pre>"

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SearchWindow()
    window.show()
    sys.exit(app.exec_())