// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Macoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcclient.h"

#include "rpcprotocol.h"
#include "util.h"
#include "ui_interface.h"
#include "chainparams.h" // for Params().RPCPort()


#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include "json/json_spirit_writer_template.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace json_spirit;

static bool oauth2debug = false;
Object CallHTTP(const string& host, const string& url, const string& method, const map<string,string>& params, const map<string,string>& header, bool fUseSSL)
{
    // Connect to localhost
    asio::io_service io_service;
    ssl::context context(io_service, ssl::context::sslv23);
    context.set_options(ssl::context::no_sslv2);
    asio::ssl::stream<asio::ip::tcp::socket> sslStream(io_service, context);
    SSLIOStreamDevice<asio::ip::tcp> d(sslStream, fUseSSL);
    iostreams::stream< SSLIOStreamDevice<asio::ip::tcp> > stream(d);
    string port = fUseSSL ? "443" : "80";
    bool fConnected = d.connect(host, port);
    if (!fConnected) {
        throw runtime_error("couldn't connect to http server");
    }

    // Send request
    string strRequest;
    if (method == "GET") {
         strRequest = HTTPGetUrl(host, url, params, header);
    } else {
        strRequest = HTTPPostUrl(host, url, params, header);
    }
    if (oauth2debug) {
         cout << "req: " << strRequest << endl;
    }
    stream << strRequest << std::flush;

    // Receive HTTP reply status
    int nProto = 0;
    int nStatus = ReadHTTPStatus(stream, nProto);

    // Receive HTTP reply message headers and body
    map<string, string> mapHeaders;
    string strReply;
    ReadHTTPMessage(stream, mapHeaders, strReply, nProto);
    if (oauth2debug) {
        cout << "reply: " << strReply << endl;
    }
    if (nStatus == HTTP_UNAUTHORIZED)
        throw runtime_error("incorrect httpuser or httppassword (authorization failed)");
    else if (nStatus >= 400 && nStatus != HTTP_BAD_REQUEST && nStatus != HTTP_NOT_FOUND && nStatus != HTTP_INTERNAL_SERVER_ERROR)
        throw runtime_error(strprintf("server returned HTTP error %d", nStatus));
    else if (strReply.empty())
        throw runtime_error("no response from server");

    // Parse reply
    Value valReply;
    if (!read_string(strReply, valReply))
        throw runtime_error("couldn't parse reply from server");
    const Object& reply = valReply.get_obj();
    if (reply.empty())
        throw runtime_error("expected reply to have result, error and id properties");
    return reply;
}


