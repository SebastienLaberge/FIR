"""
Defines function "recon_OSEM", which applies the OSEM
reconstruction algorithm to a projection using a given scanner
geometry and volume template to get a reconstructed volume
"""

from dataclasses import dataclass
from os import makedirs
from os.path import join, relpath
from typing import Optional, Tuple

from FIR.PyInteractor.call_exec import call_exec
from FIR.PyInteractor.interfile_io import write_interfile
from FIR.PyInteractor.scanner import ScannerHeader
from FIR.PyInteractor.vol import VolHeader

EXECUTABLE_NAME = 'FIR_OSEM'


@dataclass
class OSEMAlgoParams:

    n_iterations: int
    n_subsets: int
    save_interval: Optional[int] = None


def recon_OSEM(
        in_proj_path: str,
        scanner: ScannerHeader,
        vol_template: VolHeader,
        exec_dir: str,
        work_dir: str,
        out_dir: str,
        out_vol_name: str,
        algo_params: OSEMAlgoParams,
        sensitivity_path: Optional[str] = None,
        bias_proj_path: Optional[str] = None,
        recompute_sensitivity: bool = True,
        dry_run: bool = False) -> Tuple[str, str]:

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

    params_filename, out_vol_path, out_vol_subiter_paths = \
        _write_params_file(
            in_proj_path,
            scanner_filename,
            vol_template_filename,
            work_dir,
            out_dir,
            out_vol_name,
            algo_params,
            sensitivity_path,
            bias_proj_path)

    # Call executable except for dry runs
    if not dry_run:

        # Recompute sensitivity flag must be given as a string
        recompute_sensitivity_str = \
            str(int(recompute_sensitivity))

        call_exec(
            work_dir,
            exec_dir,
            EXECUTABLE_NAME,
            [params_filename, recompute_sensitivity_str])

    return out_vol_path, out_vol_subiter_paths


def _write_params_file(
        in_proj_path: str,
        scanner_filename: str,
        vol_template_filename: str,
        work_dir: str,
        out_dir: str,
        out_vol_name: str,
        algo_params: OSEMAlgoParams,
        sensitivity_path: Optional[str],
        bias_proj_path: Optional[str]) -> Tuple[str, str, str]:

    # Get paths relative to working directory
    in_proj_rel_path = relpath(in_proj_path, work_dir)
    out_vol_rel_path = join(
        relpath(out_dir, work_dir),
        '_'.join(['recon', out_vol_name]))

    file_type = 'OSEM PARAMETERS'

    lines = [
        ('input projection file',
         f'{in_proj_rel_path}'),
        ('scanner file',
         f'{scanner_filename}'),
        ('output volume header',
         f'{vol_template_filename}'),
        ('output volume file name',
         f'{out_vol_rel_path}'),
        ('number of iterations',
         f'{algo_params.n_iterations}'),
        ('number of subsets',
         f'{algo_params.n_subsets}')
    ]

    if algo_params.save_interval is not None:

        lines.append(
            ('save interval',
             f'{algo_params.save_interval}'))

    if sensitivity_path is not None:

        sensitivity_path_rel = \
            relpath(sensitivity_path, work_dir)

        lines.append(
            ('sensitivity map volume',
             f'{sensitivity_path_rel}'))

    if bias_proj_path is not None:

        bias_proj_path_rel = relpath(bias_proj_path, work_dir)

        lines.append(
            ('bias projection',
             f'{bias_proj_path_rel}'))

    params_filename = \
        '_'.join(['OSEM', out_vol_name]) + '.params'

    # File names modified by executable
    out_vol_path = \
        join(work_dir, out_vol_rel_path + '.h33')

    out_vol_subiter_paths = []
    if algo_params.save_interval is not None:

        subiter_range = range(
            algo_params.save_interval,
            algo_params.n_subsets * algo_params.n_iterations,
            algo_params.save_interval)

        out_vol_subiter_paths = [
            join(work_dir, '_'.join(
                [out_vol_rel_path,
                 'subiter', str(subiter)]) + '.h33')
            for subiter in subiter_range]

    # Create output directory and write parameter file
    makedirs(out_dir, exist_ok=True)
    write_interfile(
        join(work_dir, params_filename),
        file_type,
        lines)

    return params_filename, \
        out_vol_path, \
        out_vol_subiter_paths
