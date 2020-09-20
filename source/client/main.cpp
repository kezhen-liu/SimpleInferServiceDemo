#include <iostream>
#include <boost/program_options.hpp>

#include <client/client.hpp>

struct sisdOption{
    enum{
        Predict,
        History
    } type;

    std::vector<std::string> images;
};

sisdOption parseArguments(int argc, char* argv[])
{
    using namespace boost::program_options;
    
    variables_map vm;
    options_description opt_desc("This is a Simple Inference Service Demo (SISD).\n\nExample usages: SISDClient -i 1.jpeg -i 2.jpeg\n\nOptions");
    opt_desc.add_options()
        ("help,h", "Produce this help message")
        ("image,i", value<std::vector<std::string>>(), "Input image to be inferenced. Can specify multiple times")
        ("type,t", value<std::string>(), "Request type. Value could be predict or history. Currently only predict is supported");

    store(parse_command_line(argc, argv, opt_desc), vm);
    notify(vm);

    if (vm.count("help")) {
        std::cout << opt_desc << std::endl;
        exit(0);
    }

    sisdOption ret;
    if (vm.count("type")) {
        std::string type = vm["type"].as<std::string>();
        if(type == "predict"){
            ret.type = sisdOption::Predict;
        }
        else if(type == "history"){
            ret.type = sisdOption::History;
            std::cout<< "Currently the history query is not supported. Exit" << std::endl;
            exit(0);
        }
        else{
            std::cout<< "Invalid request type. Value could be predict or history. Exit" << std::endl;
            exit(0);
        }
    } else {
        std::cout << "Request type unset. Defaults to predict"<<std::endl;
        ret.type = sisdOption::Predict;
    }
    if(ret.type == sisdOption::Predict){
        if (vm.count("image")) {
            ret.images = vm["image"].as<std::vector<std::string>>();
        } else {
            std::cerr << "Predict request requires at least 1 input image! Exit" << std::endl;
            exit(0);
        }
    }
    return ret;
}

int main(int argc, char* argv[]){

    sisdOption opt = parseArguments(argc, argv);

    SISD::Client client;

    SISD::Client::Request req = client.formRequest(opt.images, SISD::Client::Person);

    if(!req){
        std::cerr << "Not a valid request!"<<std::endl;
        exit(0);
    }

    std::string response = client.sendRequest(req);

    std::cout<<"Response received is "<<std::endl;
    std::cout<<response<<std::endl;

    return 0;
}