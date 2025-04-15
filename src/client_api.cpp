// client.cpp
#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>

using json = nlohmann::json;
using namespace std;

string leaderURL, serverUrl;
int portOffset = 2048; // Offset for API port numbers

// A callback function to capture HTTP responses
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper function to validate the command format.
bool validCommand(const string &cmd) {
    istringstream iss(cmd);
    string token;
    if (!(iss >> token)) return false;
    
    if (token == "CREATE") {
        // Expect a filename.
        string filename;
        return bool(iss >> filename);
    } else if (token == "WRITE") {
        // Expect an id and a non-empty message
        int id;
        if (!(iss >> id)) return false;
        string message;
        getline(iss, message);
        // Remove potential leading spaces.
        size_t pos = message.find_first_not_of(" ");
        return (pos != string::npos);
    } else if (token == "READ") {
        // Expect an id
        int id;
        return bool(iss >> id);
    } else if (token == "APPEND") {
        // Expect an id and a non-empty message.
        int id;
        if (!(iss >> id)) return false;
        string message;
        getline(iss, message);
        size_t pos = message.find_first_not_of(" ");
        return (pos != string::npos);
    }
    return false;
}

// Sends a POST request to /command endpoint with a JSON payload containing a command.
void Command(const string &serverUrl, const string &command) {
    getLeader(serverUrl);

    // Validate the command structure.
    if (!validCommand(command)) {
        cerr << "[ERROR] Command does not follow the required format." << endl;
        cerr << "Expected formats:" << endl;
        cerr << "  CREATE <filename>" << endl;
        cerr << "  WRITE <id> <message to be written>" << endl;
        cerr << "  READ <id>" << endl;
        cerr << "  APPEND <id> <message to be appended>" << endl;
        return;
    }
    
    // Initialize CURL for HTTP requests.
    CURL *curl;
    CURLcode res;
    string readBuffer;
    json payload = { {"command", command} };
    string payloadStr = payload.dump();

    curl = curl_easy_init();
    if(curl) {
        string url = leaderURL + "/command";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            cerr << "[ERROR] curl_easy_perform() failed: " 
                 << curl_easy_strerror(res) << endl;
        } else {
            cout << "[INFO] Append response: " << readBuffer << endl;
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

// Sends a GET request to /status endpoint to retrieve node status.
void getStatus(const string &serverUrl) {
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        string url = serverUrl + "/status";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Set timeout to 5 seconds
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Set connection timeout to 5 seconds
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            if(res == CURLE_OPERATION_TIMEDOUT) {
                cerr << "[ERROR] Request timed out." << endl;
            }
            else if(res == CURLE_COULDNT_CONNECT) {
                cerr << "[ERROR] Could not connect to server." << endl;
            }
            else {
                cerr << "[ERROR] curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            }
            curl_easy_cleanup(curl);
            return;
        } else {
            cout << "[INFO] Server Response: " << readBuffer << endl;
        }
        curl_easy_cleanup(curl);
    }
}

// Sends a GET request to /leader endpoint to retrieve the current leader info.
void getLeader(const string &serverUrl) {
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        string url = serverUrl + "/leader";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Set timeout to 5 seconds
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Set connection timeout to 5 seconds
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            if(res == CURLE_OPERATION_TIMEDOUT) {
                cerr << "[ERROR] Request timed out." << endl;
            }
            else if(res == CURLE_COULDNT_CONNECT) {
                cerr << "[ERROR] Could not connect to server." << endl;
            }
            else {
                cerr << "[ERROR] curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            }
            curl_easy_cleanup(curl);
            return;
        } else {
            cout << "[INFO] Server Response: " << readBuffer << endl;
        }
        curl_easy_cleanup(curl);
    }

    // Parse the JSON response to extract leader information
    json responseJson = json::parse(readBuffer);
    if (responseJson.contains("leaderIp") && responseJson.contains("leaderHTTPPort")) {
        string leaderIp = responseJson["leaderIp"];
        int leaderHTTPPort = responseJson["leaderHTTPPort"];
        int leaderPort = responseJson["leaderPort"]; 
        cout << "[INFO] Current Leader: " << leaderIp << ":" << leaderPort << ", HTTP : " << leaderHTTPPort << endl;
        leaderURL = "http://" + leaderIp + ":" + to_string(leaderHTTPPort);
        cout << "[INFO] Leader URL: " << leaderURL << endl;
    } else {
        cout << "[ERROR] Leader information not found in the response." << endl;
    }
}

int main(int argc, char* argv[]) {
    // The client now interacts with the server using HTTP.
    // Usage: ./client http://<server_ip>:<api_port>
    if(argc != 2) {
        cerr << "Usage: " << argv[0] << " <server_api_url>" << endl;
        cerr << "Example: ./" << argv[0] << "http://127.0.0.1:8080" << endl;
        return EXIT_FAILURE;
    }

    serverUrl = argv[1];
    int choice = 0;
    while (true) {
        cout << "\n========== Client Menu ==========" << endl;
        cout << "1. Command" << endl;
        cout << "2. Get Status" << endl;
        cout << "3. Get Leader" << endl;
        cout << "4. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore(); // Clear the newline from input
        if(choice == 1) {
            cout << "Expected formats:" << endl;
            cout << "  CREATE" << endl;
            cout << "  WRITE <id> <message to be written>" << endl;
            cout << "  READ <id>" << endl;
            cout << "  APPEND <id> <message to be appended>" << endl;
            cout << "Enter command to : ";
            string command;
            getline(cin, command);
            Command(leaderURL, command);
        } else if(choice == 2) {
            cout << "Enter server URL to get Status: ";
            cin >> serverUrl;
            getStatus(serverUrl);
        } else if(choice == 3) {
            cout << "Currently speaking to server: " << serverUrl << endl;
            getLeader(serverUrl);
        } else if(choice == 4) {
            cout << "Exiting client." << endl;
            break;
        } else {
            cout << "Invalid option. Please try again." << endl;
        }
    }
    return 0;
}
