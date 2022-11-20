#include "pch.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib/httplib.h"
#include <Windows.h>
#include <thread>

#include <LoggerAPI.h>
#include <LLAPI.h>
#include <string>
#include <Nlohmann/json.hpp>
#include "Version.h"


using json = nlohmann::json;

Logger AutoUpradelogger(PLUGIN_NAME);

void CheckUpdate(const std::string minebbs_resid) {
    httplib::Client clt("https://api.minebbs.com");
    //auto res = clt.Get("/api/openapi/v1/resources/4312")
    if (auto res = clt.Get("/api/openapi/v1/resources/" + minebbs_resid)) {
        if (res->status == 200) {
            if (!res->body.empty()) {
                auto res_json = nlohmann::json::parse(res->body);
                if (res_json.contains("status") && res_json["status"].get<int>() == 2000) {
                    ll::Version verRemote = ll::Version::parse(res_json["data"]["version"].get<std::string>());
                    ll::Version verLocal = ll::getPlugin(::GetCurrentModule())->version;
                    if (verRemote > verLocal) {
                        AutoUpradelogger.warn("发现新版本:{1}, 当前版本:{0} 更新地址:{2}", verLocal.toString(), res_json["data"]["version"], res_json["data"]["view_url"]);
                    }
                }
            }
        }
    }
}



void AutoUprade(const std::string minebbs_resid) {
    std::thread([minebbs_resid]() {
        CheckUpdate(minebbs_resid);
    }).detach();
}
