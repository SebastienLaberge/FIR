"""
Defines function "backward", which applies a backward projection
to a projection using a given scanner geometry and volume
template to get a reconstructed volume
"""
from os import makedirs
from os.path import join, relpath

from FIR.PyInteractor.call_exec import call_exec
from FIR.PyInteractor.scanner import ScannerHeader
from FIR.PyInteractor.vol import VolHeader

EXECUTABLE_NAME = 'FIR_BackProj'


def backward(
        in_proj_path: str,
        scanner: ScannerHeader,
        vol_template: VolHeader,
        exec_dir: str,
        work_dir: str,
        out_dir: str,
        out_vol_name: str,
        dry_run: bool = False) -> str:

    # Write scanner header in working directory
    scanner_name = \
        scanner.name \
        if scanner.name is not None else \
        "scanner"
    scanner_filename = scanner_name + '.scan'
    scanner.write(work_dir, scanner_filename)

    # Write volume template header in working directory
    vol_template_filename = \
        '_'.join(['volTemplate', out_vol_name]) + '.h33'
    vol_template.write(join(work_dir, vol_template_filename))

    exec_args, out_vol_path = \
        _get_args(
            in_proj_path,
            scanner_filename,
            vol_template_filename,
            work_dir,
            out_dir,
            out_vol_name)

    # Call executable except for dry runs
    if not dry_run:

        call_exec(
            work_dir,
            exec_dir,
            EXECUTABLE_NAME,
            exec_args)
    else:
        print(exec_args)

    # Return expected location of output volume
    return out_vol_path


def _get_args(
        in_proj_path: str,
        scanner_filename: str,
        vol_template_filename: str,
        work_dir: str,
        out_dir: str,
        out_vol_name: str):

    # Get paths relative to working directory
    in_proj_rel_path = relpath(in_proj_path, work_dir)
    out_vol_rel_path = join(
        relpath(out_dir, work_dir),
        '_'.join(['backproj', out_vol_name]))

    # Get input arguments to executable
    exec_args = [
        in_proj_rel_path,
        scanner_filename,
        vol_template_filename,
        out_vol_rel_path]

    # Get expected location of output volume
    out_vol_path = join(work_dir, out_vol_rel_path) + '.h33'

    # Create output directory
    makedirs(out_dir, exist_ok=True)

    return exec_args, out_vol_path
