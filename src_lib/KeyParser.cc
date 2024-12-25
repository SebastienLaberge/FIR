#include <KeyParser.h>

#include <console.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

template<typename T>
static std::istream& operator>>(
  std::istream& stream,
  std::vector<T>& vector)
{
  // Clear vector
  vector.clear();

  // Remove leading whitespace and get next character
  char nextChar;
  stream >> std::ws >> nextChar;

  // Return if stream is exhausted or if the next thing to
  // be parsed is not a list
  if (stream.fail() || nextChar != '{')
  {
    return stream;
  }

  // Parse values from list and populate vector
  T value;
  do
  {
    stream >> value;

    if (stream.fail())
    {
      break;
    }

    // Add parsed value to vector
    vector.push_back(value);

    stream >> std::ws >> nextChar;

  } while (!(stream.fail() || nextChar != ','));

  if (stream.fail())
  {
    stream.clear();
    stream >> std::ws >> nextChar;
  }

  if (stream.fail())
  {
    warning(
      "While reading a vector, expected closing }, but found "
      "EOF or worse.\n",
      "Length of vector returned is ",
      vector.size());

    return stream;
  }

  if (nextChar != '}')
  {
    warning(
      "While reading a vector, expected closing }, but found ",
      nextChar,
      " instead.\n",
      "Length of vector returned is ",
      vector.size());
  }

  return stream;
}

static bool extractValueFromString(
  std::string& value,
  const std::string& stringIn)
{
  const auto separator = stringIn.find('=', 0);

  if (separator == std::string::npos)
  {
    // There is no keyword to be found, let alone a value
    return false;
  }

  // Beginning of slice to strip leading whitespace
  const auto start =
    stringIn.find_first_not_of(" \t", separator + 1);

  if (start == std::string::npos)
  {
    // There is no value to be extracted
    return false;
  }

  // End of slice to strip trailing whitespace (exclusive)
  const auto stop = stringIn.find_last_not_of(" \t") + 1;

  // Extract slice
  value = stringIn.substr(start, stop - start);

  // A value has been extracted
  return true;
}

template<typename T>
static bool extractValueFromString(
  T& value,
  const std::string& stringIn)
{
  const auto separator = stringIn.find('=', 0);

  if (separator == std::string::npos)
  {
    // There is no keyword to be found, let alone a value
    return false;
  }

  std::istringstream str(stringIn.c_str() + separator + 1);
  str >> value;
  return !str.fail();
}

// Special handling for vectors if no {} means one element
template<typename T>
static bool extractVectorValueFromString(
  std::vector<T>& value,
  const std::string& stringIn)
{
  const auto separator = stringIn.find('=', 0);

  if (separator == std::string::npos)
  {
    // There is no keyword to be found, let alone a value
    return false;
  }

  // Beginning of slice to strip leading whitespace
  const auto start =
    stringIn.find_first_not_of(" \t", separator + 1);

  if (start == std::string::npos)
  {
    // There is no value to be extracted
    return false;
  }

  std::istringstream str(stringIn.c_str() + start);

  if (stringIn[start] == '{')
  {
    str >> value;
  }
  else
  {
    value.resize(1);
    str >> value[0];
  }

  return !str.fail();
}

template<class T>
struct Type2Type
{
  typedef T type;
};

template<class T>
static bool extractAnyValueFromString(
  std::any& value,
  Type2Type<T>,
  const std::string& stringIn)
{
  value = T();

  return extractValueFromString(
    *std::any_cast<T>(&value),
    stringIn);
}

template<class T>
static bool extractAnyVectorValueFromString(
  std::any& value,
  Type2Type<T>,
  const std::string& stringIn)
{
  value = T();

  return extractVectorValueFromString(
    *std::any_cast<T>(&value),
    stringIn);
}

MapElement::MapElement(
  KeyType typeIn,
  KeyProcessor pProcessorIn,
  void* pVariableIn):
  type{typeIn},
  pProcessor{pProcessorIn},
  pVariable{pVariableIn}
{
}

