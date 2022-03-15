from typing import Any

def isatty(stream): ...
def abort(msg): ...
def warn(msg): ...
def indent(text, spaces: int = ..., strip: bool = ...): ...
def puts(text, show_prefix: Any | None = ..., end: str = ..., flush: bool = ...) -> None: ...
def fastprint(text, show_prefix: bool = ..., end: str = ..., flush: bool = ...): ...
def handle_prompt_abort(prompt_for) -> None: ...

class _AttributeDict(dict):
    def __getattr__(self, key): ...
    def __setattr__(self, key, value) -> None: ...
    def first(self, *names): ...

class _AliasDict(_AttributeDict):
    def __init__(self, arg: Any | None = ..., aliases: Any | None = ...) -> None: ...
    def __setitem__(self, key, value): ...
    def expand_aliases(self, keys): ...

def error(message, func: Any | None = ..., exception: Any | None = ..., stdout: Any | None = ..., stderr: Any | None = ...): ...
def apply_lcwd(path, env): ...
