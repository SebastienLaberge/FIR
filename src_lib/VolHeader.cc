#include <VolHeader.h>

#include <console.h>

void VolHeader::check()
{
  if (
    volSize.nPixelsX <= 0 || //
    volSize.nPixelsY <= 0 || //
    volSize.nSlices <= 0)
  {
    error("Number of voxels must be greater than zero in each "
          "dimension");
  }

  if (
    voxelExtent.pixelWidth <= 0 ||
    voxelExtent.pixelHeight <= 0 ||
    voxelExtent.sliceThickness <= 0)
  {
    error("Voxel extent must be greater than zero in each "
          "dimension");
  }

  if (nFrames <= 0)
  {
    error("Number of frames must be greater than zero");
  }
}

void VolGeometry::fill(const VolHeader& header)
{
  // Volume extent
  volExtent = header.volSize * header.voxelExtent;

  // Number of voxels
  nVoxelsPerFrame =           //
    header.volSize.nPixelsX * //
    header.volSize.nPixelsY * //
    header.volSize.nSlices;
  nVoxelsTotal = nVoxelsPerFrame * header.nFrames;
}
