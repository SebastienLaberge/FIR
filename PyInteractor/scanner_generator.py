"""
"""

import math

import numpy as np

from FIR.PyInteractor.scanner import ScannerHeader


def rotate_xy(x_in, y_in, angle):

    if angle == 0:
        return x_in, y_in

    cos_angle = math.cos(angle)
    sin_angle = math.sin(angle)

    x_out = x_in * cos_angle - y_in * sin_angle
    y_out = x_in * sin_angle + y_in * cos_angle

    return x_out, y_out


def compute_crystal_xy_position_vector(scanner: ScannerHeader):

    [crystal_repeat_vector,
     module_repeat_vector,
     rsector_x_translation,
     _,
     n_crystals_per_ring,
     _,
     _,
     crystal_offset] = scanner.get_derived_params()

    # Parameters for the crystal positions in Y within the first
    # r-sector, which is vertical and repeated rotationally

    n_crystals_y = scanner.crystal_repeat_numbers[0]
    crystal_repeat_vector_y = crystal_repeat_vector[0]

    n_modules_y = scanner.module_repeat_numbers[0]
    module_repeat_vector_y = module_repeat_vector[0]

    # Y position of the first crystal of the first r-sector:
    # Half the distance between the first and the last crystal
    # of the first r-sector, on the negative Y side
    first_crystal_coord_y = \
        -((n_modules_y - 1) * module_repeat_vector_y +
          (n_crystals_y - 1) * crystal_repeat_vector_y) / 2.0

    # Position in Y of each crystal in the first r-sector
    rsector_n_crystals_y = n_modules_y * n_crystals_y
    rsector_y = [None] * rsector_n_crystals_y
    for module_index in range(n_modules_y):
        for crystal_index in range(n_crystals_y):
            ind = module_index * n_crystals_y + crystal_index
            rsector_y[ind] = \
                module_index * module_repeat_vector_y + \
                crystal_index * crystal_repeat_vector_y + \
                first_crystal_coord_y

    # Angular distance between r-sectors
    angle_interval = 2 * math.pi / scanner.rsector_repeat_number

    # Index of the first crystal of the first r-sector:
    # Such that the crystal with index 0 is the middle crystal
    # of the first r-sector that is on the X axis or close to it
    first_crystal_index = n_crystals_per_ring - crystal_offset

    corner_offset_xy = [
        scanner.crystal_dimensions[0] *
        np.array([-0.5, 0.5, 0.5, -0.5, -0.5]),
        scanner.crystal_dimensions[1] *
        np.array([0.5, 0.5, -0.5, -0.5, 0.5])]

    # Crystal positions in XY for an entire ring
    crystal_xy_positions = \
        np.zeros([n_crystals_per_ring, 2])

    # Crystal corner positions in XY for an entire ring
    crystal_y_corners = \
        np.zeros([n_crystals_per_ring, 5, 2])

    def update(x, y, angle):

        x_rot, y_rot = rotate_xy(x, y, angle)

        crystal_xy_positions[crystal_index, :] = \
            [x_rot, y_rot]

        corner_x = x + corner_offset_xy[0]
        corner_y = y + corner_offset_xy[1]

        corner_x_rot, corner_y_rot = \
            rotate_xy(corner_x, corner_y, angle)

        crystal_y_corners[crystal_index, :, :] = \
            np.array([corner_x_rot, corner_y_rot]).T

    # Find the position of each crystal from the middle crystal
    # of the first r-sector (index 0) up to but excluding the
    # first crystal of the first r-sector (first_crystal_index)
    for crystal_index in range(first_crystal_index):

        crystal_index_with_offset = \
            crystal_index + crystal_offset

        x = rsector_x_translation
        y = rsector_y[crystal_index_with_offset %
                      rsector_n_crystals_y]

        angle = \
            (crystal_index_with_offset //
             rsector_n_crystals_y) * angle_interval

        update(x, y, angle)

    # Complete the first half of the first r-sector
    for crystal_index in \
            range(first_crystal_index, n_crystals_per_ring):

        x = rsector_x_translation
        y = rsector_y[crystal_index - first_crystal_index]

        update(x, y, 0)

    return crystal_xy_positions, crystal_y_corners


def compute_slice_z_position_vector(scanner: ScannerHeader):

    [crystal_repeat_vector,
     module_repeat_vector,
     rsector_x_translation,
     _,
     _,
     _,
     n_slices,
     _] = scanner.get_derived_params()

    # Parameters for crystal positions in Z within each r-sector

    n_crystals_z = scanner.crystal_repeat_numbers[1]
    crystal_repeat_vector_z = \
        crystal_repeat_vector[1]

    n_modules_z = scanner.module_repeat_numbers[1]
    module_repeat_vector_z = \
        module_repeat_vector[1]

    # Z position of the first ring (and of the first slice):
    # Half the distance between the first ring and the last
    # ring, on the negative Z side
    first_crystal_offset_z = \
        -((n_modules_z - 1) * module_repeat_vector_z +
          (n_crystals_z - 1) * crystal_repeat_vector_z) / 2.0

    corner_offset_zy = [
        scanner.crystal_dimensions[2] *
        np.array([-0.5, 0.5, 0.5, -0.5, -0.5]),
        scanner.crystal_dimensions[0] *
        np.array([0.5, 0.5, -0.5, -0.5, 0.5])]

    rsector_n_crystals_z = n_modules_z * n_crystals_z

    # Position in Z of each ring in a r-sector
    crystal_z_positions = np.zeros(rsector_n_crystals_z)

    # Crystal corner positions in YZ for all rings
    crystal_zy_corners = \
        np.zeros([2 * rsector_n_crystals_z, 5, 2])

    for module_index in range(n_modules_z):
        for crystal_index in range(n_crystals_z):

            ind = module_index * n_crystals_z + crystal_index

            y = rsector_x_translation
            z = module_index * module_repeat_vector_z + \
                crystal_index * crystal_repeat_vector_z + \
                first_crystal_offset_z

            crystal_z_positions[ind] = z

            corner_y = y + corner_offset_zy[1]
            corner_z = z + corner_offset_zy[0]

            crystal_zy_corners[ind, :, :] = \
                np.array([corner_z, corner_y]).T

    # Crystals on lower half of the ring (negated y)
    crystal_zy_corners[rsector_n_crystals_z:, :, 0] = \
        crystal_zy_corners[:rsector_n_crystals_z, :, 0]
    crystal_zy_corners[rsector_n_crystals_z:, :, 1] = \
        -crystal_zy_corners[:rsector_n_crystals_z, :, 1]

    # Slice positions in Z
    slice_z_positions = np.zeros(n_slices)

    for slice_index in range(n_slices):

        # Even slice: Aligned with a ring
        # Odd slice: Between two neighboring rings
        z = crystal_z_positions[slice_index // 2] \
            if slice_index % 2 == 0 else \
            (crystal_z_positions[slice_index // 2] +
             crystal_z_positions[(slice_index+1) // 2]) \
            / 2.0

        slice_z_positions[slice_index] = z

    return crystal_z_positions, \
        crystal_zy_corners, \
        slice_z_positions
