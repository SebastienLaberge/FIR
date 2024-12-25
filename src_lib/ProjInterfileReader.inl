#pragma once

#include <ProjInterfileReader.h>

ProjHeader ProjInterfileReader::getHeader()
{
  return mHeader;
}

ProjGeometry ProjInterfileReader::getGeometry()
{
  return mGeometry;
}
