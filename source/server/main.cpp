#include <fstream>
#include <iostream>
#include <server/PersonPipeline/PersonPipeline.hpp>
#include <server/server/server.hpp>

// int main(){

//     SISD::PersonPipeline person;
//     person.init();
    
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

//     person.run(data, size);

//     delete[] data;

//     return 0;
// }

int main(){
    try
    {

        // Initialise the server.
        http::server::server s("localhost", "80", ".");

        // Run the server until stopped.
        s.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}