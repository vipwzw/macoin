// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOINRPC_CLIENT_H_
#define _BITCOINRPC_CLIENT_H_ 1

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

int CommandLineRPC(int argc, char *argv[]);

json_spirit::Object CallHTTP(const std::string& host, const std::string& url, const std::string& method, const std::map<std::string, std::string>& params, const std::map<std::string, std::string>& header, bool fUseSSL);

json_spirit::Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams);

/** Show help message for bitcoin-cli.
 * The mainProgram argument is used to determine whether to show this message as main program
 * (and include some common options) or as sub-header of another help message.
 *
 * @note the argument can be removed once bitcoin-cli functionality is removed from bitcoind
 */
std::string HelpMessageCli(bool mainProgram);

class OAuth2 {
private:
    /*
    static std::string strClientId;
    static std::string strClientPass;
    static std::string strAccessTokenURL;
    static std::string strHost;
    static bool   bIsSSL;
    static std::string strAccessToken;
    static std::string strRefreshToken;
    static int    nExpireIn;
    */

public:
    static void init(const std::string& clientId, const std::string& clientPass);
    
    static std::string getAccessToken();

    static void enableDebug();

    static void disableDebug();

    static std::string getClientId();

    static json_spirit::Object login(const std::string& username, const std::string& password);
    /**
     * 刷新授权信息
     * 此处以SESSION形式存储做演示，实际使用场景请做相应的修改
     */
    static json_spirit::Object refreshToken();
    /**
     * 验证授权是否有效
     */
    static bool checkOAuthValid();
    static void clear();
};


//class Macoin, visit api of macoin
class Macoin
{
private:
    //接口url
    static std::string strApiUrl;
    //调试模式
    static bool debug;
    static std::string strHost;
    static bool   bIsSSL;
    static int seq;
    /**
     * 发起一个API请求
     * @param $command 接口名称 如：t/add
     * @param $params 接口参数  array('content'=>'test');
     * @param $method 请求方式 POST|GET
     * @param $multi 图片信息
     * @return string
     */
public:
    static json_spirit::Object balance(const std::string& addr);
    static json_spirit::Object api(const std::string& command, std::map<std::string,std::string> params, const std::string& method = "GET");
    static json_spirit::Object  createrawtransaction(const std::string& recvaddr, const std::string& amount, const std::string& code = "0", const std::string& sendaddr = "all");
    static json_spirit::Object  addmultisigaddress(const std::string& pubkey1);
    static json_spirit::Object sendRandCode(const std::string& mobile, const std::string& type = "voice_api");
    static json_spirit::Object validateRandCode(const std::string& mobile, const std::string& code, const std::string& type = "voice_api");
private:
    static std::string nextSeq();
};

#endif