Object CallRPC(const string& strMethod, const Array& params)
{
    if (mapArgs["-rpcuser"] == "" && mapArgs["-rpcpassword"] == "")
        throw runtime_error(strprintf(
            _("You must set rpcpassword=<password> in the configuration file:\n%s\n"
              "If the file does not exist, create it with owner-readable-only file permissions."),
                GetConfigFile().string().c_str()));

    // Connect to localhost
    bool fUseSSL = GetBoolArg("-rpcssl", false);
    asio::io_service io_service;
    ssl::context context(io_service, ssl::context::sslv23);
    context.set_options(ssl::context::no_sslv2);
    asio::ssl::stream<asio::ip::tcp::socket> sslStream(io_service, context);
    SSLIOStreamDevice<asio::ip::tcp> d(sslStream, fUseSSL);
    iostreams::stream< SSLIOStreamDevice<asio::ip::tcp> > stream(d);

    bool fWait = GetBoolArg("-rpcwait", false); // -rpcwait means try until server has started
    do {
        bool fConnected = d.connect(GetArg("-rpcconnect", "127.0.0.1"), GetArg("-rpcport", itostr(Params().RPCPort())));
        if (fConnected) break;
        if (fWait)
            MilliSleep(1000);
        else
            throw runtime_error("couldn't connect to server");
    } while (fWait);

    // HTTP basic authentication
    string strUserPass64 = EncodeBase64(mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"]);
    map<string, string> mapRequestHeaders;
    mapRequestHeaders["Authorization"] = string("Basic ") + strUserPass64;

    // Send request
    string strRequest = JSONRPCRequest(strMethod, params, 1);
    string strPost = HTTPPost(strRequest, mapRequestHeaders);
    stream << strPost << std::flush;

    // Receive HTTP reply status
    int nProto = 0;
    int nStatus = ReadHTTPStatus(stream, nProto);

    // Receive HTTP reply message headers and body
    map<string, string> mapHeaders;
    string strReply;
    ReadHTTPMessage(stream, mapHeaders, strReply, nProto);

    if (nStatus == HTTP_UNAUTHORIZED)
        throw runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
    else if (nStatus >= 400 && nStatus != HTTP_BAD_REQUEST && nStatus != HTTP_NOT_FOUND && nStatus != HTTP_INTERNAL_SERVER_ERROR)
        throw runtime_error(strprintf("server returned HTTP error %d", nStatus));
    else if (strReply.empty())
        throw runtime_error("no response from server");

    // Parse reply
    Value valReply;
    if (!read_string(strReply, valReply))
        throw runtime_error("couldn't parse reply from server");
    const Object& reply = valReply.get_obj();
    if (reply.empty())
        throw runtime_error("expected reply to have result, error and id properties");

    return reply;
}

template<typename T>
void ConvertTo(Value& value, bool fAllowNull=false)
{
    if (fAllowNull && value.type() == null_type)
        return;
    if (value.type() == str_type)
    {
        // reinterpret string as unquoted json value
        Value value2;
        string strJSON = value.get_str();
        if (!read_string(strJSON, value2))
            throw runtime_error(string("Error parsing JSON:")+strJSON);
        ConvertTo<T>(value2, fAllowNull);
        value = value2;
    }
    else
    {
        value = value.get_value<T>();
    }
}

// Convert strings to command-specific RPC representation
Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    Array params;
    BOOST_FOREACH(const std::string &param, strParams)
        params.push_back(param);

    int n = params.size();

    //
    // Special case non-string parameter types
    //
    if (strMethod == "stop"                   && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "getaddednodeinfo"       && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "setgenerate"            && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "setgenerate"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "getnetworkhashps"       && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "getnetworkhashps"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "sendtoaddress"          && n > 1) ConvertTo<double>(params[1]);
    if (strMethod == "settxfee"               && n > 0) ConvertTo<double>(params[0]);
    if (strMethod == "getreceivedbyaddress"   && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "getreceivedbyaccount"   && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "listreceivedbyaddress"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "listreceivedbyaddress"  && n > 1) ConvertTo<bool>(params[1]);
    if (strMethod == "listreceivedbyaccount"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "listreceivedbyaccount"  && n > 1) ConvertTo<bool>(params[1]);
    if (strMethod == "getbalance"             && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "getblockhash"           && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "move"                   && n > 2) ConvertTo<double>(params[2]);
    if (strMethod == "move"                   && n > 3) ConvertTo<boost::int64_t>(params[3]);
    if (strMethod == "sendfrom"               && n > 2) ConvertTo<double>(params[2]);
    if (strMethod == "sendfrom"               && n > 3) ConvertTo<boost::int64_t>(params[3]);
    if (strMethod == "listtransactions"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "listtransactions"       && n > 2) ConvertTo<boost::int64_t>(params[2]);
    if (strMethod == "listaccounts"           && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "walletpassphrase"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "getblocktemplate"       && n > 0) ConvertTo<Object>(params[0]);
    if (strMethod == "listsinceblock"         && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "sendmany"               && n > 1) ConvertTo<Object>(params[1]);
    if (strMethod == "sendmany"               && n > 2) ConvertTo<boost::int64_t>(params[2]);
    if (strMethod == "addmultisigaddress"     && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "addmultisigaddress"     && n > 1) ConvertTo<Array>(params[1]);
    if (strMethod == "createmultisig"         && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "createmultisig"         && n > 1) ConvertTo<Array>(params[1]);
    if (strMethod == "listunspent"            && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "listunspent"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "listunspent"            && n > 2) ConvertTo<Array>(params[2]);
    if (strMethod == "getblock"               && n > 1) ConvertTo<bool>(params[1]);
    if (strMethod == "getrawtransaction"      && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "createrawtransaction"   && n > 0) ConvertTo<Array>(params[0]);
    if (strMethod == "createrawtransaction"   && n > 1) ConvertTo<Object>(params[1]);
    if (strMethod == "signrawtransaction"     && n > 1) ConvertTo<Array>(params[1], true);
    if (strMethod == "signrawtransaction"     && n > 2) ConvertTo<Array>(params[2], true);
    if (strMethod == "sendrawtransaction"     && n > 1) ConvertTo<bool>(params[1], true);
    if (strMethod == "gettxout"               && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "gettxout"               && n > 2) ConvertTo<bool>(params[2]);
    if (strMethod == "lockunspent"            && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "lockunspent"            && n > 1) ConvertTo<Array>(params[1]);
    if (strMethod == "importprivkey"          && n > 2) ConvertTo<bool>(params[2]);
    if (strMethod == "verifychain"            && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "verifychain"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
    if (strMethod == "keypoolrefill"          && n > 0) ConvertTo<boost::int64_t>(params[0]);
    if (strMethod == "getrawmempool"          && n > 0) ConvertTo<bool>(params[0]);

    return params;
}

int CommandLineRPC(int argc, char *argv[])
{
    string strPrint;
    int nRet = 0;
    try
    {
        // Skip switches
        while (argc > 1 && IsSwitchChar(argv[1][0]))
        {
            argc--;
            argv++;
        }

        // Method
        if (argc < 2)
            throw runtime_error("too few parameters");
        string strMethod = argv[1];

        // Parameters default to strings
        std::vector<std::string> strParams(&argv[2], &argv[argc]);
        Array params = RPCConvertValues(strMethod, strParams);

        // Execute
        Object reply = CallRPC(strMethod, params);

        // Parse reply
        const Value& result = find_value(reply, "result");
        const Value& error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            strPrint = "error: " + write_string(error, false);
            int code = find_value(error.get_obj(), "code").get_int();
            nRet = abs(code);
        }
        else
        {
            // Result
            if (result.type() == null_type)
                strPrint = "";
            else if (result.type() == str_type)
                strPrint = result.get_str();
            else
                strPrint = write_string(result, true);
        }
    }
    catch (boost::thread_interrupted) {
        throw;
    }
    catch (std::exception& e) {
        strPrint = string("error: ") + e.what();
        nRet = abs(RPC_MISC_ERROR);
    }
    catch (...) {
        PrintExceptionContinue(NULL, "CommandLineRPC()");
        throw;
    }

    if (strPrint != "")
    {
        fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
    }
    return nRet;
}

