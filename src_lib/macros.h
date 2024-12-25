// Macros for quick basic math operations
// Note: SIGN considers 0 to be  positive
#define ABS(a) ((a) < 0 ? (-(a)) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Machine precision
#define EPSILON 0.00001

// Pi
#define PI 3.141592653589793

// Macro to make index ind loop within [MIN, MAX]
#define LOOP(INDEX, MIN, MAX) \
  for (int INDEX = (MIN); INDEX <= (MAX); ++INDEX)

// Macros to loop across projection data

#define LOOP(INDEX, MIN, MAX) \
  for (int INDEX = (MIN); INDEX <= (MAX); ++INDEX)

#define LOOP_SEG(SEG, PROJ)          \
  LOOP(                              \
    SEG,                             \
    -(PROJ).getGeometry().segOffset, \
    (PROJ).getGeometry().segOffset)

#define LOOP_VIEW(VIEW, PROJ) \
  LOOP(VIEW, 0, (PROJ).getGeometry().nViews - 1)

#define LOOP_AXIAL(AXIAL_COORD, PROJ, SEG) \
  LOOP(                                    \
    AXIAL_COORD,                           \
    0,                                     \
    (PROJ).getGeometry().getNAxialCoords(SEG) - 1)

#define LOOP_TANG(TANG_COORD, PROJ)         \
  LOOP(                                     \
    TANG_COORD,                             \
    -(PROJ).getGeometry().tangCoordOffset,  \
    -(PROJ).getGeometry().tangCoordOffset + \
      (PROJ).getHeader().nTangCoords - 1)

// Macro to access bin from segment number and index along the
// remaining dimension
#define BIN(PROJ, SEG, BIN_INDEX)                             \
  (PROJ).getDataArray()[SEG + (PROJ).getGeometry().segOffset] \
                       [BIN_INDEX]
