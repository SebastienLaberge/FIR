#pragma once

#include <ProjData.h>

const ProjHeader& ProjData::getHeader() const
{
  return mHeader;
}

const ProjGeometry& ProjData::getGeometry() const
{
  return mGeometry;
}

types::BinValue** ProjData::getDataArray() const
{
  return mDataArray;
}

void ProjData::setBin(
  int seg,
  int view,
  int axialCoord,
  int tangCoord,
  types::BinValue value)
{
  const auto [seg_adj, ind] =
    getInd(seg, view, axialCoord, tangCoord);

  mDataArray[seg_adj][ind] = value;
}

types::BinValue ProjData::getBin(
  int seg,
  int view,
  int axialCoord,
  int tangCoord) const
{
  const auto [seg_adj, ind] =
    getInd(seg, view, axialCoord, tangCoord);

  return mDataArray[seg_adj][ind];
}

void ProjData::incrementBin(
  int seg,
  int view,
  int axialCoord,
  int tangCoord)
{
  const auto [seg_adj, ind] =
    getInd(seg, view, axialCoord, tangCoord);

  mDataArray[seg_adj][ind]++;
}

void ProjData::weightBin(
  int seg,
  int view,
  int axialCoord,
  int tangCoord,
  types::BinValue weight)
{
  const auto [seg_adj, ind] =
    getInd(seg, view, axialCoord, tangCoord);

  mDataArray[seg_adj][ind] *= weight;
}

std::tuple<int, int> ProjData::getInd(
  int seg,
  int view,
  int axialCoord,
  int tangCoord) const
{
  seg += mGeometry.segOffset;
  tangCoord += mGeometry.tangCoordOffset;

  const auto nAxialCoords = mGeometry.getNAxialCoords(seg);

  const auto ind =                              //
    view * nAxialCoords * mHeader.nTangCoords + //
    axialCoord * mHeader.nTangCoords +          //
    tangCoord;

  return {seg, ind};
}
