#ifndef __PLAYDAR_REQUEST_HANDLER_H__
#define __PLAYDAR_REQUEST_HANDLER_H__

#include <vector>
#include <string>
#include <iostream>

#include <moost/http.hpp>

#include <uriparser/Uri.h>
#include <uriparser/UriBase.h>
#include <uriparser/UriDefsAnsi.h>
#include <uriparser/UriDefsConfig.h>
#include <uriparser/UriDefsUnicode.h>
#include <uriparser/UriIp4.h>

#include "playdar/library.h"
#include "playdar/application.h"
#include "playdar/auth.hpp"
         
class playdar_request_handler : public moost::http::request_handler_base<playdar_request_handler>
{
public:
    void init(MyApplication * app);
    void handle_request(const moost::http::request& req, moost::http::reply& rep);
    MyApplication * app() { return m_app; }
    
private:

    playdar::auth * m_pauth;    
    // un%encode
    string unescape(string s)
    {
        char * n = (char *) malloc(sizeof(char) * s.length()+1);
        memcpy(n, s.data(), s.length());
        uriUnescapeInPlaceA(n);
        string ret(n);
        delete(n);
        return ret;
    }

    string sid_to_url(source_uid sid); 
    int collect_params(const string & url, map<string,string> & vars);
    void collect_parts(const string & url, vector<string>& parts);
    void handle_json_query(string query, const moost::http::request& req, moost::http::reply& rep);
    void handle_rest_api(map<string, string> querystring, const moost::http::request& req, moost::http::reply& rep, string permissions);

    void serve_body(string reply, const moost::http::request& req, moost::http::reply& rep);
    void serve_stats(const moost::http::request& req, moost::http::reply& rep);
    void serve_static_file(const moost::http::request& req, moost::http::reply& rep);
    void serve_track(const moost::http::request& req, moost::http::reply& rep, int tid);
    void serve_sid(const moost::http::request& req, moost::http::reply& rep, source_uid sid);
    void serve_dynamic( const moost::http::request& req, moost::http::reply& rep, 
                                        string tpl, map<string,string> vars);

    void handle_auth1( const moost::http::request& req, moost::http::reply& );
    void handle_auth2( const moost::http::request& req, moost::http::reply& );
    void handle_root( const moost::http::request& req, moost::http::reply& );
    void handle_shutdown( const moost::http::request& req, moost::http::reply& );
    void handle_settings( const moost::http::request& req, moost::http::reply& );
    void handle_queries( const moost::http::request& req, moost::http::reply& );
    void handle_serve( const moost::http::request& req, moost::http::reply& );
    void handle_sid( const moost::http::request& req, moost::http::reply& );
    void handle_quickplay( const moost::http::request& req, moost::http::reply& );
    void handle_api( const moost::http::request& req, moost::http::reply& );
    
    string handle_queries_root();
    MyApplication * m_app;
   
    typedef std::map< const string, boost::function<void( const moost::http::request&, moost::http::reply&)> > HandlerMap;
    HandlerMap m_urlHandlers;

};


#endif // __PLAYDAR_REQUEST_HANDLER_H__
