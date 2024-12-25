#pragma once

#include <types.h>

namespace types
{
Size VolSize::nVoxelsTotal() const
{
  return nPixelsX * nPixelsY * nSlices;
}

bool VolSize::operator==(const VolSize& rhs) const
{
  return nPixelsX == rhs.nPixelsX && //
         nPixelsY == rhs.nPixelsY && //
         nSlices == rhs.nSlices;
}

bool VoxelExtent::operator==(const VoxelExtent& rhs) const
{
  return pixelWidth == rhs.pixelWidth &&
         pixelHeight == rhs.pixelHeight &&
         sliceThickness == rhs.sliceThickness;
}

VolExtent operator*(
  const VolSize& volSize,
  const VoxelExtent& voxelExtent)
{
  return {
    volSize.nPixelsX * voxelExtent.pixelWidth,
    volSize.nPixelsY * voxelExtent.pixelHeight,
    volSize.nSlices * voxelExtent.sliceThickness};
}

bool SpatialCoords3D::operator==(
  const SpatialCoords3D& rhs) const
{
  return x == rhs.x && y == rhs.y && z == rhs.z;
}
}
