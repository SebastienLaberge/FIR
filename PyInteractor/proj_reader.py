"""
Defines class ProjReader, which can be used to read projection
data and extract sinograms and michelograms.
"""

from os.path import dirname, join

import numpy as np

from FIR.PyInteractor.proj import ProjHeader


class ProjReader:

    def __init__(self, proj_file: str):

        self.header = ProjHeader.read(proj_file)

        (self.n_views,
         self.n_axial_coords,
         self.n_bins,
         self.seg_offset,
         self.tang_coord_offset) = \
            self.header.get_derived_params()

        proj_dir = dirname(proj_file)
        proj_data_file = \
            join(proj_dir, self.header.data_file_name)

        dtype = np.dtype('float32')

        self.data = np.fromfile(proj_data_file, dtype=dtype)

        assert self.data.size == self.n_bins

    def get_n_axial_coords(self, segment):

        assert -self.seg_offset <= segment <= self.seg_offset

        return self.n_axial_coords[segment+self.seg_offset]

    def get_viewgram(self, segment: int, view: int):

        if segment < -self.seg_offset or \
           segment > self.seg_offset:

            raise ValueError("Invalid segment offset")

        offset_segment = segment + self.seg_offset

        if view < 0 or view >= self.n_views:

            raise ValueError("Invalid view")

        n_axial_coords = self.n_axial_coords[offset_segment]

        offset = 0
        for current_segment in range(offset_segment):

            offset += \
                self.n_views * \
                self.n_axial_coords[current_segment] * \
                self.header.n_tang_coords

        offset += view * n_axial_coords * \
            self.header.n_tang_coords

        shape = (n_axial_coords,
                 self.header.n_tang_coords)

        size = shape[0] * shape[1]

        viewgram = \
            self.data[offset:offset+size].reshape(shape).T

        viewgram = np.flipud(viewgram)

        return viewgram

    def get_sinogram(self, segment: int, axial_coord: int):

        if segment > self.seg_offset or \
           segment < -self.seg_offset:

            raise ValueError("Invalid segment offset")

        offset_segment = segment + self.seg_offset

        n_axial_coords = \
            self.n_axial_coords[offset_segment]

        if axial_coord < 0 or \
           axial_coord >= n_axial_coords:

            raise ValueError("Invalid axial coordinate")

        offset = 0
        for current_segment in range(offset_segment):

            offset += \
                self.n_views * \
                self.n_axial_coords[current_segment] * \
                self.header.n_tang_coords

        def get_view_line(view):

            extra_offset = \
                (view * n_axial_coords + axial_coord) * \
                self.header.n_tang_coords
            total_offset = offset + extra_offset
            size = self.header.n_tang_coords

            return self.data[total_offset:total_offset+size]

        sinogram = \
            np.stack(
                [get_view_line(view)
                 for view in range(self.n_views)]).T

        sinogram = np.flipud(sinogram)

        return sinogram
