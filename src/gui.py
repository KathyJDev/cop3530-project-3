import sys
import os
import re
import requests
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLineEdit, QPushButton,
    QLabel, QListWidget, QListWidgetItem, QMessageBox, QInputDialog, QTextEdit,
    QFileDialog, QDialog, QSizePolicy, QButtonGroup # QDialog, QSizePolicy, QButtonGroup still needed for other UI elements
)
from PyQt5.QtGui import QPixmap, QFont, QIcon
from PyQt5.QtCore import Qt, QSize

# Import refactored components
from worker_threads import WorkerThread
from file_converters import convert_all_non_txt
from gutenberg_api import search_gutenberg_books, download_gutenberg_content, save_book_to_file
from gui_helpers import select_book_dialog, highlight_text # NEW IMPORT

class SearchWindow(QWidget):
    """
    Main GUI window for the Simple Search Engine.
    Handles user interaction, triggers background tasks, and displays results.
    """
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Simple Search Engine with Gutenberg")
        self.setFixedSize(750, 580)
        
        # State variables
        self.docs_indexed = False
        self.indexed_folder = "." # Default to current directory
        self.last_query = ""      # Stores the last search query for snippets/full view
        self.doc_id_map = {}      # Maps QListWidget row index to internal document ID
        self.doc_map = {}         # Maps display text to raw snippet (for non-snippet search fallback)
        self.current_worker = None # To manage ongoing background tasks
        self._last_viewer = None  # Holds a reference to the last opened text viewer to prevent GC

        self.init_ui()

    def init_ui(self):
        """Initializes the graphical user interface elements."""
        main_layout = QVBoxLayout()

        # Logo/Title
        self.logo_label = QLabel()
        self.setStyleSheet("background-color: #202020; color: white;") # General window styling

        pixmap = QPixmap("resources/images/logo.png")
        if pixmap.isNull():
            self.logo_label.setText("Search Engine")
            self.logo_label.setFont(QFont("Arial", 24))
            self.logo_label.setStyleSheet("font-weight: 600;")
        else:
            self.logo_label.setPixmap(pixmap.scaled(200, 70, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        self.logo_label.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(self.logo_label)

        # Search Method Toggle Buttons
        toggle_layout = QHBoxLayout()
        self.inverted_btn = QPushButton("Inverted Index")
        self.suffix_btn = QPushButton("Suffix Array")
        btn_font = QFont("Arial", 10)
        
        for btn in (self.inverted_btn, self.suffix_btn):
            btn.setFont(btn_font)
            btn.setCursor(Qt.PointingHandCursor)
            btn.setCheckable(True)
            btn.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
            btn.setStyleSheet("""
                QPushButton { padding: 6px 12px; border-radius: 4px; background-color: #202020; color: #8E8E8E; font-weight: 500; }
                QPushButton:checked { background-color: #414141; color: white; }
                QPushButton:hover { background-color: #414141; color: white; }
            """)
        
        toggle_group = QButtonGroup(self)
        toggle_group.setExclusive(True)
        toggle_group.addButton(self.inverted_btn)
        toggle_group.addButton(self.suffix_btn)
        self.inverted_btn.setChecked(True) # Default selection

        toggle_layout.addStretch()
        toggle_layout.addWidget(self.inverted_btn)
        toggle_layout.addWidget(self.suffix_btn)
        toggle_layout.addStretch()
        toggle_layout.setContentsMargins(0, 6, 0, 6)
        main_layout.addLayout(toggle_layout)

        # Search Input Field and Button
        search_layout = QHBoxLayout()
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("type keywords or phrases...")
        self.search_input.setFont(QFont("Inter", 11))
        self.search_input.setMinimumHeight(32)
        self.search_input.setStyleSheet("""
            QLineEdit { padding: 8px 24px; border-radius: 16px; background-color: #D9D9D9; color: #202020; font-weight: 400; }
        """)
        
        self.search_icon = QPushButton()
        self.search_icon.setIcon(QIcon("resources/images/search.svg"))
        self.search_icon.setIconSize(QSize(16, 16))
        self.search_icon.setFixedSize(40, 40)
        self.search_icon.setStyleSheet("""
            QPushButton { background-color: #414141; border-radius: 20px; }
        """)
        self.search_icon.setCursor(Qt.PointingHandCursor)
        
        search_layout.addWidget(self.search_input)
        search_layout.addWidget(self.search_icon)
        search_layout.setContentsMargins(48, 0, 48, 0)
        main_layout.addLayout(search_layout)

        # Document Indexing and Online Search Buttons
        action_buttons_layout = QVBoxLayout()
        self.index_btn = QPushButton("Index Local Documents")
        self.online_btn = QPushButton("Search Online (Gutenberg)")
        
        for btn in (self.index_btn, self.online_btn):
            btn.setFont(QFont("Arial", 10))
            btn.setStyleSheet("""
                QPushButton { padding: 6px; border-radius: 4px; background-color: #202020; color: white; font-weight: 500; }
                QPushButton:hover { background-color: #414141; }
            """)
            btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        
        action_buttons_layout.addWidget(self.index_btn)
        action_buttons_layout.addWidget(self.online_btn)
        main_layout.addLayout(action_buttons_layout)

        # Document Results List
        self.results = QListWidget()
        self.results.setMinimumHeight(150)
        self.results.setFont(QFont("Arial", 11))
        self.results.setStyleSheet("""
            QListWidget { border-radius: 8px; color: black; background-color: #D9D9D9; }
        """)
        main_layout.addWidget(self.results)

        # View Full Document Button
        self.view_full_doc_btn = QPushButton("View Full Document")
        self.view_full_doc_btn.setEnabled(False) # Disabled by default
        self.view_full_doc_btn.setFont(QFont("Arial", 10))
        self.view_full_doc_btn.setStyleSheet("""
            QPushButton { padding: 6px; border-radius: 4px; background-color: #414141; color: white; font-weight: 500; }
            QPushButton:hover { background-color: #555555; }
            QPushButton:disabled { background-color: #252525; color: #666666; }
        """)
        main_layout.addWidget(self.view_full_doc_btn)

        self.setLayout(main_layout)

        # Connect signals to slots
        self.index_btn.clicked.connect(self.index_documents)
        self.search_icon.clicked.connect(self.search)
        self.online_btn.clicked.connect(self.search_online)
        self.results.itemDoubleClicked.connect(self.show_document) # Double-click to view snippets
        self.results.itemSelectionChanged.connect(self.on_selection_changed)
        self.view_full_doc_btn.clicked.connect(self.view_full_document) # Button to view full document


    def get_ds_flag(self):
        """
        Determines the selected data structure flag for the C++ backend.
        Returns:
            tuple: A tuple ('--ds', 'suffix') or ('--ds', 'inverted').
        """
        return ('--ds', 'suffix') if self.suffix_btn.isChecked() else ('--ds', 'inverted')


    ## Local Document Indexing


    def index_documents(self):
        """
        Prompts the user to select a directory, converts non-TXT files,
        and starts a worker thread to index documents using the C++ backend.
        """
        folder = QFileDialog.getExistingDirectory(self, "Select Documents Directory")
        if not folder:
            return # User canceled

        self.indexed_folder = folder
        QApplication.setOverrideCursor(Qt.WaitCursor) # Show busy cursor

        # Convert non-text files to .txt using the external utility
        convert_all_non_txt(folder)

        # Start the C++ search engine for indexing
        self.current_worker = WorkerThread(['./search_engine', '--index', folder])
        self.current_worker.finished.connect(self.on_indexing_finished)
        self.current_worker.error.connect(self.on_indexing_error)
        self.current_worker.start()

    def on_indexing_finished(self, result):
        """Handles the completion of the indexing process."""
        QApplication.restoreOverrideCursor() # Restore normal cursor
        if result.returncode == 0:
            self.docs_indexed = True
            QMessageBox.information(self, "Indexing Complete", "Documents indexed successfully!")
        else:
            QMessageBox.warning(self, "Indexing Failed", result.stderr or "Failed to index documents.")
        self.current_worker = None # Clear worker reference

    def on_indexing_error(self, error_message):
        """Handles errors during the indexing process."""
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None


    ## Search Functionality


    def search(self):
        """
        Initiates a search operation based on user input.
        Sends the query to the C++ backend via a worker thread.
        """
        if not self.docs_indexed:
            QMessageBox.warning(self, "Error", "Please index documents first.")
            return

        query = self.search_input.text().strip()
        if not query:
            QMessageBox.warning(self, "Input Error", "Please enter a search query.")
            return

        self.last_query = query # Store query for later snippet/full view
        ds_flag, ds_value = self.get_ds_flag() # Get selected data structure

        QApplication.setOverrideCursor(Qt.WaitCursor)
        self.current_worker = WorkerThread(
            ['./search_engine', '--search', query, self.indexed_folder, ds_flag, ds_value]
        )
        self.current_worker.finished.connect(self.on_search_finished)
        self.current_worker.error.connect(self.on_search_error)
        self.current_worker.start()

    def on_search_finished(self, result):
        """Handles search results returned from the worker thread."""
        QApplication.restoreOverrideCursor()
        if result.returncode != 0 or not result.stdout.strip():
            QMessageBox.information(self, "No Results", "No documents found containing your search.")
            self.results.clear()
        else:
            self.display_results(result.stdout) # Populate QListWidget with results
        self.current_worker = None

    def on_search_error(self, error_message):
        """Handles errors during the search operation."""
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None

    def display_results(self, text):
        """
        Parses search engine output and displays results in the QListWidget.
        Maps document IDs to list widget rows for later retrieval.
        """
        self.results.clear()
        self.doc_map.clear()
        self.doc_id_map.clear()
        for line in text.strip().splitlines():
            m = re.match(r"Document (\d+): (.+)", line) # Capture ID and a bit of preview
            if not m:
                continue
            doc_id = int(m.group(1))
            display_text = m.group(0) # Keep full line for display
            
            item = QListWidgetItem(display_text)
            self.results.addItem(item)
            
            row = self.results.count() - 1
            self.doc_id_map[row] = doc_id # Store mapping
            self.doc_map[display_text] = line # Store original line (if needed for non-snippet view)


    ## Online Search (Gutenberg)


    # Removed select_book_dialog from here, now in gui_helpers.py

    def search_online(self):
        """
        Handles the online search functionality via Project Gutenberg.
        Prompts for a query, fetches books, allows selection, downloads, and indexes the chosen book.
        """
        query, ok = QInputDialog.getText(self, "Search Online", "Enter book title or author:")
        if not ok or not query.strip():
            return

        QApplication.setOverrideCursor(Qt.WaitCursor)

        try:
            books = search_gutenberg_books(query) # Call external API function
            if not books:
                QApplication.restoreOverrideCursor()
                QMessageBox.information(self, "No Results", "No books found for your query on Project Gutenberg.")
                return
        except requests.exceptions.RequestException as e:
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "Error", f"Failed to fetch books from Gutenberg: {e}")
            return

        book_list_display = []
        for book in books:
            title = book.get("title", "Unknown Title")
            authors = book.get("authors", [])
            author_names = ", ".join(a.get("name", "") for a in authors) if authors else "Unknown Author"
            book_list_display.append(f"{title} by {author_names}")

        # Call the external select_book_dialog function, passing self as parent
        idx = select_book_dialog(self, book_list_display) 
        if idx is None: # User canceled selection
            QApplication.restoreOverrideCursor()
            return

        # Prompt user for a folder to save the book
        container_folder = QFileDialog.getExistingDirectory(
            self,
            "Select Folder to Save Book In",
            self.indexed_folder # Default to last indexed folder
        )
        if not container_folder:
            QApplication.restoreOverrideCursor()
            return # User canceled folder selection
        self.indexed_folder = container_folder # Update indexed folder for future use

        selected_book_info = books[idx]
        
        # Download book content using the external utility
        content, download_error = download_gutenberg_content(selected_book_info)
        if download_error:
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "Download Error", download_error)
            return

        # Save the book content to a local file using the external utility
        full_path, save_error = save_book_to_file(selected_book_info, content, self.indexed_folder)
        if save_error:
            QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "Save Error", save_error)
            return

        # Index the newly added file
        self.current_worker = WorkerThread(
            ['./search_engine', '--add-file', full_path, self.indexed_folder],
            is_add_file=True
        )
        self.current_worker.finished.connect(lambda res: self.on_add_file_finished(res, os.path.basename(full_path)))
        self.current_worker.error.connect(self.on_add_file_error)
        self.current_worker.start()

    def on_add_file_finished(self, result, filename):
        """Handles the completion of adding and indexing a new file."""
        QApplication.restoreOverrideCursor()
        if result.returncode == 0:
            self.docs_indexed = True
            QMessageBox.information(self, "Book Added", f"Book '{filename}' downloaded and indexed! You can now search it locally.")
        else:
            QMessageBox.warning(self, "Indexing Failed", result.stderr or f"Failed to index downloaded book '{filename}'.")
        self.current_worker = None

    def on_add_file_error(self, error_message):
        """Handles errors during the adding/indexing of a new file."""
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None


    ## Document Viewing


    def on_selection_changed(self):
        """Enables/disables the 'View Full Document' button based on selection."""
        self.view_full_doc_btn.setEnabled(len(self.results.selectedItems()) > 0)

    def view_full_document(self):
        """
        Retrieves and displays the full content of the selected document.
        Uses the C++ backend to get content and highlights the last search query.
        """
        selected_items = self.results.selectedItems()
        if not selected_items:
            return

        row = self.results.row(selected_items[0])
        doc_id = self.doc_id_map.get(row)

        if doc_id is not None:
            QApplication.setOverrideCursor(Qt.WaitCursor)
            # Request full content from C++ search engine
            self.current_worker = WorkerThread(['./search_engine', '--get-content', str(doc_id), self.indexed_folder])
            self.current_worker.finished.connect(self.on_get_content_finished)
            self.current_worker.error.connect(self.on_show_document_error)
            self.current_worker.start()

    def show_document(self, item):
        """
        Displays snippets for the double-clicked document.
        If no query or document ID mapping, falls back to showing raw document text.
        """
        row = self.results.row(item)
        doc_id = self.doc_id_map.get(row, None)

        if doc_id is not None and self.last_query:
            ds_flag, ds_value = self.get_ds_flag()
            QApplication.setOverrideCursor(Qt.WaitCursor)
            # Request snippets from C++ search engine for the last query
            self.current_worker = WorkerThread(
                ['./search_engine', '--snippets', self.last_query, str(doc_id), self.indexed_folder, ds_flag, ds_value]
            )
            self.current_worker.finished.connect(self.on_show_document_finished)
            self.current_worker.error.connect(self.on_show_document_error)
            self.current_worker.start()
        else:
            # Fallback for displaying content if no proper doc_id or last_query
            content = self.doc_map.get(item.text(), "")
            self.show_document_content(content, highlight=False)

    def on_get_content_finished(self, result):
        """Handles the return of full document content."""
        QApplication.restoreOverrideCursor()
        if result.returncode == 0:
            # Call the external highlight_text function
            self.show_document_content(result.stdout, highlight=True)
        else:
            QMessageBox.warning(self, "Error", f"Could not retrieve document content:\n{result.stderr}")
        self.current_worker = None

    def on_show_document_finished(self, result):
        """Handles the return of document snippets."""
        QApplication.restoreOverrideCursor()
        snippets = result.stdout if result.stdout is not None else "No snippets found."
        if result.returncode != 0:
            snippets += f"\n(Search engine error: {result.stderr})"
        self.show_document_content(snippets, highlight=True) # Display with highlighting
        self.current_worker = None

    def on_show_document_error(self, error_message):
        """Generic error handler for document viewing related workers."""
        QApplication.restoreOverrideCursor()
        QMessageBox.warning(self, "Error", error_message)
        self.current_worker = None

    def show_document_content(self, content, highlight=True):
        """
        Displays document content (or snippets) in a new QTextEdit window.
        Applies HTML highlighting if requested and a query is available.
        """
        if content is None:
            content = ""
        
        viewer = QTextEdit()
        viewer.setReadOnly(True)
        
        if highlight and self.last_query:
            # Call the external highlight_text function
            html = highlight_text(content, self.last_query) 
            viewer.setHtml(html)
        else:
            viewer.setPlainText(content)
        
        viewer.setWindowTitle("Document Viewer")
        viewer.resize(600, 400)
        viewer.show()
        self._last_viewer = viewer # Keep reference to prevent early garbage collection

    # Removed highlight_text from here, now in gui_helpers.py


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SearchWindow()
    window.show()
    sys.exit(app.exec_())