KeyParser::KeyParser():
  mStatus{ParseStatus::endParsing},
  mpCurrentMapElement{nullptr}
{
}

void KeyParser::addStartKey(const std::string& keyword)
{
  addFunctionKey(keyword, &KeyParser::startParsing);
}

void KeyParser::addStopKey(const std::string& keyword)
{
  addFunctionKey(keyword, &KeyParser::stopParsing);
}

void KeyParser::addKey(
  const std::string& keyword,
  std::string* pVariable)
{
  addVariableKey(keyword, KeyType::ASCII, pVariable);
}

void KeyParser::addKey(
  const std::string& keyword,
  int* pVariable)
{
  addVariableKey(keyword, KeyType::INT, pVariable);
}

void KeyParser::addKey(
  const std::string& keyword,
  float* pVariable)
{
  addVariableKey(keyword, KeyType::FLOAT, pVariable);
}

void KeyParser::addKey(
  const std::string& keyword,
  double* pVariable)
{
  addVariableKey(keyword, KeyType::DOUBLE, pVariable);
}

void KeyParser::addKey(
  const std::string& keyword,
  std::vector<int>* pVariable)
{
  addVariableKey(keyword, KeyType::LIST_OF_INTS, pVariable);
}

void KeyParser::addKey(
  const std::string& keyword,
  std::vector<float>* pVariable)
{
  addVariableKey(keyword, KeyType::LIST_OF_FLOATS, pVariable);
}

void KeyParser::addKey(
  const std::string& keyword,
  std::vector<double>* pVariable)
{
  addVariableKey(keyword, KeyType::LIST_OF_DOUBLES, pVariable);
}

bool KeyParser::parse(
  const std::string& filePath,
  bool writeWarnings)
{
  std::ifstream inputStream;

  inputStream.open(filePath);

  if (!inputStream)
  {
    error("Couldn't open file ", filePath);
  }

  mpInputStream = &inputStream;

  return doParsing(writeWarnings);
}

void KeyParser::addFunctionKey(
  const std::string& keyword,
  KeyProcessor function)
{
  addKeyToMap(
    keyword,
    MapElement(KeyType::NONE, function, nullptr));
}

void KeyParser::addVariableKey(
  const std::string& keyword,
  KeyType type,
  void* pVariable)
{
  addKeyToMap(
    keyword,
    MapElement(type, &KeyParser::setVariable, pVariable));
}

void KeyParser::addKeyToMap(
  const std::string& keyword,
  const MapElement& newElement)
{
  const auto standardisedKeyword = standardiseKeyword(keyword);

  auto* pElement = findInKeyMap(standardisedKeyword);

  if (pElement == nullptr)
  {
    mKeyMap.push_back(std::pair<std::string, MapElement>(
      standardisedKeyword,
      newElement));
  }
  else
  {
    warning(
      "Keyword ",
      keyword,
      " already registered for parsing. "
      "Overwriting previous value.");

    *pElement = newElement;
  }
}

bool KeyParser::doParsing(bool writeWarnings)
{
  if (readAndParseLine(writeWarnings))
  {
    processKey();
  }

  if (mStatus != ParseStatus::parsing)
  {
    // Something's wrong. We're finding data, but we're not
    // supposed to be parsing. We'll exit with an error, but
    // first write a warning. The warning will say that we miss
    // the "start parsing" keyword (if we can find it in the
    // map)

    // Find starting keyword
    std::string startKeyword;
    for (auto& elem : mKeyMap)
    {
      if (elem.second.pProcessor == &KeyParser::startParsing)
      {
        startKeyword = elem.first;
      }
    }

    if (startKeyword.length() > 0)
    {
      warning(
        "Required first keyword \"",
        startKeyword,
        "\" not found");
    }
    else
    {
      // There doesn't seem to be a startParsing keyword, so we
      // cannot include it in the warning. (it could be a
      // side-effect of another key, so we're not sure if the
      // map is correct or not)
      warning("Data found, but key parser status is "
              "\"not parsing\". Keymap possibly incorrect.");
    }

    return false;
  }

  while (mStatus == ParseStatus::parsing)
  {
    if (readAndParseLine(writeWarnings))
    {
      processKey();
    }
  }

  return true;
}