std::string HelpMessageCli(bool mainProgram)
{
    string strUsage;
    if(mainProgram)
    {
        strUsage += _("Options:") + "\n";
        strUsage += "  -?                     " + _("This help message") + "\n";
        strUsage += "  -conf=<file>           " + _("Specify configuration file (default: macoin.conf)") + "\n";
        strUsage += "  -datadir=<dir>         " + _("Specify data directory") + "\n";
        strUsage += "  -testnet               " + _("Use the test network") + "\n";
        strUsage += "  -regtest               " + _("Enter regression test mode, which uses a special chain in which blocks can be "
                                                    "solved instantly. This is intended for regression testing tools and app development.") + "\n";
    } else {
        strUsage += _("RPC client options:") + "\n";
    }

    strUsage += "  -rpcconnect=<ip>       " + _("Send commands to node running on <ip> (default: 127.0.0.1)") + "\n";
    strUsage += "  -rpcport=<port>        " + _("Connect to JSON-RPC on <port> (default: 8332 or testnet: 18332)") + "\n";
    strUsage += "  -rpcwait               " + _("Wait for RPC server to start") + "\n";
    strUsage += "  -rpcuser=<user>        " + _("Username for JSON-RPC connections") + "\n";
    strUsage += "  -rpcpassword=<pw>      " + _("Password for JSON-RPC connections") + "\n";

    if(mainProgram)
    {
        strUsage += "\n" + _("SSL options: (see the Macoin Wiki for SSL setup instructions)") + "\n";
        strUsage += "  -rpcssl                " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n";
    }

    return strUsage;
}

static std::string strClientId   = "demoapp";
static std::string strClientPass = "demopass";
static std::string strAccessTokenURL = "/oauth/token";
static std::string strHost = "zc.macoin.org";
static std::string strAccessToken = "";
static std::string strRefreshToken = "";
static bool bIsSSL = true;
static int nExpireIn = 0;
void OAuth2::init(const std::string& clientId, const std::string& clientPass) {
     strClientId = clientId;
     strClientPass = clientPass;
}

Object OAuth2::login(const string& username, const string& password)
{
    map<string, string> header;
    map<string, string> params;
    params["client_id"] = strClientId;
    params["client_secret"] = strClientPass;
    params["grant_type"] = "password";
    params["username"] = username;
    params["password"] = password;
    const Object& r = CallHTTP(strHost, strAccessTokenURL, "POST", params, header, bIsSSL);
    if (oauth2debug) {
        cout << "call token url end" << endl;
    }
    const Value& access_token  = find_value(r, "access_token");
    if (access_token.type() != null_type) {
       strAccessToken  = access_token.get_str();
       strRefreshToken = find_value(r, "refresh_token").get_str();
       nExpireIn        = find_value(r, "expires_in").get_int();
       if (oauth2debug) {
            cout << "call access token = " << access_token.get_str() << endl;
       }
       return r;
    }
    if (oauth2debug) {
        cout << "call access token error" << endl;
    }
    return r;
}

