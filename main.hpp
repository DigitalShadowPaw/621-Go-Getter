#ifndef main_h
#define main_h

#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <chrono>
#include <thread>
#include <unordered_set>
#include <regex>
#include <cctype>

using namespace std;

// folders paths
extern const string secretsFolder = "secrets";
extern const string poolsFolder = "pools";
extern const string DataFolder = "Data";


// the username
extern string username;
// the api key
extern string apiKey;


//debug
extern bool debugMode;

#endif
