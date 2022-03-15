from fabric.api import abort as abort, env as env, hide as hide, put as put, run as run, settings as settings, sudo as sudo
from fabric.utils import apply_lcwd as apply_lcwd
from typing import Any

def exists(path, use_sudo: bool = ..., verbose: bool = ...): ...
def is_link(path, use_sudo: bool = ..., verbose: bool = ...): ...
def first(*args, **kwargs): ...
def upload_template(filename, destination, context: Any | None = ..., use_jinja: bool = ..., template_dir: Any | None = ..., use_sudo: bool = ..., backup: bool = ..., mirror_local_mode: bool = ..., mode: Any | None = ..., pty: Any | None = ..., keep_trailing_newline: bool = ..., temp_dir: str = ...): ...
def sed(filename, before, after, limit: str = ..., use_sudo: bool = ..., backup: str = ..., flags: str = ..., shell: bool = ...): ...
def uncomment(filename, regex, use_sudo: bool = ..., char: str = ..., backup: str = ..., shell: bool = ...): ...
def comment(filename, regex, use_sudo: bool = ..., char: str = ..., backup: str = ..., shell: bool = ...): ...
def contains(filename, text, exact: bool = ..., use_sudo: bool = ..., escape: bool = ..., shell: bool = ..., case_sensitive: bool = ...): ...
def append(filename, text, use_sudo: bool = ..., partial: bool = ..., escape: bool = ..., shell: bool = ...) -> None: ...