bool KeyParser::readAndParseLine(const bool writeWarnings)
{
  std::string line;

  // Read lines until either a non-empty line is found or the
  // end of the input stream is reached
  while (true)
  {
    if (!mpInputStream->good())
    {
      warning("Early EOF or bad file");
      stopParsing();
      return false;
    }

    std::getline(*mpInputStream, line);

    // Check if current character is white space
    // If not, get out of the loop to continue
    const auto pos = line.find_first_not_of(" \t");
    if (pos != std::string::npos)
    {
      break;
    }
  }

  // Substitute environment variables in line
  substituteEnvironmentVariables(line);

  // Get keyword and standardise it
  mCurrentKeyword = standardiseKeyword(getKeyword(line));

  return parseValueInLine(line, writeWarnings);
}

void KeyParser::substituteEnvironmentVariables(std::string& s)
{
  // Replace ${value} with value of environment variable
  std::string::size_type startEnvString;
  while ((startEnvString = s.find("${")) != std::string::npos)
  {
    // Find closing bracket
    const std::string::size_type endEnvString =
      s.find('}', startEnvString + 2);

    if (endEnvString == std::string::npos)
    {
      break;
    }

    const auto sizeEnvString =
      endEnvString - startEnvString + 1;

    const auto nameEnvVar =
      s.substr(startEnvString + 2, sizeEnvString - 3);

    const auto valueEnvVar = std::getenv(nameEnvVar.c_str());

    if (valueEnvVar == nullptr)
    {
      warning(
        "Environment variable '",
        nameEnvVar,
        " not found. Replaced by empty string.\n"
        "This happened on the following line: ",
        s);

      s.erase(startEnvString, sizeEnvString);
    }
    else
    {
      s.replace(startEnvString, sizeEnvString, valueEnvVar);
    }
  }
}

std::string KeyParser::getKeyword(const std::string& line) const
{
  const auto eok = line.find_first_of(":", 0);

  return line.substr(0, eok);
}

bool KeyParser::parseValueInLine(
  const std::string& line,
  bool writeWarnings)
{
  // Set the current map element to that corresponding to the
  // current keyword
  mpCurrentMapElement = findInKeyMap(mCurrentKeyword);

  if (mpCurrentMapElement == nullptr)
  {
    // Issue warning unless this is an empty line or a comment
    if (
      mCurrentKeyword.length() != 0 &&
      mCurrentKeyword[0] != ';' && writeWarnings)
    {
      warning("Unrecognized keyword: ", mCurrentKeyword);
    }

    // No processing of this key
    return false;
  }

  // Depending on the parameter type, get the correct value
  // from the line and set the right temporary variable
  switch (mpCurrentMapElement->type)
  {
  case KeyType::NONE:
    mKeywordHasValue = false;
    break;

  case KeyType::ASCII:
    mKeywordHasValue = extractAnyValueFromString(
      mCurrentParameter,
      Type2Type<std::string>(),
      line);
    break;

  case KeyType::INT:
    mKeywordHasValue = extractAnyValueFromString(
      mCurrentParameter,
      Type2Type<int>(),
      line);
    break;

  case KeyType::FLOAT:
    mKeywordHasValue = extractAnyValueFromString(
      mCurrentParameter,
      Type2Type<float>(),
      line);
    break;

  case KeyType::DOUBLE:
    mKeywordHasValue = extractAnyValueFromString(
      mCurrentParameter,
      Type2Type<double>(),
      line);
    break;

  case KeyType::LIST_OF_INTS:
    mKeywordHasValue = extractAnyVectorValueFromString(
      mCurrentParameter,
      Type2Type<std::vector<int>>(),
      line);
    break;

  case KeyType::LIST_OF_FLOATS:
    mKeywordHasValue = extractAnyVectorValueFromString(
      mCurrentParameter,
      Type2Type<std::vector<float>>(),
      line);
    break;

  case KeyType::LIST_OF_DOUBLES:
    mKeywordHasValue = extractAnyVectorValueFromString(
      mCurrentParameter,
      Type2Type<std::vector<double>>(),
      line);
    break;

  default:
    error(
      "Keyword ",
      mCurrentKeyword,
      " has unsupported type of parameters");

    // To avoid compiler warnings
    return false;
  }

  return true;
}

