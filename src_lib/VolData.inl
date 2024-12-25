#pragma once

#include <VolData.h>

#include <cassert>

int VolData::getActiveFrame() const
{
  return mActiveFrame;
}

const VolHeader& VolData::getHeader() const
{
  return mHeader;
}

int VolData::getNFrames() const
{
  return mHeader.nFrames;
}

const types::VolExtent& VolData::getVolExtent() const
{
  return mGeometry.volExtent;
}

int VolData::getNVoxelsPerFrame() const
{
  return mGeometry.nVoxelsPerFrame;
}

bool VolData::isAllocated() const
{
  return mDataArray != nullptr;
}

types::VoxelValue* VolData::getDataArray() const
{
  return mDataArray;
}

void VolData::setVoxel(
  int i,
  int j,
  int k,
  types::VoxelValue value)
{
  assert(
    (i < mHeader.volSize.nPixelsX) &&
    (j < mHeader.volSize.nPixelsY) &&
    (k < mHeader.volSize.nSlices));

  const auto ind = //
    i +            //
    j * mHeader.volSize.nPixelsX +
    k * mHeader.volSize.nPixelsX * mHeader.volSize.nPixelsY;

  mDataArray[ind] = value;
}

types::VoxelValue VolData::getVoxel(int i, int j, int k) const
{
  assert(
    (i >= 0) && (i < mHeader.volSize.nPixelsX) && //
    (j >= 0) && (j < mHeader.volSize.nPixelsY) && //
    (k >= 0) && (k < mHeader.volSize.nSlices));

  const auto ind = //
    i +            //
    j * mHeader.volSize.nPixelsX +
    k * mHeader.volSize.nPixelsX * mHeader.volSize.nPixelsY;

  return mDataArray[ind];
}
