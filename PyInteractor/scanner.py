"""
"""

from dataclasses import dataclass
from os.path import join
from typing import Self

from FIR.PyInteractor.interfile_io import \
    read_interfile, write_interfile

RepeatNumber = int
RepeatNumbers2D = list[RepeatNumber, RepeatNumber]

SpatialExtent = float
SpatialExtent2D = list[SpatialExtent, SpatialExtent]
SpatialExtent3D = \
    list[SpatialExtent, SpatialExtent, SpatialExtent]


# TODO: Give better error messages when number of arguments or
# argument type is wrong

def _to_str(numbers):

    return '{' + ', '.join([str(n) for n in numbers]) + '}'\
        if numbers is not None else ""


def _from_str(
        list_str,
        constructor,
        expected_length,
        default_value):

    if len(list_str) == 0:
        return [constructor(default_value)] * expected_length

    start, finish = list_str.find('{') + 1, list_str.find('}')
    elements = list_str[start:finish].split(',')

    if len(elements) != expected_length:
        raise ValueError('Wrong number of elements in list')

    return list(map(constructor, elements))


@dataclass
class ScannerHeader():

    name: str

    crystal_dimensions: SpatialExtent3D
    crystal_repeat_numbers: RepeatNumbers2D
    inter_crystal_distance: SpatialExtent2D

    module_dimensions: SpatialExtent3D
    module_repeat_numbers: RepeatNumbers2D
    inter_module_distance: SpatialExtent2D

    rsector_dimensions: SpatialExtent3D
    rsector_repeat_number: RepeatNumber
    rsector_inner_radius: SpatialExtent

    def __post_init__(self):

        # Throw if data is malformed
        self.check()

    @classmethod
    def read(cls,
             scanner_file: str) -> Self:
        """
        Read interfile scanner specified by scanner file.
        """

        header = \
            read_interfile(scanner_file, 'SCANNER PARAMETERS')

        return cls(
            None,
            _from_str(
                header.get('crystal dimensions XYZ in mm', ""),
                SpatialExtent, 3, 0.0),
            _from_str(
                header.get('crystal repeat numbers YZ', ""),
                RepeatNumber, 2, 1),
            _from_str(
                header.get('inter-crystal distance YZ in mm', ""),
                SpatialExtent, 2, 0.0),
            # If set to 0: Get min value
            _from_str(
                header.get('module dimensions XYZ in mm', ""),
                SpatialExtent, 3, 0.0),
            _from_str(
                header.get('module repeat numbers YZ', ""),
                RepeatNumber, 2, 1),
            _from_str(
                header.get('inter-module distance YZ in mm', ""),
                SpatialExtent, 2, 0.0),
            # If set to 0: Get min value
            _from_str(
                header.get('rSector dimensions XYZ in mm', ""),
                SpatialExtent, 3, 0.0),
            # Mandatory
            RepeatNumber(header['rSector repeat number']),
            # Mandatory
            SpatialExtent(header['rSector inner radius in mm']))

    def write(self, save_dir: str, filename: str):

        crystal_dimensions = \
            _to_str(self.crystal_dimensions)
        crystal_repeat_numbers = \
            _to_str(self.crystal_repeat_numbers)
        inter_crystal_distance = \
            _to_str(self.inter_crystal_distance)

        module_dimensions = \
            _to_str(self.module_dimensions)
        module_repeat_numbers = \
            _to_str(self.module_repeat_numbers)
        inter_module_distance = \
            _to_str(self.inter_module_distance)

        rsector_dimensions = _to_str(self.rsector_dimensions)
        rsector_repeat_number = str(self.rsector_repeat_number)
        rsector_inner_radius = str(self.rsector_inner_radius)

        file_type = 'SCANNER PARAMETERS'

        lines = [
            ('crystal dimensions XYZ in mm',
             f'{crystal_dimensions}'),
            ('crystal repeat numbers YZ',
             f'{crystal_repeat_numbers}'),
            ('inter-crystal distance YZ in mm',
             f'{inter_crystal_distance}'),
            None,
            ('module dimensions XYZ in mm',
             f'{module_dimensions}'),
            ('module repeat numbers YZ',
             f'{module_repeat_numbers}'),
            ('inter-module distance YZ in mm',
             f'{inter_module_distance}'),
            None,
            ('rSector dimensions XYZ in mm',
             f'{rsector_dimensions}'),
            ('rSector repeat number',
             f'{rsector_repeat_number}'),
            ('rSector inner radius in mm',
             f'{rsector_inner_radius}'),
        ]

        write_interfile(
            join(save_dir, filename),
            file_type,
            lines)

    def check(self):

        def some_negative(array):

            return any(map(lambda x: x < 0, array))

        def some_zero_or_negative(array):

            return any(map(lambda x: x <= 0, array))

        # Check range of values

        if some_negative(self.crystal_dimensions):

            raise ValueError(
                'Elements of "crystal dimensions XYZ in mm" '
                'must not be negative')

        if some_zero_or_negative(self.crystal_repeat_numbers):

            raise ValueError(
                'Elements of "crystal repeat numbers YZ" '
                'must be greater than zero')

        if some_negative(self.inter_crystal_distance):

            raise ValueError(
                'Elements of "inter-crystal distance YZ in mm" '
                'must not be negative')

        if some_negative(self.module_dimensions):

            raise ValueError(
                'Elements of "module dimensions XYZ in mm" '
                'must not be negative')

        if some_zero_or_negative(self.module_repeat_numbers):

            raise ValueError(
                'Elements of "module repeat numbers YZ" must '
                'be greater than zero')

        if some_negative(self.inter_module_distance):

            raise ValueError(
                'Elements of "inter-module distance YZ in mm" '
                'must not be negative')

        if some_negative(self.rsector_dimensions):

            raise ValueError(
                'Elements of "rSector dimensions XYZ in mm" '
                'must not be negative')

        if self.rsector_repeat_number <= 0:

            raise ValueError(
                '"rSector repeat number" must be greater than '
                'zero')

        if self.rsector_inner_radius <= 0:

            raise ValueError(
                '"rSector inner radius in mm\" must be greater '
                'than zero')

        # For dimensions Y and Z, check that crystals have
        # non-zero dimension if crystal repeat number is
        # greater than 1

        if self.crystal_repeat_numbers[0] > 1 and \
           self.crystal_dimensions[1] == 0:

            raise ValueError(
                "Crystal repeat number in Y cannot be greater "
                "than 1 if crystal dimension in Y is zero")

        if self.crystal_repeat_numbers[1] > 1 and \
           self.crystal_dimensions[2] == 0:

            raise ValueError(
                "Crystal repeat number in Z cannot be greater "
                "than 1 if crystal dimension in Z is zero")

        # Default and minimum module size:
        # Fits tightly on crystals

        min_module_dimension_x = self.crystal_dimensions[0]

        if self.module_dimensions[0] == 0.0:

            self.module_dimensions[0] = min_module_dimension_x

        elif self.module_dimensions[0] < min_module_dimension_x:

            raise ValueError(
                f'Module dimension in X must be greater than '
                f'or equal to {min_module_dimension_x}')

        min_module_dimension_y = \
            self.crystal_dimensions[1] * \
            self.crystal_repeat_numbers[0] + \
            self.inter_crystal_distance[0] * \
            (self.crystal_repeat_numbers[0] - 1)

        if self.module_dimensions[1] == 0.0:

            self.module_dimensions[1] = min_module_dimension_y

        elif self.module_dimensions[1] < min_module_dimension_y:

            raise ValueError(
                f'Module dimension in Y must be greater than '
                f'or equal to {min_module_dimension_y}')

        min_module_dimension_z = \
            self.crystal_dimensions[2] * \
            self.crystal_repeat_numbers[1] + \
            self.inter_crystal_distance[1] * \
            (self.crystal_repeat_numbers[1] - 1)

        if self.module_dimensions[2] == 0.0:

            self.module_dimensions[2] = min_module_dimension_z

        elif self.module_dimensions[2] < min_module_dimension_z:

            raise ValueError(
                f'Module dimension in Z must be greater than '
                f'or equal to {min_module_dimension_z}')

        # For dimensions Y and Z, check that modules have
        # non-zero dimension if module repeat number is
        # greater than 1

        if self.module_repeat_numbers[0] > 1 and \
           self.module_dimensions[1] == 0:

            raise ValueError(
                "Module repeat number in Y cannot be greater "
                "than 1 if module dimension in Y is zero")

        if self.module_repeat_numbers[1] > 1 and \
           self.module_dimensions[2] == 0:

            raise ValueError(
                "Module repeat number in Z cannot be greater "
                "than 1 if module dimension in Z is zero")

        # Default and minimum rSector size:
        # Fits tightly on modules

        min_rsector_dimension_x = self.module_dimensions[0]

        if self.rsector_dimensions[0] == 0:

            self.rsector_dimensions[0] = min_rsector_dimension_x

        elif self.rsector_dimensions[0] < \
                min_rsector_dimension_x:

            raise ValueError(
                f'rSector dimension in X must be greater than '
                f'or equal to {min_rsector_dimension_x}')

        min_rsector_dimension_y = \
            self.module_dimensions[1] * \
            self.module_repeat_numbers[0] + \
            self.inter_module_distance[0] * \
            (self.module_repeat_numbers[0] - 1)

        if self.rsector_dimensions[1] == 0:

            self.rsector_dimensions[1] = \
                min_rsector_dimension_y

        elif self.rsector_dimensions[1] < \
                min_rsector_dimension_y:

            raise ValueError(
                f'rSector dimension in Y must be greater than '
                f'or equal to {min_rsector_dimension_y}')

        min_rsector_dimension_z = \
            self.module_dimensions[2] * \
            self.module_repeat_numbers[1] + \
            self.inter_module_distance[1] * \
            (self.module_repeat_numbers[1] - 1)

        if self.rsector_dimensions[2] == 0:

            self.rsector_dimensions[2] = \
                min_rsector_dimension_z

        elif self.rsector_dimensions[2] < \
                min_rsector_dimension_z:

            raise ValueError(
                f'rSector dimension in Z must be greater than '
                f'or equal to {min_rsector_dimension_z}')

    def get_derived_params(self):

        # Crystal repeat vector
        crystal_repeat_vector = [
            self.crystal_dimensions[1] +
            self.inter_crystal_distance[0],
            self.crystal_dimensions[2] +
            self.inter_crystal_distance[1]]

        # Module repeat vector
        module_repeat_vector = [
            self.module_dimensions[1] +
            self.inter_module_distance[0],
            self.module_dimensions[2] +
            self.inter_module_distance[1]]

        # Translation of reference rSector from origin along x
        rsector_translation = \
            self.rsector_inner_radius + \
            self.rsector_dimensions[0] / 2

        # Numbers
        n_rings = \
            self.crystal_repeat_numbers[1] * \
            self.module_repeat_numbers[1]
        n_crystals_per_ring = \
            self.crystal_repeat_numbers[0] * \
            self.module_repeat_numbers[0] * \
            self.rsector_repeat_number
        n_crystals = n_rings * n_crystals_per_ring

        # Number of slices
        n_slices = 2 * n_rings - 1

        # Crystal offset
        crystal_offset = \
            self.module_repeat_numbers[0] * \
            self.crystal_repeat_numbers[0] // 2

        return [crystal_repeat_vector,
                module_repeat_vector,
                rsector_translation,
                n_rings,
                n_crystals_per_ring,
                n_crystals,
                n_slices,
                crystal_offset]

    def __repr__(self):

        [crystal_repeat_vector,
         module_repeat_vector,
         rsector_translation,
         n_rings,
         n_crystals_per_ring,
         n_crystals,
         n_slices,
         crystal_offset] = self.get_derived_params()

        return f"""
= Scanner header:

== Crystals:
crystal dimensions XYZ in mm: {self.crystal_dimensions}
crystal repeat numbers YZ: {self.crystal_repeat_numbers}
inter-crystal distance YZ in mm: {self.inter_crystal_distance}

== Modules:
module dimensions XYZ in mm: {self.module_dimensions}
module repeat numbers YZ: {self.module_repeat_numbers}
inter-module distance YZ in mm: {self.inter_module_distance}

== rSectors:
rSector dimensions XYZ in mm: {self.rsector_dimensions}
rSector repeat number: {self.rsector_repeat_number}
rSector inner radius in mm: {self.rsector_inner_radius}

== Translations:
crystal repeat vector YZ in mm: {crystal_repeat_vector}
module repeat vector YZ in mm: {module_repeat_vector}
rSector translation in X in mm: {rsector_translation}

== Numbers:
number of rings: {n_rings}
number of crystals per ring: {n_crystals_per_ring}
number of crystals: {n_crystals}
number of slices: {n_slices}
crystal offset: {crystal_offset}
"""
