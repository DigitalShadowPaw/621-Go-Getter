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

bool is_md5_in_checklist(const string& md5, const string& checkFile) {
    ifstream file(checkFile);
    if (!file.is_open()) {
        cerr << "Error opening checklist file: " << checkFile << endl;
        return false;
    }

    unordered_set<string> checklist;
    string line;
    while (getline(file, line)) {
        checklist.insert(line);
    }

    file.close();

    return checklist.find(md5) != checklist.end();
}

void add_md5_to_checklist(const string& md5, const string& checkFile) {
    ofstream file(checkFile, ios::app);
    if (!file.is_open()) {
        cerr << "Error opening checklist file: " << checkFile << endl;
        return;
    }

    file << md5 << endl;
    file.close();
}

bool check_and_add_md5(const string& md5)
{
    const string checkFile = DataFolder + "/md5.txt";
    if (!is_md5_in_checklist(md5, checkFile)) {
        cout << "MD5 not found in checklist. Adding to the list..." << endl;
        add_md5_to_checklist(md5, checkFile);
        return true;
    } else {
        cout << "MD5 found in checklist. File can be downloaded." << endl;
        return false;
    }
}


// Callback function to write the received data to a string
size_t writeCallback (void *contents, size_t size, size_t nmemb, string *buffer)
{
    size_t totalSize = size * nmemb;
    buffer->append((char *)contents, totalSize);
    return totalSize;
}

// Download data
size_t write_data (void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


string getResponse (string url){
    // Wait to avoid overwhelming the server
    this_thread::sleep_for(chrono::seconds(1));
    
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

string urlConstructor(int postID)// TODO make the function witch make the url
{
    return "https://e621.net/posts/" + to_string(postID) + ".json";
}

void postDownloader(const string url, const string filename, const string& poolPath, int postID, ofstream& outputFile){
    // Wait to avoid overwhelming the server
    this_thread::sleep_for(chrono::seconds(1));

    //string testurl = urlConstructor(postID);
    //string res = getResponse(testurl);
    //json jsonData = json::parse(res);
    
    //string fileToDownload = jsonData["post"]["file"]["url"];
    //string filename = jsonData["post"]["file"]["md5"];

    string filepath = poolPath + "/" + filename;

    FILE *fp = fopen(filepath.c_str(), "wb");
    if (!fp) {
        cerr << "Error opening file for writing" << endl;
        return;
    }

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "621-go-Getter/0.1 (by ShadowDarkPaw on e621)");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "Failed to download file: " << curl_easy_strerror(res) << endl;
        }

        curl_easy_cleanup(curl);
    }

    fclose(fp);
}

void removeDoubleQuotes(string& str) {
    str.erase(remove_if(str.begin(), str.end(), [](char c) {
        return c == '"';
    }), str.end());
}

void poolDownloader(string url){
    vector<int> posts;
    string poolsPath;
    // Parse the JSON string
    try {
        string response = getResponse(url);
        json jsonData = json::parse(response);
        
        poolsPath = poolsFolder + "/" + to_string(jsonData["name"]);
        removeDoubleQuotes(poolsPath);
        
        createFolder(poolsPath);
        
        for (const auto& value : jsonData["post_ids"]) {
            if (value.is_number()) {
                posts.push_back(value);
            } else {
                cout << "Dont a int" << endl;
            }
        }
        
    } catch (json::exception& e){
        cerr << "Failed to parse JSON in poolDownloader: " << e.what() << endl;
    }
    
    // Using a range-based for loop (available in C++11 and later)
    for (const auto& postID : posts) {
        //cout << "Post ID: " << post << endl;
        string testurl = urlConstructor(postID);
        string res = getResponse(testurl);
        json jsonData = json::parse(res);
        
        if (check_and_add_md5(jsonData["post"]["file"]["md5"])){
        
            string fileToDownload = jsonData["post"]["file"]["url"];
            string filename = jsonData["post"]["file"]["md5"];
            filename = filename + ".png";
            
            ofstream outputFile(poolsPath + "/" + filename, ios::binary);
            if (!outputFile.is_open()) {
            cerr << "Failed to open file for writing" << endl;
                cin.ignore();
            }
            
            postDownloader(fileToDownload, filename, poolsPath, postID, outputFile);
            //TODO here it cant close the file / it creates the file it need to download and the fail?
            // Close the file after the function call
            outputFile.close();
        }else{
            
        }
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
    if (!createFolder(DataFolder)) {
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
