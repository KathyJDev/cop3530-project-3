"""
from transformers import pipeline

class DocSummary:
    def __init__ (self):
        self.model_name = "gpt2-medium"
        self.tokenizer = None
        self.summarizer = None
        self.device = 0

    def load_model(self):
        self.summarizer = pipeline(
            "summarization",
            model = self.model_name,
            device = self.device
        )

    def summarize(self, text, max_length = 150, min_length = 30):
        if self.summarizer is None:
            self.load_model()

        summary = self.summarizer(text, max_length = max_length, min_length = min_length)
        return summary

"""
