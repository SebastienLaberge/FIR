"""
Defines function "forward", which applies a forward projection to
a volume using a given scanner geometry and projection template
"""

from os import makedirs
from os.path import join, relpath
from typing import Optional, Tuple

from FIR.PyInteractor.call_exec import call_exec
from FIR.PyInteractor.interfile_io import write_interfile
from FIR.PyInteractor.proj import ProjHeader
from FIR.PyInteractor.scanner import ScannerHeader
from FIR.PyInteractor.vol import VolHeader

EXECUTABLE_NAME = 'FIR_ForwardProj'


def forward(
        in_vol_path: str,
        scanner: ScannerHeader,
        proj_template: ProjHeader,
        exec_dir: str,
        work_dir: str,
        out_dir: str,
        out_proj_name: str,
        frame_to_project: Optional[int] = None,
        in_mask_path: Optional[str] = None,
        dry_run: bool = False):

    # Write scanner header in working directory
    scanner_name = \
        scanner.name \
        if scanner.name is not None else \
        "scanner"
    scanner_filename = scanner_name + '.scan'
    scanner.write(work_dir, scanner_filename)

    # Write projection template header in working directory
    proj_template_filename = \
        '_'.join(['projTemplate', out_proj_name]) + '.hs'
    proj_template.write(work_dir, proj_template_filename)

    # Get number of frames (all of them if no frame is specified)
    # Read volume header to get number of frames if necessary
    n_frames = VolHeader.read(in_vol_path).n_frames \
        if frame_to_project is None else 1

    # Write parameter file
    params_filename, out_proj_paths = \
        _write_params_file(
            in_vol_path,
            scanner_filename,
            proj_template_filename,
            work_dir,
            out_dir,
            out_proj_name,
            frame_to_project,
            n_frames,
            in_mask_path)

    # Call executable except for dry runs
    if not dry_run:

        call_exec(
            work_dir,
            exec_dir,
            EXECUTABLE_NAME,
            [params_filename])

    # Return expected locations of output projections
    return out_proj_paths


def _write_params_file(
        in_vol_path: str,
        scanner_filename: str,
        proj_template_filename: str,
        work_dir: str,
        out_dir: str,
        out_proj_name: str,
        frame_to_project: Optional[int],
        n_frames: int,
        in_mask_path: Optional[str]) -> Tuple[str, str]:

    # Get paths relative to working directory
    in_vol_rel_path = relpath(in_vol_path, work_dir)
    out_proj_rel_path = join(
        relpath(out_dir, work_dir),
        '_'.join(['proj', out_proj_name]))

    frame_to_project_str = \
        str(frame_to_project) \
        if frame_to_project is not None else \
        ""

    file_type = 'FORWARD PROJECTION PARAMETERS'

    lines = [
        ('input volume file',
         f'{in_vol_rel_path}'),
        ('scanner file',
         f'{scanner_filename}'),
        ('output projection header',
         f'{proj_template_filename}'),
        ('output projection file name',
         f'{out_proj_rel_path}'),
        ('frames to project',
         f'{frame_to_project_str}')]

    if in_mask_path is not None:

        in_mask_rel_path = relpath(in_mask_path, work_dir)
        lines.insert(
            -1,
            ('mask volume file',
             f'{in_mask_rel_path}'))

    # Parameter file
    params_filename = \
        '_'.join(['forward', out_proj_name]) + '.params'

    def get_out_proj_path(frame_index):

        proj_file = '_'.join([out_proj_rel_path,
                              'frame',
                             str(frame_index)]) + '.hs'

        return join(work_dir, proj_file)

    out_frames = [frame_to_project] \
        if frame_to_project is not None else \
        range(n_frames)

    # Get expected locations of output projections
    out_proj_paths = [get_out_proj_path(frame_index)
                      for frame_index in out_frames]

    # Create output directory and write parameter file
    makedirs(out_dir, exist_ok=True)
    write_interfile(
        join(work_dir, params_filename),
        file_type,
        lines)

    return params_filename, out_proj_paths
