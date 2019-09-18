//
//  main.cpp
//  Bitly Test Server
//
//  Created by Jason Ghiglieri on 4/2/19.
//  Copyright © 2019 Jason Ghiglieri. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <vector>

#include <curl/curl.h>

#define PORT 80

std::string userData;



size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up) {
    for (int i = 0; i < size*nmemb; i++) {
        userData.push_back(buf[i]);
    }
    return size*nmemb;
}


int main(int argc, const char * argv[]) {
    
    int serverFileDescriptor;
    int server_socket;
    long input;
    
    struct sockaddr_in address;
    int addrLength = sizeof(address);
    
    
    
    // Create Socket file descriptor
    if ((serverFileDescriptor = socket(AF_INET,
                                       SOCK_STREAM,
                                       0)) == 0) {
        std::cerr << "Error in Socket";
        exit(EXIT_FAILURE);
    }
    
    
    // Populating struct info, for use in Socket binding
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    
    // Fixes small bug with sockaddr_in that might otherwise cause undefined behavior
    memset(address.sin_zero, '\0', sizeof(address.sin_zero));
    
    
    // Identify the socket to be used, using bind.
    if (bind(serverFileDescriptor,
             (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        std::cerr << "Error in Bind";
        exit(EXIT_FAILURE);
    }
    
    
    // Enable the socket to be able to listen for incoming connections
    if (listen(serverFileDescriptor, 10) < 0) {
        std::cerr << "Error in Listen";
        exit(EXIT_FAILURE);
    }
    
    
    // Wait until a connection is made to the socket
    while(1) {
        // HANDLE CLIENT INPUT
        
        std::cout << "\n----- Awaiting Connection -----\n";
        
        
        // Accept the incoming connection
        if ((server_socket = accept(serverFileDescriptor,
                                    (struct sockaddr *)&address,
                                    (socklen_t*)&addrLength)) < 0) {
            std::cerr << "Error in Accept";
            exit(EXIT_FAILURE);
        }
        
        
        
        // Buffer for incoming information to be stored in.
        char buffer[30000] = {0};
        // Store information received from socket in buffer
        input = read(server_socket, buffer, 30000);
        
        
        
        
        // HANDLE BIT.LY API REQUESTS
        
        // Construct the authorization token from the client input
        std::string token;
        for (int i = 5; i < 45; i++) {
            token.append(1, buffer[i]);
        }
        
        
        
        // Construct curl pointer
        CURL *curl;
        
        // Initialize curl
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        
        
        
        // URL for User request
        char* userRequest = "https://api-ssl.bitly.com/v4/user";
        
        
        // Construct Authorization token custom header
        struct curl_slist *header = NULL;
        std::string authHead = "Authorization: Bearer ";
        authHead.append(token);
        const char *head = authHead.c_str();
        header = curl_slist_append(header, head);
        
        
        
        
        // Assign custom header for future requests
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        
        
        // Request bit.ly api for user information
        curl_easy_setopt(curl, CURLOPT_URL, userRequest);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_perform(curl);
        
        
        
        
        // URL for default group request
        std::string groupReq = "https://api-ssl.bitly.com/v4/groups/";
        // Add the group guid to the url for the next request
        for (int i = 277; i < 288; i++) {
            groupReq.append(1, userData.at(i));
        }
        // finish the url for the next request, and convert it to a format usable by curl
        groupReq.append("/bitlinks?size=1");
        const char *groupRequest = groupReq.c_str();
        
        
        
        // Clear return data for next request
        userData = "";
        
        
        
        // Send Request for all links within default group associated with user.
        curl_easy_setopt(curl, CURLOPT_URL, groupRequest);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_perform(curl);
        

        
        
        // Get total number of bitlinks in string form, then convert to an integer
        std::string totalStr;
        while(userData.back() != ':') {
            if (userData.back() != '}') {
                totalStr.append(1, userData.back());
            }
            userData.pop_back();
        }
        std::reverse(totalStr.begin(), totalStr.end());
        int total = std::stoi(totalStr);
        
        
        
        
        // Get the bitlink for the last page
        std::string currLink = "";
        for (int i = 88; i < 102; i++) {
            currLink.append(1, userData.at(i));
        }
        
        
        
        //vBase form of URL for request of clicks per country for a bitlink
        std::string countryRequestOrig = "https://api-ssl.bitly.com/v4/bitlinks/";
        
        // Construct remainder of URL for country request
        std::string countryRequest = countryRequestOrig;
        countryRequest.append(currLink);
        countryRequest.append("/countries?unit=day&units=30");
        const char* realCountryRequest = countryRequest.c_str();
        
        
        
        // Clear return data for next request
        userData = "";
        
        
        
        
        // Send Request for all links within default group associated with user.
        curl_easy_setopt(curl, CURLOPT_URL, realCountryRequest);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_perform(curl);
        

        
        
        
        // Read in country code
        std::string countryCode = "";
        for (int j = 66; j < 68; j++) {
            countryCode.append(1, userData.at(j));
        }
        
        
        
        // Read in number of clicks for country
        std::string clickStr = "";
        for (int i = 79; userData.at(i) != '}'; i++) {
            if (userData.at(i) != ':') {
                clickStr.append(1, userData.at(i));
            }
        }
        float clicks = std::stof(clickStr);
        // Add clicks to average, for later calculation
        float averageClicks = 0 + clicks;
        
        
        
        // Make an easy reuasable variable for requesting individual pages
        std::string pageEnd = "&page=";
        // Loop through all the pages to acquire all the bitlinks from the group, then request clicks per country of each link and use values to calculate average
        for (int i = 2; i <= total; i++) {
            // Set up URL for next Page request
            std::string pRequest = groupReq;
            pRequest.append(pageEnd);
            std::string strInt = std::to_string(i);
            pRequest.append(strInt);
            const char* pageRequest = pRequest.c_str();
            
            
            
            // Clear return data for next request
            userData = "";
            
            
            
            
            // Send Request for next page containing link
            curl_easy_setopt(curl, CURLOPT_URL, pageRequest);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            curl_easy_perform(curl);
            
            
            
            
            
            // Get the bitlink for the last page
            currLink = "";
            for (int i = 88; i < 102; i++) {
                currLink.append(1, userData.at(i));
            }
            countryRequest = countryRequestOrig;
            countryRequest.append(currLink);
            countryRequest.append("/countries?unit=day&units=30");
            const char* realCountryRequest = countryRequest.c_str();
            
            
            
            
            // Clear return data for next request
            userData = "";
            
            
            
            
            
            // Send Request for all links within default group associated with user.
            curl_easy_setopt(curl, CURLOPT_URL, realCountryRequest);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            curl_easy_perform(curl);
            
            
            
            
            
            // Read in number of clicks for country
            clickStr = "";
            for (int i = 79; userData.at(i) != '}'; i++) {
                if (userData.at(i) != ':') {
                    clickStr.append(1, userData.at(i));
                }
            }
            clicks = std::stof(clickStr);
            // Add clicks to the average
            averageClicks += clicks;
        }
        
        
        // Calculate average clicks for country
        float divTotal = total;
        averageClicks = averageClicks / divTotal;
        
        
        
        // Cleanup curl
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        
        
        
        
        // FORMAT AND RETURN RESPONSE TO CLIENT
        
        // Construct response message
        std::string testResponse = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: ";
        
        std::string message = "Average Clicks for: ";
        message.append(countryCode);
        message.append(1, ':');
        message.append(std::to_string(averageClicks));
        int length = message.length();
        testResponse.append(1, length);
        testResponse.append("\n\n");
        testResponse.append(message);
        const char *response = testResponse.c_str();
        
        
        
        // Send response to client
        write(server_socket, response, strlen(response));
        
        
        // Clear token in case of repeated requests
        token = "";
        std::cout << "----- Response Sent -----\n";
        
        // Close socket to remove currently connected client
        close(server_socket);
    }
    return 0;
}
