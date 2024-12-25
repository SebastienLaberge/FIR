#include <ProjHeader.h>

#include <console.h>
#include <macros.h>

void ProjHeader::check()
{
  // Number of rings

  if (nRings <= 0)
  {
    error("\"number of rings\" must be provided and > 0");
  }

  // Number of crystals per ring

  if (nCrystalsPerRing <= 0)
  {
    error("\"number of crystals per ring\" must be provided "
          "and > 0");
  }

  if (nCrystalsPerRing % 4 != 0)
  {
    error("\"number of crystals per ring\" must be a multiple "
          "of four");
  }

  // Segment span

  if (segmentSpan <= 0)
  {
    error("\"segment span\" must be > 0");
  }

  if (segmentSpan % 2 != 1)
  {
    error("\"segment span\" must be an odd number");
  }

  // Maximum span allowed, guaranteed to be odd
  const auto maxSpan = 2 * nRings - 1;

  if (segmentSpan > maxSpan)
  {
    error(
      "For ",
      nRings,
      " ring(s), \"segment span\" must not be > ",
      maxSpan);
  }

  // Number of segments

  if (nSegments < 0)
  {
    error("\"number of segments\" must not be negative");
  }

  if (nSegments % 2 != 1)
  {
    error("\"number of segments\" must be an odd number");
  }

  // Maximum number of segments allowed, guaranteed to be odd
  // Decrement first result by 1 if it is even to make it odd
  auto maxNSegments = maxSpan / segmentSpan;
  maxNSegments -= 1 - maxNSegments % 2;

  if (nSegments > maxNSegments)
  {
    error(
      "For ",
      nRings,
      " ring(s) and a segment span of ",
      segmentSpan,
      ", \"number of segments\" must not be > ",
      maxNSegments);
  }

  // Number of tangential coordinates

  if (nTangCoords < 0)
  {
    error("\"number of tangential coordinates\" must not be "
          "negative");
  }

  // Maximum number of tangential coordinates allowed
  const auto maxNTangCoords = nCrystalsPerRing - 1;

  if (nTangCoords == 0)
  {
    // Default value: Largest possible value
    nTangCoords = maxNTangCoords;
  }
  else if (nTangCoords > maxNTangCoords)
  {
    error(
      "For ",
      nCrystalsPerRing,
      " crystals per ring, "
      "\"number of tangential coordinates\" must not be > ",
      maxNTangCoords);
  }
}

void ProjGeometry::fill(const ProjHeader& header)
{
  // Offsets for bin coordinates that can be negative
  segOffset = (header.nSegments - 1) / 2;
  tangCoordOffset = header.nTangCoords / 2;

  // Number of axial coordinates for each included segment
  nAxialCoords.resize(header.nSegments);
  if (header.segmentSpan == 1)
  {
    // For a span of 1, the segment length reduces by one for
    // each segment away from the central segment, which has
    // the same length as the number of rings
    LOOP(seg, -segOffset, segOffset)
    {
      nAxialCoords[seg + segOffset] = header.nRings - ABS(seg);
    }
  }
  else
  {
    const auto centralSegmentLength = 2 * header.nRings - 1;

    LOOP(seg, -segOffset, segOffset)
    {
      const auto absSeg = ABS(seg);

      // Starting from central segment
      auto segmentLength = centralSegmentLength;

      if (absSeg >= 1)
      {
        // Length reduction when leaving central segment
        // Derived from: 2*((segmentSpan-1)/2+1)
        segmentLength -= header.segmentSpan + 1;

        if (absSeg >= 2)
        {
          // Length reduction for each subsequent displacement
          // away from middle segment
          segmentLength -=
            2 * header.segmentSpan * (absSeg - 1);
        }
      }

      nAxialCoords[seg + segOffset] = segmentLength;
    }
  }

  // Number of views
  nViews = header.nCrystalsPerRing / 2;

  // Total number of bins
  nBins = 0;
  LOOP(seg, -segOffset, segOffset)
  {
    nBins +=                          //
      nAxialCoords[seg + segOffset] * //
      nViews *                        //
      header.nTangCoords;
  }

  // Half the segment span
  halfSegmentSpan = (header.segmentSpan - 1) / 2;

  // Maximum ring difference
  if (header.segmentSpan == 1)
  {
    maxRingDiff = segOffset;
  }
  else
  {
    maxRingDiff = halfSegmentSpan;

    if (segOffset > 0)
    {
      maxRingDiff += segOffset * header.segmentSpan;
    }
  }
}
