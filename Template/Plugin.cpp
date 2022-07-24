#include "pch.h"
#include <EventAPI.h>
#include <LoggerAPI.h>
#include <MC/Level.hpp>
#include <MC/BlockInstance.hpp>
#include <MC/Block.hpp>
#include <MC/BlockSource.hpp>
#include <MC/Actor.hpp>
#include <MC/Player.hpp>
#include <MC/ItemStack.hpp>
#include "Version.h"
#include <LLAPI.h>
#include <ServerAPI.h>
#include <MC/Json.hpp>
#include <direct.h>
#include "../minhook/MinHook.h"

#pragma comment(lib, "../minhook/MinHook.x64.lib")

Logger logger(PLUGIN_NAME);

using DestroyBlockCALLType = void(__fastcall*)(Block* thi, Player* player, BlockPos* Bpos);
DestroyBlockCALLType OBlock_PlayerDestroy;

void Block_PlayerDestroy(Block* thi, Player* player, BlockPos* Bpos);
void CheckMinerals(Block* block, Player* player, BlockPos* Bpos,std::string name, std::string compatible);

string configpath = "./plugins/TreeCuttingAndMining/";
auto config = R"(
    {
        "Time_Accrual": true,
        "Sneak": true,
        "digging": [
            "minecraft:iron_ore",
            "minecraft:gold_ore",
            "minecraft:diamond_ore",
            "minecraft:lapis_ore",
            "minecraft:lit_redstone_ore",
            "minecraft:coal_ore",
            "minecraft:copper_ore",
            "minecraft:emerald_ore",
            "minecraft:quartz_ore",
            "minecraft:nether_gold_ore",
            "minecraft:deepslate_iron_ore",
            "minecraft:deepslate_gold_ore",
            "minecraft:deepslate_diamond_ore",
            "minecraft:deepslate_lapis_ore",
            "minecraft:lit_deepslate_redstone_ore",
            "minecraft:deepslate_emerald_ore",
            "minecraft:deepslate_coal_ore",
            "minecraft:deepslate_copper_ore"
         ]
    }
)"_json;



inline void CheckProtocolVersion() {
#ifdef TARGET_BDS_PROTOCOL_VERSION
    auto currentProtocol = LL::getServerProtocolVersion();
    if (TARGET_BDS_PROTOCOL_VERSION != currentProtocol)
    {
        logger.warn("Protocol version not match, target version: {}, current version: {}.",
            TARGET_BDS_PROTOCOL_VERSION, currentProtocol);
        logger.warn("This will most likely crash the server, please use the Plugin that matches the BDS version!");
    }
#endif // TARGET_BDS_PROTOCOL_VERSION
}

void PluginInit()
{
    CheckProtocolVersion();

#if PLUGIN_VERSION_STATUS == PLUGIN_VERSION_BETA
    logger.info("当前为 BETA 版本,更多功能正在开发中,请关注最新版发布，感谢支持");
#elif PLUGIN_VERSION_STATUS == PLUGIN_VERSION_DEV
    logger.info("当前为 DEV 版本, 为开发者进行调试之版本，请更换为BETA或RELEASE版");
#endif

    //读写配置文件
    if (_access(configpath.c_str(), 0) == -1)	//表示配置文件所在的文件夹不存在
    {
        if (_mkdir(configpath.c_str()) == -1)
        {
            //文件夹创建失败
            logger.warn("Directory creation failed, please manually create the plugins/TreeCuttingAndMining directory");
            return;
        }
    }
    std::ifstream f((configpath + "TreeCuttingAndMining.json").c_str());
    if (f.good())								//表示配置文件存在
    {
        f >> config;
        f.close();
    }
    else {
        //配置文件不存在
        std::ofstream c((configpath + "TreeCuttingAndMining.json").c_str());
        c << config.dump(2);
        c.close();
    }

    MH_Initialize();
    MH_CreateHookEx((LPVOID)dlsym("?playerDestroy@Block@@QEBAXAEAVPlayer@@AEBVBlockPos@@@Z"), &Block_PlayerDestroy, &OBlock_PlayerDestroy);
    auto status = MH_EnableHook((LPVOID)dlsym("?playerDestroy@Block@@QEBAXAEAVPlayer@@AEBVBlockPos@@@Z"));

    if (status != MH_OK) {
        logger.error("MinHook Error,status:{}", status);
    }
}

