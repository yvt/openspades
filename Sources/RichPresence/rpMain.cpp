#define _CRT_SECURE_NO_WARNINGS
#include "rpMain.h"
#include "requestList.h"

namespace {
volatile bool interrupted{false};
}

discord::Core* core{};
auto result = discord::Core::Create(736944533090205756, DiscordCreateFlags_Default, &core);

int rpM(){
  SPADES_SETTING(cg_playerName);
  interrupted = true;

  discord::Activity activity{};

  SPLog("RP iniciando.");
  activity.SetDetails("Browsing on Server List...");
  activity.SetState(cg_playerName.operator std::string().substr(0, 15).c_str());
  activity.GetAssets().SetLargeImage("openspadesl");
  activity.GetAssets().SetSmallImage("openspadesf");
  activity.GetAssets().SetSmallText("Shooting on Blocks");
  activity.GetAssets().SetLargeText("");
  activity.SetType(discord::ActivityType::Playing);


  core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
    std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed")
      << " updating activity!\n";
  });

  interrupted = false;

  std::signal(SIGINT, [](int) { interrupted = true; });

  core->RunCallbacks();

  return 0;
}

int atualizarRP(){
  SPADES_SETTING(cg_lastQuickConnectHost);
  SPADES_SETTING(cg_playerName);

  interrupted = true;
  discord::Activity activity{};

  std::string player = cg_playerName.operator std::string().substr(0, 15).c_str();
  std::string playerStr = "Playing as: " + player;
  std::string checkIp = cg_lastQuickConnectHost.operator std::string().substr(0, 15).c_str();

  // If the user isnt playing on localhost the client will send the normal infos about the server
  if(checkIp.rfind("aos://16777343", 0) != 0 || checkIp.rfind("localhost", 0) != 0 || checkIp.rfind("127.0.0.1", 0) != 0){
    Json::Value infos = serverInfoM();

    std::string map = "Map: "+ infos["map"].asString();
    std::string online = "Players: "+std::to_string(infos["players_current"].asInt())
    + "/"+std::to_string(infos["players_max"].asInt());

    activity.SetDetails(infos["name"].asCString());
    activity.SetState(&online[0]);
    activity.GetAssets().SetLargeImage("openspadesl");
    activity.GetAssets().SetSmallImage("openspadesf");
    activity.GetAssets().SetSmallText(&map[0]);
    activity.GetAssets().SetLargeText(&playerStr[0]);
    activity.SetType(discord::ActivityType::Playing);
  } else {
    activity.SetDetails("Playing in Localhost!");
    activity.SetState(&playerStr[0]);
    activity.GetAssets().SetLargeImage("openspadesl");
    activity.GetAssets().SetSmallImage("openspadesf");
    activity.GetAssets().SetLargeText("Shooting on Blocks");
    activity.SetType(discord::ActivityType::Playing);
  }

  core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
    std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed")
      << " updating activity!\n";
  });

  interrupted = false;

  std::signal(SIGINT, [](int) { interrupted = true; });
  core->RunCallbacks();

  return 0;
}