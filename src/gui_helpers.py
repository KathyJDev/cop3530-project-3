import re
from PyQt5.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QListWidget, QListWidgetItem,
    QPushButton, QSizePolicy
)
from PyQt5.QtCore import Qt

def select_book_dialog(parent_widget, book_list):
    """
    Presents a modal dialog for the user to select a book from a list.

    Args:
        parent_widget (QWidget): The parent widget for the dialog (e.g., the main window).
        book_list (list): A list of strings, each representing a book (e.g., "Title by Author").

    Returns:
        int or None: The index of the selected book, or None if the user cancels.
    """
    dialog = QDialog(parent_widget)
    dialog.setWindowTitle("Choose a book to download")
    
    # Remove context help button from dialog
    flags = dialog.windowFlags()
    flags &= ~Qt.WindowContextHelpButtonHint
    dialog.setWindowFlags(flags)
    dialog.setFixedSize(parent_widget.width(), 400) # Size relative to parent

    layout = QVBoxLayout(dialog)
    list_widget = QListWidget()
    list_widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

    max_len = 70 # Max length for display, tooltip for full title
    for title_str in book_list:
        display_title = (title_str[:max_len] + "...") if len(title_str) > max_len else title_str
        item = QListWidgetItem(display_title)
        if len(title_str) > max_len:
            item.setToolTip(title_str) # Show full title on hover
        list_widget.addItem(item)

    layout.addWidget(list_widget)

    btn_layout = QHBoxLayout()
    ok_btn = QPushButton("OK")
    cancel_btn = QPushButton("Cancel")
    btn_layout.addStretch()
    btn_layout.addWidget(ok_btn)
    btn_layout.addWidget(cancel_btn)
    layout.addLayout(btn_layout)

    selected_idx = [None] # Use a list to allow modification in nested function

    def accept_selection():
        if list_widget.currentRow() >= 0:
            selected_idx[0] = list_widget.currentRow()
            dialog.accept()

    ok_btn.clicked.connect(accept_selection)
    cancel_btn.clicked.connect(dialog.reject)
    list_widget.itemDoubleClicked.connect(lambda: accept_selection()) # Double-click to select

    dialog.exec_() # Run the dialog modally
    return selected_idx[0]


def highlight_text(text, query):
    """
    Applies HTML highlighting to occurrences of the query within the text.

    Args:
        text (str): The full text content.
        query (str): The keyword or phrase to highlight.

    Returns:
        str: HTML formatted text with the query highlighted.
    """
    if not text:
        return ""
    
    def html_escape(s):
        """Escapes HTML special characters in a string."""
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    
    # Escape query for regex, then highlight
    q_escaped = re.escape(query)
    
    # Replace matches with highlighted HTML
    def replacer(match):
        return f'<span style="background-color: yellow">{html_escape(match.group(0))}</span>'
    
    highlighted = re.sub(q_escaped, replacer, html_escape(text), flags=re.IGNORECASE)
    
    # Wrap in <pre> tags to preserve whitespace and line breaks, and set monospace font
    return f"<pre style='font-family:monospace'>{highlighted}</pre>"