/* 普通橡树树叶 原木
22:10:10 INFO [TreeCuttingAndMining] 方块:minecraft:leaves
22:10:10 INFO [TreeCuttingAndMining] 方块id:18
22:10:10 INFO [TreeCuttingAndMining] 方块特殊值:0,4 【2:白桦树叶】

22:11:16 INFO [TreeCuttingAndMining] 方块:minecraft:log
22:11:16 INFO [TreeCuttingAndMining] 方块id:17
22:11:16 INFO [TreeCuttingAndMining] 方块特殊值:【0，4，8】【2，6，10：白桦树原木】
*/

bool CheckLeaves(BlockPos Bpos);

void Block_PlayerDestroy(Block* thi, Player* player, BlockPos* Bpos){
    std::string name = thi->getTypeName();
    unsigned short aux = thi->getTileData();

#if PLUGIN_VERSION_STATUS == PLUGIN_VERSION_DEV
    //如果是调试模式
    //向控制台输出 玩家破坏的方块信息
    logger.info("方块:{}", thi->getTypeName());
    logger.info("方块id:{}", thi->getId());
    logger.info("方块特殊值:{}", thi->getTileData());
    //getDestroySpeed()
    logger.info("破坏速度:{}", thi->getDestroySpeed());
    logger.info("POS-X:{},Y:{},Z:{}", Bpos->x,Bpos->y,Bpos->z);
#endif

    if (config["Sneak"] == true && !player->isSneaking())
    {
        
        return OBlock_PlayerDestroy(thi, player, Bpos);
    }

    if (name == "minecraft:leaves" && (aux == 0 || aux == 4)) {
        //检测 树叶 判断是否是生长的树
        
    }

    // 检查矿物
    for (auto& name : config["digging"]) {
        if (name == thi->getTypeName()) {
            if (name == "minecraft:lit_redstone_ore") {
                return CheckMinerals(thi, player, Bpos, name,"minecraft:redstone_ore");
            }
            else if (name == "minecraft:lit_deepslate_redstone_ore") {
                return CheckMinerals(thi, player, Bpos, name, "minecraft:deepslate_redstone_ore");
            }
            else {
                return CheckMinerals(thi, player, Bpos, name, "");
            }

        }
        
    }
    return OBlock_PlayerDestroy(thi, player, Bpos);
}

bool CheckLeaves(BlockPos Bpos, int dimid, std::string Cutname,std::string Leaves, int Leavesdata) {
    if (Bpos.y > 256) return false;

    for (int x = -1; x <= 1; x++) {
        for (int z = -1; z <= 1; z++) {
            auto nBpos = BlockPos(Bpos.x + x, Bpos.y + 1, Bpos.z + z);
            auto block = Level::getBlock(nBpos, dimid);
            if (block->getTypeName() == Leaves && block->getTileData() == Leavesdata) {
                return true;
            }
            else if (block->getTypeName() == Cutname) {
                return CheckLeaves(BlockPos(Bpos.x + x, Bpos.y + 1, Bpos.z + z));
            }
        }
    }

    return false;
}

void TreeCutting() {

}

void CheckMinerals(Block* block ,Player* player, BlockPos* Bpos,std::string Bname,std::string compatible = "") {
    OBlock_PlayerDestroy(block, player, Bpos);
    Level::setBlock(*Bpos, player->getDimensionId(), "minecraft:air", 0);
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            for (int z = -1; z <= 1; z++)
            {
                BlockPos* nBpos = new BlockPos(Bpos->x + x, Bpos->y + y, Bpos->z + z);
                Block* nblock = Level::getBlock(nBpos, player->getDimensionId());
                
                if (nblock->getTypeName() == Bname || (compatible != "" && nblock->getTypeName() == compatible) && !player->getSelectedItem().isNull())
                {
                    CheckMinerals(nblock, player, nBpos, Bname, compatible);
                    delete nBpos;
                }
            }
        }
    }
}