#include <tools.h>

#ifdef _OPENMP
#include <omp.h>
#endif

std::string strToUpper(const std::string& in)
{
  std::string out(in.size(), '\0');
  for (int i = 0; in[i] != '\0'; i++)
  {
    out[i] = toupper(in[i]);
  }
  return out;
}

bool systemIsLittleEndian()
{
  const int n = 1;

  return *(char*)&n == 1;
}

void addPath(
  const std::string& headerFile,
  std::string& dataFile)
{
  if (!dataFile.empty() && dataFile[0] != '/')
  {
    const auto lastSlash = headerFile.find_last_of("/");

    if (lastSlash != std::string::npos)
    {
      dataFile.insert(0, headerFile, 0, lastSlash + 1);
    }
  }
}

int getNThreads()
{
  int nThreads = 0;

#pragma omp parallel reduction(+ : nThreads)
  nThreads++;

  return nThreads;
}

int getCurrentThread()
{
  int currentThread = 0;

#ifdef _OPENMP
  currentThread += omp_get_thread_num();
#endif

  return currentThread;
}
