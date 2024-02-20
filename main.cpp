#include "main.h"

#include <curl/curl.h>

using namespace std;

// === Begin  curl downloader ===

// Callback function to write the received data to a string
size_t writeCallback(void *contents, size_t size, size_t nmemb, std::string *buffer) {
    size_t totalSize = size * nmemb;
    buffer->append((char *)contents, totalSize);
    return totalSize;
}

string urlConstrokter(string test)// TODO make the function witch make the url
{
    return test;
}

int poolDownloader()// TODO make pool downloader
{   
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    // Create a CURL handle
    CURL *curl = curl_easy_init();
    if (curl) {
        // Set the URL to fetch
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.example.com");

        // Set the callback function to receive the data
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            // Request successful, print the received data
            std::cout << "Received data:\n" << response << std::endl;
        } else {
            // Request failed, print error message
            std::cerr << "Failed to perform request: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize libcurl" << std::endl;
    }

    // Clean up libcurl
    curl_global_cleanup();

    return 0;
}

// === End  curl downloader ===

// === Begin creating secrets folder and key file ===

// Function to create folder if it doesn't exist
bool createFolder(const std::string& folderPath) {
    struct stat info;
    if (stat(folderPath.c_str(), &info) != 0) {
        // Folder doesn't exist, try to create it
        int status = mkdir(folderPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == 0) {
            std::cout << "Created folder: " << folderPath << std::endl;
            return true;
        } else {
            std::cerr << "Failed to create folder: " << folderPath << std::endl;
            return false;
        }
    } else if (info.st_mode & S_IFDIR) {
        // Folder exists
        // cout << "Folder already exists: " << folderPath << endl;
        return true;
    } else {
        // Path exists but is not a folder
        std::cerr << "Path already exists but is not a folder: " << folderPath << std::endl;
        return false;
    }
}


// Function to read username and API key from file
bool readCredentials(const std::string& folderPath) {
    // Construct the file path
    std::string filePath = folderPath + "/key";

    // Open the file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << endl;
        return false;
    }

    // Read username and API key from file
    if (!(file >> username >> apiKey)) {
        std::cerr << "Failed to read username and API key from file" << endl;
        file.close();
        return false;
    }

    // Close the file
    file.close();

    return true;
}

// Function to prompt user for username and API key and save to file
bool saveCredentials(const std::string& folderPath) {

    // Prompt user for username and API key
    std::cout << "Enter your username: ";
    std::cin >> username;
    std::cout << "Enter your API key: ";
    std::cin >> apiKey;

    // Construct the file path
    std::string filePath = folderPath + "/key";

    // Open the file for writing
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    // Write username and API key to file
    file << username << " " << apiKey << std::endl;

    // Close the file
    file.close();

    std::cout << "Credentials saved to file: " << filePath << std::endl;
    return true;
}

// === end creating secrets folder and key file ===

// the number of posts it get at a time min 1, max 320
extern const int limit = 10;

// username and api key
string username, apiKey;

vector<string> tags; // TODO make tag

int main() {
    //path to the secrets folder
    const string folderPath = "secrets"; 

    // Create folder if it doesn't exist
    if (!createFolder(folderPath)) {
        return 1;
    }

    // Read credentials from file
    if (!readCredentials(folderPath)) { 
        cout << "No credentials found." << endl << "Please enter your credentials:" << endl;
        if (!saveCredentials(folderPath)) {
            return 1;
        }   
    }

    return 0;
}
