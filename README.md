# Simple Search Engine – Project 3
<p align="center">
  <img src="resources/images/logo.png" alt="Project Logo" width="200" height="200">
</p>

A fast and extensible search engine for local documents and Project Gutenberg books, featuring **inverted index** and **suffix array** search, a C++ backend, and an optional PyQt5 GUI.

## 🚀 Features

- Index local `.txt` documents for **keyword** or **phrase** search
- Choose Inverted Index or Suffix Array for query processing
- Benchmark search speed
- Download and index books from [Project Gutenberg](https://gutenberg.org) (via title or author)
- GUI (PyQt5) and Command-Line Interface
- Cross-platform C++ backend (Windows & Linux/macOS supported)

## 🛠️ Requirements

### C++ Backend

- **g++** (MinGW-w64 64-bit on Windows, or any recent g++ on Linux/macOS)
- **C++14** or newer (`-std=c++14`)
- [`curl`](https://curl.se/) utility (optional; for raw CLI downloads)
- **On Windows:** Link with `-lws2_32`

### Python GUI (Optional)

- [Python 3.7+](https://www.python.org/)
- [PyQt5](https://pypi.org/project/PyQt5/)
- [requests](https://pypi.org/project/requests/)
- For PDF, docx, epub support, and for GUI to work the best: `PyPDF2`, `python-docx`, `ebooklib`, `beautifulsoup4`)

## 📁 Directory Layout

- `repo/`
    - `main.cpp`
    - `document.cpp/h`
    - `tokenizer.cpp/h`
    - `inverted_index.cpp/h`
    - `suffix_array.cpp/h`
    - `performance.cpp/h`
    - `utils.cpp/h`
    - `httplib.h` # Required C++ HTTP library
    - `gui.py` # Python GUI script (optional, if used)
    - `[Any other dependencies]`
    - `[test_data/]` # Your documents folder (place text files here)
    - `README.md`


## ⚡ Quick Start

### 1. Install Dependencies

**C++ Backend Dependencies:**

- **g++:**
  - **Windows (MSYS2/MinGW):**
    ```bash
    pacman -S mingw-w64-x86_64-gcc
    ```
  - **Linux / macOS:**
    ```bash
    # Debian/Ubuntu
    sudo apt update && sudo apt install build-essential
    # macOS (with Homebrew)
    brew install gcc
    ```
- **curl:**
  - **Windows:** Install separately if not available (e.g., via [scoop](https://scoop.sh/) or directly from [curl.se](https://curl.se/)).
  - **Linux / macOS:**
    ```bash
    # Debian/Ubuntu
    sudo apt install curl
    # macOS (usually pre-installed, or via Homebrew)
    brew install curl
    ```

**Python GUI Dependencies (Optional):**
```bash
pip install pyqt5 requests pypdf2 python-docx ebooklib beautifulsoup4 gutenbergpy
```

### 2. Build the C++ Backend

In your project root directory, run the appropriate command:

**Windows (using MinGW/MSYS2):**
```bash
g++ -std=c++14 src/main.cpp src/menu.cpp src/gutenberg.cpp  src/document.cpp src/tokenizer.cpp src/inverted_index.cpp src/suffix_array.cpp src/performance.cpp src/utils.cpp -o search_engine.exe -lws2_32
```

**Linux / macOS:**
```bash
g++ -std=c++14 src/main.cpp src/menu.cpp src/gutenberg.cpp  src/document.cpp src/tokenizer.cpp src/inverted_index.cpp src/suffix_array.cpp src/performance.cpp src/utils.cpp -o search_engine
```

> **Note:** A exe should be provided in the project root directory, this is in case it doesn't work

### 3. Run the Application

#### Command-Line Interface (CLI)

To start the interactive CLI menu:

**Windows Command Prompt / PowerShell**
```bash
search_engine.exe
```
**Windows MSYS2/Git Bash**
```bash
./search_engine.exe
```
**Linux/macOS**
```bash
./search_engine
```

You’ll see a menu:
```
------------------------------------------
|         Simple Search Engine           |
------------------------------------------
| 1. Index Documents                    |
| 2. Search Keywords                    |
| 3. Search Phrase                      |
| 4. Performance Report                 |
| 5. Exit                               |
| 6. Download Book from Gutenberg       |
------------------------------------------
Enter your choice:
```

- **Option 1:** Type the path to your local documents folder (e.g., `data`)
- **Option 2/3:** Search for keywords or phrases (indexing must be done first)
- **Option 6:** Download and index books from Project Gutenberg

> **Note:** A dataset of books from Gutenberg is already provided in test_data/

#### Python GUI

Run:
```bash
python src/gui.py
```
(Follow the GUI’s prompts.)
## ⭐ Future Implementations
- Using a text generation model, it would be able to generate summaries of the document you have chosen.
- Relevant Files: gui.py, generate_summary.py
- Source model: https://huggingface.co/openai-community/gpt2-medium
- @article{radford2019language,
  title={Language models are unsupervised multitask learners},
  author={Radford, Alec and Wu, Jeffrey and Child, Rewon and Luan, David and Amodei, Dario and Sutskever, Ilya and others},
  journal={OpenAI blog},
  volume={1},
  number={8},
  pages={9},
  year={2019}
}


## 📝 Other Files
- Files with test_XXXXX.cpp files are used to test through each function of the code using Catch2
  

## ❓ Troubleshooting

**No output when running .exe:**
- Check you’re running the correct filename (e.g., `search-engine.exe` not `search_engine.exe`)
- Try running from a different shell (e.g., CMD or PowerShell on Windows).
- Add `std::cout << "Program started!" << std::endl;` at the top of `main()` and see if it prints.

**Build/link errors (`__imp_*`):**
- On Windows, ensure you have `-lws2_32` in your compile command.

**Project Gutenberg download fails:**
- Check your internet connection.
- Make sure nothing is blocking outbound HTTP(S).
- On Windows, ensure your compiler and WinSock are up-to-date (see README).

**Invisible Characters/Early Exit:**
- Only edit code in a plain text editor; avoid MS Word or other tools that may introduce invisible Unicode characters.
- If the program still exits instantly and prints nothing, clean your source file(s) of non-breaking spaces.


## 📚 References
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [PyQt5](https://wiki.python.org/moin/PyQt)
- [Project Gutenberg](https://gutenberg.org/)
- [os module](https://docs.python.org/3/library/os.html)

## 📝 License

This code is for educational/non-commercial use, by COP3530 or similar courses.

## 🆘 Getting Help

If you encounter any issues:
- Paste your build command and error output
- Paste your terminal output from running the executable
- List your operating system and compiler version

## 🎉 Happy searching and building!