/**
 * 刷新授权信息
 * 此处以SESSION形式存储做演示，实际使用场景请做相应的修改
 */
Object OAuth2::refreshToken()
{
    map<string, string> header;
    map<string, string> params;
    params["client_id"] = strClientId;
    params["client_secret"] = strClientPass;
    params["grant_type"] = "refresh_token";
    params["refresh_token"] = strRefreshToken;
    const Object& r = CallHTTP(strHost, strAccessTokenURL, "POST", params, header, bIsSSL);
    if (oauth2debug) {
        cout << "refresh token url end" << endl;
    }
    const Value& access_token  = find_value(r, "access_token");
    if (access_token.type() != null_type)  {
        strAccessToken = access_token.get_str();
        nExpireIn      = find_value(r, "expires_in").get_int();
        if (oauth2debug) {
            cout << "refresh access token = " << access_token.get_str() << endl;
        }
        return r;
    }
    if (oauth2debug) {
        cout << "refresh access token error" << endl;
    }
    return r;
}

/**
 * 验证授权是否有效
 */
bool OAuth2::checkOAuthValid()
{
    map<string,string> params;
    const Object& r = Macoin::api("user/hello", params);
    const Value& hello  = find_value(r, "hello");
    if (hello.type() != null_type && hello.get_str() == "word") {
        return true;
    }
    OAuth2::clear();
    return false;
}
 
string OAuth2::getAccessToken() {
    return strAccessToken;
}

string OAuth2::getClientId() {
    return strClientId;
}

void OAuth2::clear() {
     strAccessToken = "";
     strRefreshToken = "";
     nExpireIn = 0;
}

void OAuth2::enableDebug() {
    oauth2debug = true;
}

void OAuth2::disableDebug() {
    oauth2debug = false;
}

string Macoin::strApiUrl = "/api";
bool Macoin::debug = false;
string Macoin::strHost = "zc.macoin.org";
bool   Macoin::bIsSSL = true;
int    Macoin::seq = 0;

Object Macoin::api(const string& command, map<string,string> params, const string& method)
{   
    if (oauth2debug) {
        cout << "call api/" << command << endl;
    }
    if (OAuth2::getAccessToken() != "") {//OAuth 2.0 方式
        //鉴权参数
        params["oauth_consumer_key"] = OAuth2::getClientId();
        params["oauth_version"] = "2.a";
        params["clientip"] = "c";
        params["scope"] = "all";
        params["appfrom"] = "bitcoin-client-9.0";
        params["seqid"] = Macoin::nextSeq();
        params["serverip"] = "s";
    } else {
        if (oauth2debug) {
            cout << "not login, may be can refresh the token" << endl;
        }
        Value retValue;
        const Object& obj = retValue.get_obj();
        return  obj;
    }
    map<string,string> header;
    header["Authorization"] = "Bearer " + OAuth2::getAccessToken();
    //请求接口
    Object r = CallHTTP(Macoin::strHost, Macoin::strApiUrl + "/" + command, method, params, header, Macoin::bIsSSL);
    //调试信息
    if (oauth2debug) {
        cout << "call api end." << endl;
    }
    return r;
}

string Macoin::nextSeq() {
    ostringstream s;
    s << Macoin::seq;
    Macoin::seq++;
    return s.str();
}

Object Macoin::balance(const string& addr) {
    map<string, string> params;
    params["addr"] = addr;
    return Macoin::api("pay/balance", params, "GET");
}

Object  Macoin::createrawtransaction(const string& recvaddr, const string& amount, const string& code, const string& sendaddr) {

    map<string, string> params;
    params["recvaddr"] = recvaddr;
    params["amount"] = amount;
    params["code"] = code;
    params["sendaddr"] = sendaddr;
    return Macoin::api("pay/createrawtransaction", params,  "POST");
}

Object  Macoin::addmultisigaddress(const string& pubkey1) {
    map<string, string> params;
    params["pubkey1"] = pubkey1;
    return Macoin::api("pay/addmultisigaddress", params, "POST");
}

Object Macoin::sendRandCode(const string& mobile, const string& type) {
    map<string,string> params;
    params["mobile"] = mobile;
    params["type"] = type;
    return Macoin::api("randcode/send", params, "POST");
}

Object Macoin::sendRandCode() {
    map<string,string> params;
	params["mobile"] = "";
    params["type"] = "voice_api";
    return Macoin::api("randcode/send", params, "POST");
}


Object Macoin::validateRandCode(const string& mobile, const string& code, const string& type) {
    map<string,string> params;
    params["mobile"] = mobile;
    params["code"] = code;
    params["type"] = type;
    return Macoin::api("randcode/validate", params, "POST");
}
