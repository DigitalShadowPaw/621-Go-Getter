//
//  main.cpp
//  621-Go-Getter
//
#include "main.hpp"
#include <curl/curl.h>
#include <json.hpp>

namespace fs = filesystem;

using namespace std;

using json = nlohmann::json;


// === Begin creating secrets folder and key file ===

// Function to create folder if it doesn't exist
bool createFolder(const string& folderPath) {
    struct stat info;
    if (stat(folderPath.c_str(), &info) != 0) {
        // Folder doesn't exist, try to create it
        int status = mkdir(folderPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == 0) {
            cout << "Created folder: " << folderPath << endl;
            return true;
        } else {
            cerr << "Failed to create folder: " << folderPath << endl;
            return false;
        }
    } else if (info.st_mode & S_IFDIR) {
        // Folder exists
        // cout << "Folder already exists: " << folderPath << endl;
        return true;
    } else {
        // Path exists but is not a folder
        cerr << "Path already exists but is not a folder: " << folderPath << endl;
        return false;
    }
}


// Function to read username and API key from file
bool readCredentials(const string& folderPath) {
    // Construct the file path
    string filePath = folderPath + "/key";
    
    // print out
    if (debugMode)
    {
        // Get the absolute path of the file
        fs::path absolutePath = fs::absolute(filePath);

        // Print the full system path of the key file
        cout << "Attempting to read credentials from file: " << absolutePath << endl;
    }
    
    // Open the file
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filePath << endl;
        return false;
    }

    // Read username and API key from file
    if (!(file >> username >> apiKey)) {
        cerr << "Failed to read username and API key from file" << endl;
        file.close();
        return false;
    }

    // Close the file
    file.close();

    return true;
}

// Function to prompt user for username and API key and save to file
bool saveCredentials(const string& folderPath) {

    // Prompt user for username and API key
    cout << "Enter your username: ";
    cin >> username;
    cout << "Enter your API key: ";
    cin >> apiKey;

    // Construct the file path
    string filePath = folderPath + "/key";

    // Open the file for writing
    ofstream file(filePath);
    if (!file.is_open()) {
        cerr << "Failed to open file for writing: " << filePath << endl;
        return false;
    }

    // Write username and API key to file
    file << username << " " << apiKey << endl;

    // Close the file
    file.close();

    cout << "Credentials saved to file: " << filePath << endl;
    return true;
}

// === end creating secrets folder and key file ===

// === Begin  curl downloader ===

// Callback function to write the received data to a string
size_t writeCallback(void *contents, size_t size, size_t nmemb, string *buffer)
{
    size_t totalSize = size * nmemb;
    buffer->append((char *)contents, totalSize);
    return totalSize;
}

string getResponse (string url){
    string response;
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    // Create a CURL handle
    CURL *curl = curl_easy_init();
    if (curl) {
        // Set the URL to fetch
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // Set the user agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "621-go-Getter/0.1 (by ShadowDarkPaw on e621)");

        // Set the callback function to receive the data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            // Request successful, print the received data
            //cout << "Received data:\n" << response << endl;
        } else {
            // Request failed, print error message
            cerr << "Failed to perform request: " << curl_easy_strerror(res) << endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
    } else {
        cerr << "Failed to initialize libcurl" << endl;
    }

    // Clean up libcurl
    curl_global_cleanup();
    
    return response;
}

//TODO
string urlConstrokter(int postID)// TODO make the function witch make the url
{
    return "test";
}

//TODO
void postDownloader(int postID){
    json jsonData = json::parse(getResponse(urlConstrokter(postID)));
}

void poolDownloader(string url){
    // Parse the JSON string
    try {
        json jsonData = json::parse(getResponse(url));
        
        string poolsPath = poolsFolder + "/" + (string)jsonData["name"];
        
        createFolder(poolsPath);
        
        for (const auto& value : jsonData["post_ids"]) {
            postDownloader(value);
        }
        
    } catch (json::exception& e){
        cerr << "Failed to parse JSON: " << e.what() << endl;
    }
}

// === End curl downloader ===

// the number of posts it get at a time min 1, max 320
extern const int limit = 10;

// username and api key
string username, apiKey;

vector<string> tags; // TODO make tag

bool debugMode = false;

int main()
{
    
    // Create folder if it doesn't exist
    if (!createFolder(secretsFolder)) {
        return 1;
    }
    if (!createFolder(poolsFolder)) {
        return 1;
    }
    
    
    // Read credentials from file
    if (!readCredentials(secretsFolder)) {
        cout << "No credentials found." << endl << "Please enter your credentials:" << endl;
        if (!saveCredentials(secretsFolder)) {
            return 1;
        }
    }
    
    poolDownloader("https://e621.net/pools/38173.json");
    
    return 0;
}
