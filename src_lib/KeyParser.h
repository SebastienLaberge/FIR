#pragma once

#include <any>
#include <istream>
#include <list>
#include <string>
#include <utility>
#include <vector>

class KeyParser;
typedef void (KeyParser::*KeyProcessor)();

enum class KeyType
{
  NONE,
  ASCII,
  INT,
  FLOAT,
  DOUBLE,
  LIST_OF_INTS,
  LIST_OF_FLOATS,
  LIST_OF_DOUBLES
};

struct MapElement
{
  MapElement(
    KeyType typeIn,
    KeyProcessor pProcessorIn,
    void* pVariableIn);

  // Type of keyword
  KeyType type;

  // Pointer to a member function of class KeyParser
  KeyProcessor pProcessor;

  // Pointer to a variable
  void* pVariable;
};

class KeyParser
{
public:

  KeyParser();

  // Add start and stop keys
  void addStartKey(const std::string& keyword);
  void addStopKey(const std::string& keyword);

  // Add variable keys
  void addKey(
    const std::string& keyword,
    std::string* pVariable);
  void addKey(const std::string& keyword, int* pVariable);
  void addKey(const std::string& keyword, float* pVariable);
  void addKey(const std::string& keyword, double* pVariable);
  void addKey(
    const std::string& keyword,
    std::vector<int>* pVariable);
  void addKey(
    const std::string& keyword,
    std::vector<float>* pVariable);
  void addKey(
    const std::string& keyword,
    std::vector<double>* pVariable);

  // Execute parsing
  bool parse(
    const std::string& filePath,
    bool writeWarnings = false);

private:

  // Implementation methods for adding keys

  void addFunctionKey(
    const std::string& keyword,
    KeyProcessor function);

  void addVariableKey(
    const std::string& keyword,
    KeyType type,
    void* variable);

  void addKeyToMap(
    const std::string& keyword,
    const MapElement& newElement);

  // Implementation methods for parsing

  // Read all lines in input stream with readAndParseLine and
  // execute processKey for each line
  bool doParsing(bool writeWarnings);

  // Read a line, find keyword and call parseValueInLine()
  bool readAndParseLine(bool writeWarning);

  // Substitute the sequence ${ENV} in a parsed line by
  // environment variable ENV
  void substituteEnvironmentVariables(std::string& s);

  // Get a keyword from a string
  std::string getKeyword(const std::string&) const;

  // See if current keyword is in the keyword map
  // If so, call its callback function and return true
  // Otherwise, conditionally write a warning and return false
  bool parseValueInLine(
    const std::string& line,
    bool writeWarning);

  // Call appropriate member function
  void processKey();

  // Common
  MapElement* findInKeyMap(const std::string& keyword);

  // Convert rough keyword into a standardised form
  std::string standardiseKeyword(
    const std::string& keyword) const;

public:

  // Callback function to start parsing
  // Has to be set by the first keyword
  void startParsing();

  // Callback function to stop parsing
  void stopParsing();

  // Callback function  that sets the variable to the value
  // given as the value of the keyword. If the keyword has no
  // value, setVariable will do nothing.
  void setVariable();

private:

  std::istream* mpInputStream;

  using KeyMap = std::list<std::pair<std::string, MapElement>>;
  KeyMap mKeyMap;

  enum class ParseStatus
  {
    endParsing,
    parsing
  };
  ParseStatus mStatus;

  // Working variables
  std::string mCurrentKeyword;
  bool mKeywordHasValue; // False: Only a keyword on the line
  MapElement* mpCurrentMapElement;
  std::any mCurrentParameter;
};
