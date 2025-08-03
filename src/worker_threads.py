import subprocess
from PyQt5.QtCore import QThread, pyqtSignal

class WorkerThread(QThread):
    """
    A QThread subclass to run external subprocess commands in the background,
    preventing the GUI from freezing.
    """
    finished = pyqtSignal(object) # Emits the completed subprocess.CompletedProcess object
    error = pyqtSignal(str)      # Emits an error message string

    def __init__(self, command_args, is_add_file=False):
        """
        Initializes the WorkerThread.

        Args:
            command_args (list): A list of strings representing the command and its arguments.
            is_add_file (bool): A flag to indicate if the task is adding a new file,
                                 which might influence error handling or feedback.
        """
        super().__init__()
        self.command_args = command_args
        self.is_add_file = is_add_file

    def run(self):
        """
        Executes the subprocess command and emits signals upon completion or error.
        """
        try:
            # Run the external executable, capturing its output
            # errors='replace' handles characters that can't be decoded
            result = subprocess.run(
                self.command_args,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors='replace',
                check=False # Do not raise CalledProcessError for non-zero exit codes
            )
            self.finished.emit(result)
        except FileNotFoundError:
            self.error.emit(
                f"Error: search_engine executable not found. Make sure it's compiled and in the same directory."
            )
        except Exception as e:
            self.error.emit(f"An unexpected error occurred: {e}")

# If you decide to re-implement summarization later, the SummarizerWorker would go here.
# """
# class SummarizerWorker(QThread):
#     finished = pyqtSignal(str)
#     error = pyqtSignal(str)
#     def __init__(self, text_to_summarize):
#         super().__init__()
#         self.text = text_to_summarize
#     def run(self):
#         try:
#             words = self.text.split()
#             reduced_content = " ".join(words[:1000])
#             summarizer = DocSummary()
#             summary_list = summarizer.summarize(reduced_content)
#             self.finished.emit(summary_list[0]['summary_text'])
#         except Exception as e:
#             self.error.emit(f"Error Summary unable to generate:{e}")
# """