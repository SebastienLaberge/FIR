"""
"""

from dataclasses import dataclass
from os.path import join
from typing import Optional, Self

from FIR.PyInteractor.interfile_io import \
    read_interfile, write_interfile

DEFAULT_SPAN = 1
DEFAULT_N_SEGMENTS = 1


@dataclass
class ProjHeader:

    data_file_name: Optional[str]  # Set to None for templates

    # Scanner basic geometry (mandatory)
    # Must match the values from the associated scanner object
    n_rings: int
    n_crystals_per_ring: int

    # Michelogram compression
    segment_span: int = DEFAULT_SPAN

    # Projection dimensions
    n_segments: int = DEFAULT_N_SEGMENTS
    n_tang_coords: int = 0  # If 0, replace with maximum value

    def __post_init__(self):

        # Throw if data is malformed and set defaults
        self.check()

    # set read_data_file to False to get a template for the
    # projection read
    @classmethod
    def read(cls,
             vol_file: str,
             read_data_file: bool = True) -> Self:

        header = \
            read_interfile(
                vol_file,
                'PROJECTION DATA PARAMETERS')

        data_file_name = \
            header.get('name of data file') \
            if read_data_file else None

        vol_header = cls(
            data_file_name,
            # Mandatory: Zeros values trigger exception later
            int(header.get('number of rings', 0)),
            int(header.get('number of crystals per ring', 0)),
            # If absent: Preset default values
            int(header.get('segment span', DEFAULT_SPAN)),
            int(header.get(
                'number of segments',
                DEFAULT_N_SEGMENTS)),
            # If absent: Default value to be computed later
            int(header.get(
                'number of tangential coordinates', 0)))

        return vol_header

    def write(self, save_dir: str, filename: str):

        file_type = 'PROJECTION DATA PARAMETERS'

        lines = [
            ('number of rings',
             f'{self.n_rings}'),
            ('number of crystals per ring',
             f'{self.n_crystals_per_ring}'),
            ('segment span',
             f'{self.segment_span}'),
            ('number of segments',
             f'{self.n_segments}'),
            ('number of tangential coordinates',
             f'{self.n_tang_coords}')
        ]

        write_interfile(
            join(save_dir, filename),
            file_type,
            lines)

    def check(self):

        # Number of rings

        if self.n_rings <= 0:

            raise ValueError(
                '"number of rings" must be provided and '
                'greater than zero')

        # Number of crystals per ring

        if self.n_crystals_per_ring <= 0:

            raise ValueError(
                '"number of crystals per ring" must be '
                'provided and greater than zero')

        if self.n_crystals_per_ring % 4 != 0:

            raise ValueError(
                '"number of crystals per ring" must be a '
                'multiple of 4')

        # Segment span

        if self.segment_span <= 0:

            raise ValueError(
                '"segment span" must be greater than zero')

        if self.segment_span % 2 != 1:

            raise ValueError(
                '"segment span" must be an odd number')

        if self.segment_span > 2 * self.n_rings - 1:

            raise ValueError(
                f'For {self.n_rings} rings, "segment span" '
                f'must not be greater than {2*self.n_rings-1}')

        # Number of segments

        # Maximum number of segments allowed
        max_n_segments = \
            (2 * self.n_rings - 1) // self.segment_span

        # Decrement by 1 if it's even to make it odd
        max_n_segments -= 1 - max_n_segments % 2

        if self.n_segments < 0:

            raise ValueError(
                '"number of segments" must not be negative')

        if self.n_segments % 2 != 1:

            raise ValueError(
                '"number of segments" must be an odd number')

        if self.n_segments > max_n_segments:

            raise ValueError(
                f'For {self.n_rings} rings and a segment span '
                f'of {self.segment_span}, "number of segments" '
                f'must not be greater than {max_n_segments}')

        # Number of tangential coordinates

        # Maximum number of tangential coordinates allowed
        max_n_tang_coords = self.n_crystals_per_ring - 1

        if self.n_tang_coords == 0:

            # Default value: Largest possible value
            self.n_tang_coords = max_n_tang_coords

        elif self.n_tang_coords < 0:

            raise ValueError(
                '"number of tangential coordinates" must not '
                'be negative')

        elif self.n_tang_coords > max_n_tang_coords:

            raise ValueError(
                f'For {self.n_crystals_per_ring} crystals per '
                f'ring, "number of tangential coordinates" '
                f'must not be greater than {max_n_tang_coords}')

    def get_derived_params(self):

        # Offsets for bin coordinates that can be negative
        seg_offset = (self.n_segments - 1) // 2
        tang_coord_offset = (self.n_tang_coords + 2) // 2 - 1

        # Remaining projection dimensions
        n_views = self.n_crystals_per_ring // 2
        n_axial_coords = [0] * self.n_segments
        if self.segment_span == 1:
            # For a span of 1, the segment length reduces by
            # one for each segment away from the center
            # segment, which has the same length as the number
            # of rings
            for seg in range(-seg_offset, seg_offset + 1):
                n_axial_coords[seg+seg_offset] = \
                    self.n_rings - abs(seg)
        else:
            # Length of middle segment
            middle_segment_length = 2 * self.n_rings - 1

            for seg in range(-seg_offset, seg_offset + 1):

                abs_seg = abs(seg)

                # Starting from middle segment
                segment_length = middle_segment_length

                if abs_seg >= 1:
                    # Length reduction when leaving middle
                    # segment; Derived from: 2*((span-1)/2+1)
                    segment_length -= self.segment_span + 1

                    if abs_seg >= 2:
                        # Length reduction for each subsequent
                        # displacement away from middle segment
                        segment_length -= \
                            2 * self.segment_span * \
                            (abs_seg - 1)

                n_axial_coords[seg+seg_offset] = segment_length

        n_bins = 0
        for seg in range(-seg_offset, seg_offset+1):
            n_bins += \
                n_views * \
                n_axial_coords[seg+seg_offset] * \
                self.n_tang_coords

        return (n_views,
                n_axial_coords,
                n_bins,
                seg_offset,
                tang_coord_offset)

    def __repr__(self):

        (n_views,
         n_axial_coords,
         n_bins,
         seg_offset,
         tang_coord_offset) = \
            self.get_derived_params()

        return f"""
= Projection header:

== Scanner basic geometry:
number of rings: {self.n_rings}
number of crystals per ring: {self.n_crystals_per_ring}

== Michelogram compression:
segment span: {self.segment_span}

== Projection dimensions:
number of segments: {self.n_segments}
number of views: {n_views}
number of axial coords: {n_axial_coords}
number of tangential coordinates: {self.n_tang_coords}

== Others:
total number of bins: {n_bins}
segment offset: {seg_offset}
tangential coordinate offset: {tang_coord_offset}
"""
