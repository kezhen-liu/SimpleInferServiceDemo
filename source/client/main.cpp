#include <fstream>
#include <iostream>

#include <client/client.hpp>

// #include <common/utility/base64.h>

// int main(){

//     std::cout<<"Test start"<<std::endl;
    
//     std::ifstream fileIn("3.jpeg", std::ios::binary | std::ios::ate);
//     if(!fileIn){
//         std::cerr << "Unable to open file!" << std::endl;
//         exit(0);
//     }
//     std::streamsize size = fileIn.tellg();
//     fileIn.seekg(0, std::ios::beg);

//     char* data = new char[size];
//     if (!fileIn.read(data, size))
//     {
//         std::cerr << "Error reading contents!" << std::endl;
//         exit(0);
//     }

//     std::cout<<"raw buffer size is "<<size<<std::endl;

//     std::string base64 = base64_encode(reinterpret_cast<const unsigned char*>(data), size);

//     std::cout << "Encoded length is "<<base64.length()<<std::endl;

//     std::cout<<"Encoded base64 is "<<std::endl;

//     std::cout<<base64<<std::endl;

//     std::string decoded = base64_decode(base64, true);

//     std::ofstream fileOut("test.jpeg", std::ios::binary);

//     std::cout << "Decoded length is " << decoded.length() <<std::endl;

//     fileOut.write(decoded.data(), decoded.length());

//     // or

//     // fileOut<<decoded; //work

//     fileOut.close();
//     fileIn.close();



//     delete[] data;

//     return 0;
// }

int main(){
    SISD::Client client;

    SISD::Client::Request req = client.formRequest({"3.jpeg"}, SISD::Client::Person);

    if(!req){
        std::cerr << "Not a valid request!"<<std::endl;
        exit(0);
    }

    std::string response = client.sendRequest(req);

    std::cout<<"Response received is "<<std::endl;
    std::cout<<response<<std::endl;

    return 0;
}