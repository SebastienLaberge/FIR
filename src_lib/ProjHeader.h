#pragma once

#include <vector>

// Projection dimensions
//          || Axial          | Transversal |
// =========||================|=============|
//    Angle || segment        | view        |
// Distance || axialCoord*    | tangCoord   | => Viewgram
// =========||================|=============|
//          || => Michelogram | => Sinogram |
//
// *: Size depends on segment

struct ProjHeader
{
  // Scanner basic geometry (required)
  // Must match the values from the associated scanner object
  int nRings;
  int nCrystalsPerRing;

  // Michelogram compression
  int segmentSpan; // Defaults to 1

  // Fundamental projection dimensions (other two in geometry)
  int nSegments;   // Defaults to 1
  int nTangCoords; // Set to 0 (default) to get maximum value

  // Operators
  inline bool operator==(const ProjHeader& rhs) const;
  inline ProjHeader& operator=(const ProjHeader& rhs);

  // Fill structure with default values
  // Note: Must be executed before filling with specific values
  inline void setDefaults();

  // Check validity of values and throw if there is a problem
  // Note: Must be executed after filling with specific values
  void check();
};

struct ProjGeometry
{
  // Total number of bins kept in memory
  int nBins;

  // Offset segOffset such that, for a vector V over segments,
  // the element associated with segment segIndex is given by:
  // V[segIndex + segOffset]
  int segOffset;

  // Offset tangCoordOffset such that, for a vector V over
  // tangential coordinates, the element associated with
  // coordinate tangCoord is given by:
  // V[tangCoord + tangCoordOffset]
  int tangCoordOffset;

  // Derived projection dimensions (other two in header)
  std::vector<int> nAxialCoords;
  int nViews;

  // The number of diagonals on each side of the central
  // diagonal in any given segment on the michelogram
  int halfSegmentSpan;

  // Maximum ring difference allowed in the michelogram, i.e.
  // the distance from the central diagonal to the outermost
  // ones
  int maxRingDiff;

  // Operator
  inline void operator=(const ProjGeometry& other);

  // Get the number of axial coordinates for a given segment
  inline int getNAxialCoords(int seg) const;

  // Fill structure using information contained in header
  // Note: Must be executed before using the structure
  void fill(const ProjHeader& header);
};

#include <ProjHeader.inl>
