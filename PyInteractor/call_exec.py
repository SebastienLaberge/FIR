"""
Defines function call_exec, which calls a given executable from
a given directory with given arguments
"""

from os import chdir, getcwd
from os.path import join
import subprocess
from typing import Optional, Sequence


def call_exec(
        work_dir: str,
        exec_dir: str,
        exec_name: str,
        arguments: Optional[Sequence[str]] = None):

    exec_path = join(exec_dir, exec_name)

    full_command = [exec_path]
    if arguments is not None:
        full_command += arguments
    full_command_str = ' '.join(full_command)

    print(f'Calling command:\n'
          f'\t"{full_command_str}"\n'
          f'from dir:\n'
          f'\t"{work_dir}"')

    @_invoke_at(work_dir)
    def call_exec_from_work_dir():

        subprocess.call(full_command)

    call_exec_from_work_dir()


def _invoke_at(path: str):

    def parameterized(func):

        def wrapper(*args, **kwargs):

            cwd = getcwd()
            chdir(path)

            try:
                ret = func(*args, **kwargs)
            finally:
                chdir(cwd)

            return ret

        return wrapper

    return parameterized