void KeyParser::processKey()
{
  if (mpCurrentMapElement->pProcessor != 0)
  {
    // Call appropriate member function
    (this->*(mpCurrentMapElement->pProcessor))();
  }
}

MapElement* KeyParser::findInKeyMap(const std::string& keyword)
{
  for (auto& elem : mKeyMap)
  {
    if (elem.first == keyword)
    {
      return &(elem.second);
    }
  }

  // Key not found
  return nullptr;
}

std::string KeyParser::standardiseKeyword(
  const std::string& keyword) const
{
  // This follows Interfile 3.3 conventions:
  //
  // 1-The characters space, tab, underscore, !
  //    are all treated as white space and ignored.
  // 2-Ignoring white space means trimming at the
  //   start and end of the keyword, and replacing
  //   repeated white space with a single space.
  // 3-Case is ignored.

  std::string::size_type separator{0}; // current index
  const auto whiteSpace = " \t_!";

  // Skip white space
  separator = keyword.find_first_not_of(whiteSpace, 0);

  if (separator == std::string::npos)
  {
    return {};
  }

  // Remove trailing white spaces
  const std::string::size_type eok =
    keyword.find_last_not_of(whiteSpace);

  std::string kw;
  kw.reserve(eok - separator + 1);
  bool previousWasWhitespace = false;
  while (separator <= eok)
  {
    if (
      isspace(static_cast<int>(keyword[separator])) ||
      keyword[separator] == '_' || keyword[separator] == '!')
    {
      if (!previousWasWhitespace)
      {
        kw.append(1, ' ');
        previousWasWhitespace = true;
      }
      // else: skip this white space character
    }
    else
    {
      if (keyword[separator] == '[' && !previousWasWhitespace)
      {
        kw.append(" [");
      }
      else
      {
        kw.append(
          1,
          static_cast<char>(
            tolower(static_cast<int>(keyword[separator]))));
      }

      previousWasWhitespace = false;
    }
    ++separator;
  }
  return kw;
}

void KeyParser::startParsing()
{
  mStatus = ParseStatus::parsing;
}

void KeyParser::stopParsing()
{
  mStatus = ParseStatus::endParsing;
}

void KeyParser::setVariable()
{
  if (!mKeywordHasValue)
  {
    return;
  }

#define KP_caseAssign(KeyTypeValue, type)                      \
  case KeyTypeValue:                                           \
    *reinterpret_cast<type*>(mpCurrentMapElement->pVariable) = \
      *std::any_cast<type>(&this->mCurrentParameter);          \
    break;

  switch (mpCurrentMapElement->type)
  {
    KP_caseAssign(KeyType::ASCII, std::string) KP_caseAssign(
      KeyType::INT,
      int) KP_caseAssign(KeyType::FLOAT, float)
      KP_caseAssign(KeyType::DOUBLE, double)
        KP_caseAssign(KeyType::LIST_OF_INTS, std::vector<int>)
          KP_caseAssign(
            KeyType::LIST_OF_FLOATS,
            std::vector<float>)
            KP_caseAssign(
              KeyType::LIST_OF_DOUBLES,
              std::vector<double>)

              default:
      warning("Unknown type (Implementation error)");
    break;
  }
}
