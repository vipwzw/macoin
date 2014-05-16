#include "rpcclient.h"
#include "rpcprotocol.h"

use namespace std;
using namespace json_spirit;

class OAuth2 {
private:
    static string strClientId   = "demoapp";
    static string strClientPass = "demopass";
    static string strAccessTokenURL = "/oauth/authorize";
    static string strHost = "zc.macoin.org";
    static bool   bIsSSL = true;
    static string strAccessToken;
    static string strRefreshToken;
    static int    nExpireIn;

public:
    static void init(const string& clientId, const string& clientPass) {
        OAuth2::strClientId = clientId;
        OAuth2::strClientPass = clientPass;
    }

    public static Object login(const string& username, const string& password)
    {
        map<string, string> params;
        params["client_id"] = OAuth2::strClientId;
        params["client_secret"] = OAuth2::strClientPass;
        params["grant_type"] = "password";
        params["username"] = username;
        params["password"] = password;
        const Object& r = CallHTTP(OAuth2::strHost, OAuth2::strAccessTokenURL, "POST", params, OAuth2::bIsSSL);
        const Value& access_token  = find_value(r, "access_token");
        if (access_tokent.type() != null_type) {
            OAuth2::strAccessToken  = access_token.get_str();
            OAuth2::strRefreshToken = find_value(r, "refresh_token").get_str();
            OAuth2::nExpireIn        = find_value(r, "expires_in").get_int();
            return r;
        } else {
            return r;
        }
    }

    /**
     * 刷新授权信息
     * 此处以SESSION形式存储做演示，实际使用场景请做相应的修改
     */
    static Object refreshToken()
    {
        map<string, string> params;
        params["client_id"] = OAuth2::strClientId;
        params["client_secret"] = OAuth2::strClientPass;
        params["grant_type"] = "refresh_token";
        params["refresh_token"] = OAuth2::strRefreshToken;
        const Object& r = CallHTTP(OAuth2::strHost, OAuth2::strAccessTokenURL, "POST", params, OAuth2::bIsSSL);
        const Value& access_token  = find_value(r, "access_token");
        if (access_tokent.type() != null_type) {
            OAuth2::strAccessToken = access_token.get_str();
            OAuth2::nExpireIn      = find_value(r, "expires_in").get_int();
            return r;
        } else {
            return r;
        }
    }
    
    /**
     * 验证授权是否有效
     */
    static bool checkOAuthValid()
    {
        const Object& r = Macoin::api("user/hello");
        const Value& hello  = find_value(r, "hello");
        if (hello.type() != null_type && hello.get_str() == "word") {
            return true;
        } else {
            OAuth2::clear();
            return false;
        }
    }

    static bool clear() {
        OAuth2::strAccessToken = "";
        OAuth2::strRefreshToken = "";
        OAuth2::nExpireIn = 0;
        return true;
    }
}
