#include "main.hpp"
#include "json.hpp"

#include <curl/curl.h>

namespace fs = filesystem;

using namespace std;

using json = nlohmann::json;

//string for the curl User agent
const string userAgent = "621-go-Getter/0.1 (by ShadowDarkPaw on e621)";

// === Begin creating secrets folder and key file ===

bool createFolder(const string& folderPath) {
    struct stat info;
    if (stat(folderPath.c_str(), &info) != 0) {
        // Folder doesn't exist, try to create it
        int status = mkdir(folderPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == 0) {
            clog << "Created folder: " << folderPath << endl;
            return true;
        } else {
            cerr << "Failed to create folder: " << folderPath << endl;
            return false;
        }
    } else if (info.st_mode & S_IFDIR) {
        // Folder exists
        return true;
    } else {
        // Path exists but is not a folder
        clog << "Path already exists but is not a folder: " << folderPath << endl;
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
        clog << "Attempting to read credentials from file: " << absolutePath << endl;
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

bool is_md5_in_checklist(const string& md5) {
    const string checkFile = DataFolder + "/md5.txt";

    // Check if the file exists, if not, create it
    ifstream file(checkFile);
    if (!file.is_open()) {
        cerr << "Checklist file doesn't exist. Creating..." << endl;
        ofstream createFile(checkFile);
        if (!createFile.is_open()) {
            cerr << "Error creating checklist file: " << checkFile << endl;
            return false;
        }
        createFile.close();
        clog << "Checklist file created successfully." << endl;
        return false; // Since the file is created but empty, return false
    }
    file.close();

    // File exists, proceed with reading and checking MD5
    ifstream readFile(checkFile);
    if (!readFile.is_open()) {
        cerr << "Error opening checklist file: " << checkFile << endl;
        return false;
    }

    unordered_set<string> checklist;
    string line;
    while (getline(readFile, line)) {
        checklist.insert(line);
    }

    readFile.close();
    return checklist.find(md5) != checklist.end();
}

void add_md5_to_checklist(const string& md5) {
    const string checkFile = DataFolder + "/md5.txt";
    ofstream file(checkFile, ios::app);
    if (!file.is_open()) {
        cerr << "Error opening checklist file: " << checkFile << endl;
        return;
    }

    file << md5 << endl;
    file.close();
}

bool check_and_add_md5(const string& md5){
    if (!is_md5_in_checklist(md5)) {
        clog << "MD5 not found in checklist. Adding to the list..." << endl;
        add_md5_to_checklist(md5);
        return true;
    } else {
        clog << "MD5 found in checklist. File can be downloaded." << endl;
        return false;
    }
}


// Callback function to write the received data to a string
size_t writeCallback (void *contents, size_t size, size_t nmemb, string *buffer){
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

// Make the url to the posts
string urlConstructor(int postID){
    return "https://e621.net/posts/" + to_string(postID) + ".json";
}

void removeQuotes(string& str) {
    str.erase(remove_if(str.begin(), str.end(), [](char c) {
        return c == '"';
    }), str.end());
}

int postDownloader(const string& poolPath, int postID){
    // Wait to avoid overwhelming the server
    this_thread::sleep_for(chrono::seconds(1));
    
    // First urlConstructor takes the postid in to make the url. Then we use getResponse to the the json from the site and thenn we parse the json
    json jsonData = json::parse(getResponse(urlConstructor(postID)));
    
    string url = jsonData["post"]["file"]["url"];
    string filename = to_string(jsonData["post"]["file"]["md5"]) + "." + to_string(jsonData["post"]["file"]["ext"]);

    string filepath = poolPath + "/" + filename;
    
    removeQuotes(filepath);
    
    FILE *fp = fopen(filepath.c_str(), "wb");
    if (!fp) {
        cerr << "Error opening file for writing" << endl;
        return true;
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
            return true;
        }

        curl_easy_cleanup(curl);
    }

    fclose(fp);
    return false;
}

bool create_pool_file(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error creating pool file: " << filename << endl;
        return false;
    }
    //cout << "Pool file created successfully: " << filename << endl;
    return true;
}

void poolList(int number) {
    const string poolDir = DataFolder + "/";
    const string poolFile = poolDir + "pool.txt";
    
    // Create the pool file if it doesn't exist
     ifstream file(poolFile);
     if (!file.is_open()) {
         if (!create_pool_file(poolFile))
                return;
     }
     file.close();

    // Read existing numbers from the file into a set
    ifstream infile(poolFile);
    unordered_set<int> numbers;
    int num;
    while (infile >> num) {
        numbers.insert(num);
    }
    infile.close();

    // Check if the number is already in the set
    if (numbers.find(number) != numbers.end()) {
        clog << "Number " << number << " already exists in the pool." << endl;
        return;
    }

    // Append the number to the pool file
    ofstream outfile(poolFile, ios::app);
    if (!outfile.is_open()) {
        cerr << "Error opening pool file: " << poolFile << endl;
        return;
    }
    outfile << number << endl;
    outfile.close();
    clog << "Number added to the pool: " << number << endl;
}

void poolDownloader(string url){
    vector<int> posts;
    string poolsPath;
    // Parse the JSON string
    try {
        string response = getResponse(url);
        json jsonData = json::parse(response);
        
        poolsPath = poolsFolder + "/" + to_string(jsonData["name"]);
        removeQuotes(poolsPath);
        
        createFolder(poolsPath);
        
        for (const auto& value : jsonData["post_ids"]) {
            if (value.is_number()) {
                posts.push_back(value);
            } else {
                cerr << "Dont a int" << endl;
            }
        }
        
    } catch (json::exception& e){
        cerr << "Failed to parse JSON in poolDownloader: " << e.what() << endl;
    }
    
    // Using a range-based for loop (available in C++11 and later)
    for (const auto& postID : posts) {
        json jsonData = json::parse(getResponse(urlConstructor(postID)));
        
        if(!is_md5_in_checklist(jsonData["post"]["file"]["md5"])){
            if(!postDownloader(poolsPath, postID)){
                add_md5_to_checklist(jsonData["post"]["file"]["md5"]);
            }else{
                cerr << "error downloading the file. Please check your internet" << endl;
                return;
            }
        }else{
            //cout << "md5 has been downloaded" << endl;
        }
    }
}

// === End curl downloader ===

int extract_number_from_url(const string& url) {
    // Find the position of the last '/'
    size_t last_slash_pos = url.find_last_of('/');
    if (last_slash_pos == string::npos) {
        // URL format is invalid
        return 0;
    }

    // Extract the substring starting from last '/' to '.json'
    string sub = url.substr(last_slash_pos + 1);

    // Find the position of '.json'
    size_t json_pos = sub.find(".json");
    if (json_pos == string::npos) {
        // URL format is invalid
        return 0;
    }

    // Extract the substring containing the number
    string number_str = sub.substr(0, json_pos);

    return stoi(number_str);
}

bool isValidURL(const string& url) {
    // Regular expression for a simple URL pattern
    regex urlRegex("(https?|ftp)://[-a-zA-Z0-9+&@#/%?=~_|!:,.;]*[-a-zA-Z0-9+&@#/%=~_|]");

    return regex_match(url, urlRegex);
}

void autoDownloadPool()
{
    ifstream file(DataFolder + "/pool.txt");
        if (!file.is_open()) {
            cerr << "Error opening file: " << DataFolder << "/pool.txt" << endl;
            return;
        }
    
    string line;
    while (getline(file, line)) {
        // Process each line
        poolDownloader("https://e621.net/pools/" + line + ".json");
    }


}

// === Menu ==
void menu() {
    cout << "Menu:" << endl;
    cout << "1. Add Pool" << endl;
    cout << "2. Auto Downloed all pools" << endl;
    cout << "9. Exit" << endl;
    
    string poolAnswer = "";
    int poolNumber;
    bool isInteger;
    
    int choice;
    cout << "Enter your choice: ";
    cin >> choice;
   
    switch (choice) {
        case 1:
            while (true) {
                cout << "Please type the pool id or the url to the pool." << endl;
                cout << "type 'e' for exit" << endl;
                poolAnswer = "";
                cin >> poolAnswer;
                
                if(poolAnswer == "e")
                {
                    break;
                }
                
                // Check if the input string is a valid integer
                isInteger = true;
                for (char c : poolAnswer) {
                    if (!isdigit(c)) {
                        isInteger = false;
                        break;
                    }
                }
                
                if (isInteger) {
                    // Convert the input string to an integer
                    poolNumber = stoi(poolAnswer);
                    poolList(poolNumber);
                } else {
                    // Check if the input string is a valid URL
                    if (isValidURL(poolAnswer)) {
                        // Extract the number from the URL and pass it to poolList
                        poolNumber = extract_number_from_url(poolAnswer);
                        if (poolNumber != 0) {
                            poolList(poolNumber);
                        } else {
                            cerr << "Invalid URL format." << endl;
                        }
                    } else {
                        cerr << "Invalid input. Please enter a valid URL or a valid integer." << endl;
                    }
                }

            }
            menu();
            break;
        case 2:
            cout << "Auto Downloed all pools." << endl;
            autoDownloadPool();
            break;
        case 9:
            cout << "Exiting the program." << endl;
            exit(0);
            break;
        default:
            cerr << "Invalid choice. Please choose again." << endl;
            menu();
            break;
    }
}


void print_help() {
    cout << "Usage: program_name [OPTION]" << endl;
    cout << "Options:" << endl;
    cout << "-h, --help     Display this help message" << endl;
    cout << "-v, --version  Display version information" << endl;
    // Add more options if needed
}

void print_version() {
    cout << "Program Version 1.0" << endl;
}
// === END Menu ==

// the number of posts it get at a time min 1, max 320
//extern const int limit = 10;

// username and api key
string username, apiKey;

//vector<string> tags; // TODO make tag

bool debugMode = false;

int main(int argc, char* argv[])
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
        cerr << "No credentials found." << endl << "Please enter your credentials:" << endl;
        if (!saveCredentials(secretsFolder)) {
            return 1;
        }
    }
    
    if (argc == 1) {
        menu();
        return 0;
    }

    string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
        print_help();
    } else if (arg == "-v" || arg == "--version") {
        print_version();
    } else {
        // Invalid option, display menu
        cerr << "Invalid option. Displaying menu:" << endl;
        menu();
    }


    
    return 0;
}
