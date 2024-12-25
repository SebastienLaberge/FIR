"""
"""

from os import makedirs
from os.path import isdir, split
from typing import Sequence


def read_interfile(
        file_path: str,
        file_type: str) -> dict:

    file_type = file_type.upper()

    with open(file_path, 'r') as fd:

        lines = fd.readlines()

        def keep_line(line):

            stripped_line = line.strip()

            return stripped_line != "" and \
                stripped_line[0] != ';'

        def split_line(line):

            if len(pair := line.split(':=')) != 2:

                raise ValueError('Invalid interfile header')

            stripped_pair = [el.strip() for el in pair]

            if stripped_pair[0] == "":

                raise ValueError('Invalid interfile header')

            return stripped_pair

        pairs = [split_line(line)
                 for line in lines
                 if keep_line(line)]

        if len(pairs) == 0:

            raise ValueError('Interfile header is empty')

        if pairs[0][0] != file_type:

            raise ValueError(
                f'Interfile header is not of type {file_type}')

        if pairs[-1][0] != 'END OF ' + file_type:

            raise ValueError('Invalid interfile header')

        parsed_lines = dict(pairs[1:-1])

    return parsed_lines


def write_interfile(
        file_path: str,
        file_type: str,
        lines: Sequence[tuple[str, str]]):

    file_dir, _ = split(file_path)

    if file_dir and not isdir(file_dir):

        makedirs(file_dir)

    with open(file_path, 'w') as fd:

        processed_lines = map(
            lambda el: el[0] + ' := ' + el[1] + '\n'
            if el is not None
            else '\n',
            lines)

        fd.writelines(
            [file_type + " :=\n"] +
            list(processed_lines) +
            ["END OF " + file_type + " :=\n"])
