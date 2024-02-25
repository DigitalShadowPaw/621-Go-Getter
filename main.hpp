//
//  main.h
//  621-Go-Getter
//

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

using namespace std;

// folders paths
const string secretsFolder = "secrets";
const string poolsFolder = "pools";
const string DataFolder = "Data";


// the username
extern string username;
// the api key
extern string apiKey;


//debug
extern bool debugMode;

#endif /* main_h */
