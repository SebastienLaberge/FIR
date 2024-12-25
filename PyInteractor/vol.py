"""
"""

from dataclasses import dataclass
from typing import Optional, Self

from FIR.PyInteractor.interfile_io import \
    read_interfile, write_interfile

Size = int
Size3D = tuple[Size, Size, Size]

SpatialExtent = float
SpatialExtent3D = \
    tuple[SpatialExtent, SpatialExtent, SpatialExtent]

SpatialCoord = SpatialExtent
SpatialCoords3D = tuple[SpatialCoord, SpatialCoord, SpatialCoord]


@dataclass
class VolHeader:

    data_file_name: Optional[str]  # Set to None for templates
    number_format: str
    n_bytes_per_pixel: int

    vol_size: Size3D
    voxel_extent: SpatialExtent3D
    vol_offset: Optional[SpatialCoords3D]  # Default: Centered
    n_frames: Size  # Implied to be 1 for templates

    def __post_init__(self):

        # Throw if data is malformed and set defaults
        self.check()

    @classmethod
    def read(cls,
             vol_file: str,
             read_data_file: bool = True) -> Self:
        """
        Read interfile header specified by vol_file.
        Set read_data_file to False to ommit reading name of
        data file and get a template for the volume.
        """

        header = read_interfile(vol_file, 'INTERFILE')

        data_file_name = header.get('name of data file') \
            if read_data_file else None

        # Templates only contain one frame
        n_time_frames = \
            header['number of time frames'] \
            if read_data_file else 1

        return cls(
            data_file_name,
            header['number format'],
            int(header['number of bytes per pixel']),

            (Size(header['matrix size [1]']),
             Size(header['matrix size [2]']),
             Size(header['matrix size [3]'])),

            (SpatialExtent(
                header['scaling factor (mm/pixel) [1]']),
             SpatialExtent(
                header['scaling factor (mm/pixel) [2]']),
             SpatialExtent(
                header['scaling factor (mm/pixel) [3]'])),

            (SpatialCoord(
                header['first pixel offset (mm) [1]']),
             SpatialCoord(
                header['first pixel offset (mm) [2]']),
             SpatialCoord(
                header['first pixel offset (mm) [3]'])),

            Size(n_time_frames))

    def write(self, vol_file: str):

        file_type = 'INTERFILE'

        lines = [
            # Name of data file goes here if present

            ('data offset in bytes',
             '0'),
            ('number format',
             f'{self.number_format}'),
            ('number of bytes per pixel',
             f'{self.n_bytes_per_pixel}'),
            ('imagedata byte order',
             'LITTLEENDIAN'),
            None,
            ('number of dimensions',
             '3'),
            None,
            ('matrix size [1]',
             f'{self.vol_size[0]}'),
            ('matrix size [2]',
             f'{self.vol_size[1]}'),
            ('matrix size [3]',
             f'{self.vol_size[2]}'),
            ('number of slices',
             f'{self.vol_size[2]}'),
            None,
            ('scaling factor (mm/pixel) [1]',
             f'{self.voxel_extent[0]:.7f}'),
            ('scaling factor (mm/pixel) [2]',
             f'{self.voxel_extent[1]:.7f}'),
            ('scaling factor (mm/pixel) [3]',
             f'{self.voxel_extent[2]:.7f}'),
            ('slice thickness (pixels)',
             f'{self.voxel_extent[2]:.7f}'),
            None,
            ('first pixel offset (mm) [1]',
             f'{self.vol_offset[0]:.7f}'),
            ('first pixel offset (mm) [2]',
             f'{self.vol_offset[1]:.7f}'),
            ('first pixel offset (mm) [3]',
             f'{self.vol_offset[2]:.7f}'),
            None,
            ('number of time frames',
             f'{self.n_frames}'),
        ]

        if self.data_file_name is not None:

            lines.insert(
                0,
                ('name of data file',
                 f'{self.data_file_name}'))

        write_interfile(vol_file, file_type, lines)

    def check(self):

        def some_zero_or_negative(array):

            return any(map(lambda x: x <= 0, array))

        if some_zero_or_negative(self.vol_size):

            raise ValueError(
                "Number of voxels must be greater than zero in "
                "each dimension")

        if some_zero_or_negative(self.voxel_extent):

            raise ValueError(
                "Voxel extent must be greater than zero in "
                "each dimension")

        # Default value for volume offset
        if self.vol_offset is None:
            self.vol_offset = (
                -(self.vol_size[0] - 1) *
                self.voxel_extent[0] / 2,
                -(self.vol_size[1] - 1) *
                self.voxel_extent[1] / 2,
                0.0)

    def get_derived_params(self):

        vol_extent: SpatialExtent3D = (
            self.vol_size[0] * self.voxel_extent[0],
            self.vol_size[1] * self.voxel_extent[1],
            self.vol_size[2] * self.voxel_extent[2])

        n_voxels_per_frame: Size = self.vol_size[0] * \
            self.vol_size[1] * \
            self.vol_size[2]

        n_voxels_total: Size = \
            n_voxels_per_frame * self.n_frames

        return (vol_extent,
                n_voxels_per_frame,
                n_voxels_total)

    def __repr__(self):

        (vol_extent,
         n_voxels_per_frame,
         n_voxels_total) = self.get_derived_params()

        return f"""
= Volume header:

== Voxel data:
data file name: {self.data_file_name}
number format: {self.number_format} with \
{self.n_bytes_per_pixel} bytes per voxel

== Volume dimensions (width in x, height in y, depth in z):
volume size in voxels: {self.vol_size}
voxel extent in mm: {self.voxel_extent}
volume extent in mm: {vol_extent}

== Volume position:
x,y coordinates of first voxel in mm: {self.vol_offset[0:2]}
volume offset in z in mm: {self.vol_offset[2]}

== Number of frames and voxels:
number of time frames: {self.n_frames}
number of voxels per frame: {n_voxels_per_frame}
total number of voxels: {n_voxels_total}
"""
