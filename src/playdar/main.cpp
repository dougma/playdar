#include "playdar/application.h"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <iterator>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include "playdar/playdar_request_handler.h"

#include <boost/algorithm/string.hpp>



using namespace std;
namespace po = boost::program_options;

// global !
MyApplication * app = 0;

void sigfunc(int sig)
{
    cout << "Signal handler." << endl;
    if(app) app->shutdown(sig);
}

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
    return os;
}



void start_http_server(string ip, int port, int conc, MyApplication *app)
{
    cout << "HTTP server starting on: http://" << ip << ":" << port << "/" << endl;
    moost::http::server<playdar_request_handler> 
        s(ip, port, conc);
    s.request_handler().init(app);
    // tell app how to stop the http server:
    app->set_http_stopper(boost::bind(
        &moost::http::server<playdar_request_handler>::stop, &s));
    s.run();
    cout << "http_server thread exiting." << endl; 
}

int main(int ac, char *av[])
{

    po::options_description generic("Generic options");
    generic.add_options()
        ("config,c",    po::value<string>(),
                        "path to config file")
        ("version,v",   "print version string")
        ("help,h",      "print this message")
        ;
/*
            ("name",        po::value<string>(),
                "Name used for your collection on the network")
            ("db",          po::value<string>(),
                "Path to your database file")
            ("http_port",   po::value<int>(&opt)->default_value(8888),
                "Port used for local webserver")
            ;
*/
    po::options_description cmdline_options;
    cmdline_options.add(generic);

    po::options_description visible("Playdar configuration");
    visible.add(generic);

    po::variables_map vm;
    po::parsed_options parsedopts_cmd = 
        po::command_line_parser(ac, av).
        options(cmdline_options).run();
        
    store(parsedopts_cmd, vm);
    notify(vm);

    if (vm.count("help")) {
        cout << visible << "\n";
        return 0;
    }
    if (vm.count("version")) {
        cout << "TODO\n";
        return 0;
    }
    
    if(!vm.count("config"))
    {
        cerr << "You must use a config file." << endl;
        return 1;
    }
    string configfile = vm["config"].as<string>();
    cout << "Using config file: " << configfile << endl;
            
    playdar::Config conf(configfile);
    if(conf.get<string>("name", "YOURNAMEHERE")=="YOURNAMEHERE")
    {
        cerr << "Please edit " << configfile << endl;
        cerr << "YOURNAMEHERE is not a valid name." << endl;
		cout << "Autodetecting name: " << conf.name() << endl;
    }
    
    try 
    {
        app = new MyApplication(conf);
        
#ifndef WIN32
        /// this might not compile on windows?
        struct sigaction setmask;
        sigemptyset( &setmask.sa_mask );
        setmask.sa_handler = sigfunc;
        setmask.sa_flags   = 0;
        sigaction( SIGHUP, &setmask, (struct sigaction *) NULL );
        sigaction( SIGINT,  &setmask, (struct sigaction *) NULL );
        /// probably need to look for WM_BLAHWTFMSG or something.
#endif
        // start http server:
        string ip = "0.0.0.0"; 
        boost::thread http_thread(
            boost::bind(&start_http_server, 
                        ip, app->conf()->get<int>("http_port", 0),
                        boost::thread::hardware_concurrency()+1,
                        app));
        
        http_thread.join();
        cout << "HTTP server finished, destructing app..." << endl;
        delete(app);
        cout << "App deleted." << endl;
        return 0;
    }
    catch(exception& e)
    {
        cout << "Playdar main exception: " << e.what() << "\n";
        return 1;
    }    
}
