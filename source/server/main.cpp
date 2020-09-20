#include <fstream>
#include <iostream>
#include <server/server/server.hpp>

/**
* @brief start the server's executable. Make sure you have redis available.
*       This will start a http server on local host port 80 
*  
* @param void
* @return void
* 
*/
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