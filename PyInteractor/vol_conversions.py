"""
Conversions between numpy arrays and interfile voxelized volumes
"""

from os.path import join, split, splitext, dirname
from typing import Sequence, Union

import numpy as np

from FIR.PyInteractor.vol import VolHeader


Shape = Union[tuple[int, int, int], tuple[int, int, int, int]]


def read_interfile_volume(header_file_path: str):

    vol_header = VolHeader.read(header_file_path)

    file_dir = dirname(header_file_path)
    data_file_path = join(file_dir, vol_header.data_file_name)

    number_format = (vol_header.number_format,
                     vol_header.n_bytes_per_pixel)

    match number_format:

        case ('UNSIGNED INTEGER', 1):
            pixel_type = 'uint8'

        case ('FLOAT', 4):
            pixel_type = 'float32'

        case _:
            raise ValueError(
                f'Unsupported voxel type {number_format}')

    shape = [vol_header.vol_size[2],
             vol_header.vol_size[1],
             vol_header.vol_size[0]]

    if vol_header.n_frames > 1:

        shape.insert(0, vol_header.n_frames)

    vol_data = \
        _read_volume_data(data_file_path, pixel_type, shape)

    return vol_header, vol_data


def _read_volume_data(
        vol_data_file: str,
        voxel_type: str,
        shape: Shape) -> np.ndarray:

    dtype = np.dtype(voxel_type)

    vol_array = \
        np.fromfile(vol_data_file, dtype=dtype).reshape(shape)

    return vol_array


def read_interfile_frames_as_array(
        frame_header_files: Sequence[str]):

    return np.stack(
        [read_interfile_volume(header_file)[1]
         for header_file in frame_header_files])


def write_interfile_volume(
        vol_array: np.ndarray,
        output_file_path: str,
        voxel_type: str,
        voxel_size: tuple,
        vol_offset: tuple = None):

    match vol_array.ndim:

        case 3:
            n_slices, size_y, size_x = \
                vol_array.shape
            n_frames = 1

        case 4:
            n_frames, n_slices, size_y, size_x = \
                vol_array.shape

        case _:
            raise ValueError('Wrong number of dimensions in '
                             'volume array shape')

    match voxel_type:

        case 'uint8':
            number_format = 'UNSIGNED INTEGER'
            n_bytes_per_pixel = 1

        case 'float32':
            number_format = 'FLOAT'
            n_bytes_per_pixel = 4

        case _:
            raise ValueError(
                f'Unsupported voxel type {voxel_type}')

    # Extract voxel size elements
    pixel_width_x, pixel_width_y, slice_spacing = voxel_size

    # Get file path components
    file_dir, file_base = split(output_file_path)
    file_stem = splitext(file_base)[0]

    # Get bases for header and data files
    header_file_base = file_stem + '.h33'
    data_file_base = file_stem + '.i33'

    # Get paths for header and data files
    header_file_path = join(file_dir, header_file_base)
    data_file_path = join(file_dir, data_file_base)

    # Create header object for volume
    vol_header = VolHeader(
        data_file_base,
        number_format,
        n_bytes_per_pixel,
        (size_x, size_y, n_slices),
        (pixel_width_x, pixel_width_y, slice_spacing),
        vol_offset,
        n_frames)

    # Write volume header
    vol_header.write(header_file_path)

    # Write volume data
    _write_volume_data(vol_array, data_file_path, voxel_type)


def _write_volume_data(
        array: np.ndarray,
        vol_data_file: str,
        voxel_type: str):

    array.astype(voxel_type).tofile(vol_data_file)
