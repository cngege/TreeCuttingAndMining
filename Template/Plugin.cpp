﻿#include "pch.h"

#include <EventAPI.h>
#include <LoggerAPI.h>
#include <MC/Level.hpp>
#include <MC/BlockInstance.hpp>
#include <MC/Block.hpp>
#include <MC/BlockLegacy.hpp>
#include <MC/BlockSource.hpp>
#include <MC/Actor.hpp>
#include <MC/Player.hpp>
#include <MC/ItemStack.hpp>
#include "Version.h"
#include <LLAPI.h>
#include <ServerAPI.h>
//#include <MC/Json.hpp>
#include <direct.h>
#include "../minhook/MinHook.h"
using namespace nlohmann;

#pragma comment(lib, "../minhook/MinHook.x64.lib")

std::unordered_map<Player*, ItemStack*> playerMap;

static bool RetTrigger = false;

Logger logger(PLUGIN_NAME);

using DestroyBlockCALLType = void(__fastcall*)(Block* thi, Player* player, BlockPos* Bpos);
DestroyBlockCALLType OBlock_PlayerDestroy;

void Block_PlayerDestroy(Block*, Player*, BlockPos*);
void CheckMinerals(Block*, Player*, BlockPos*, std::string, std::string,bool);
bool CheckLeaves(json, BlockPos, int);
unsigned short Block_getTileData(Block*);
bool CheckUshortArray(json, unsigned short);
void TreeCutting(json, Block*, Player*, BlockPos*,bool);

string configpath = "./plugins/TreeCuttingAndMining/";
auto config = R"(
    {
        "Time_Accrual": true,
        "Sneak": true,
        "Digging": [
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
         ],
        "Tree": [
            {
                "Chopped_Wood_type":"minecraft:oak_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,4,8],
                "Check_Leaves_type":"minecraft:leaves",
                "Check_Leaves_Auxs":[4]
            },
            {
                "Chopped_Wood_type":"minecraft:birch_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,2,6,10],
                "Check_Leaves_type":"minecraft:leaves",
                "Check_Leaves_Auxs":[2,6]
            },
            {
                "Chopped_Wood_type":"minecraft:jungle_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,3,7,11],
                "Check_Leaves_type":"minecraft:leaves",
                "Check_Leaves_Auxs":[3,7]
            },
            {
                "Chopped_Wood_type":"minecraft:jungle_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,3,7,11],
                "Check_Leaves_type":"minecraft:leaves2",
                "Check_Leaves_Auxs":[4]
            },
            {
                "Chopped_Wood_type":"minecraft:spruce_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,1,5,9],
                "Check_Leaves_type":"minecraft:leaves",
                "Check_Leaves_Auxs":[1,5]
            },
            {
                "Chopped_Wood_type":"minecraft:dark_oak_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,1,5,9],
                "Check_Leaves_type":"minecraft:leaves2",
                "Check_Leaves_Auxs":[1,5]
            },
            {
                "Chopped_Wood_type":"minecraft:acacia_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,4,8],
                "Check_Leaves_type":"minecraft:leaves2",
                "Check_Leaves_Auxs":[0,4]
            },
            {
                "Chopped_Wood_type":"minecraft:oak_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,4,8],
                "Check_Leaves_type":"minecraft:azalea_leaves",
                "Check_Leaves_Auxs":[0]
            },
            {
                "Chopped_Wood_type":"minecraft:cherry_log",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,4,8],
                "Check_Leaves_type":"minecraft:cherry_leaves",
                "Check_Leaves_Auxs":[0]
            },
            {
                "Chopped_Wood_type":"minecraft:crimson_stem",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,4,8],
                "Check_Leaves_type":"minecraft:nether_wart_block",
                "Check_Leaves_Auxs":[0]
            },
            {
                "Chopped_Wood_type":"minecraft:warped_stem",
                "Chopped_Wood_Aux": 0,
                "Covered_Wood_Auxs":[0,4,8],
                "Check_Leaves_type":"minecraft:warped_wart_block",
                "Check_Leaves_Auxs":[0]
            }
         ]
    }
)"_json;

// 橡树原木 ok
// 白桦木  // 2 6
// 丛林木  // 3 7 树叶1
// 丛林木  // 4 树叶2
// 云杉原木 // 1 5
// 深色橡木 // 1 5
// 合金欢木 ok
// 牡丹花木 ok
// 樱花木 ok
// 绯红菌树 ok
// 诡异菌树 ok

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

void AutoUprade(const std::string minebbs_resid);

void PluginInit()
{
    CheckProtocolVersion();
    AutoUprade("4502");
#if PLUGIN_VERSION_STATUS == PLUGIN_VERSION_BETA
    logger.info("当前为 BETA 版本,更多功能正在开发中,请关注最新版发布，感谢支持");
#elif PLUGIN_VERSION_STATUS == PLUGIN_VERSION_DEV
    logger.warn("当前为 DEV 该版本的第{}个调试版本, 为开发者进行调试之版本，请更换为BETA或RELEASE版", PLUGIN_VERSION_BUILD);
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

void Block_PlayerDestroy(Block* thi, Player* player, BlockPos* Bpos){
    std::string name = thi->getTypeName();
    //unsigned short aux = Block_getTileData(thi);

#if PLUGIN_VERSION_STATUS == PLUGIN_VERSION_DEV
    unsigned short _aux = Block_getTileData(thi);
    //如果是调试模式
    //向控制台输出 玩家破坏的方块信息
    logger.info("方块:{}", thi->getTypeName());
    logger.info("方块id:{}", thi->getId());
    logger.info("方块特殊值:{}", _aux);
    logger.info("破坏速度:{}", thi->getDestroySpeed());
    logger.info("POS-X:{},Y:{},Z:{}", Bpos->x,Bpos->y,Bpos->z);
#endif
    OBlock_PlayerDestroy(thi, player, Bpos);
    if (config["Sneak"] == true && !player->isSneaking())
    {
        return;
    }

    //砍树
    for (auto& tree : config["Tree"]) {
        //判断砍的是不是配置文件中指定的木头
        if (tree["Chopped_Wood_type"] == name) {
            unsigned short aux = Block_getTileData(thi);
            // 检查 砍的是一颗配置文件中 规定的树
            if (tree["Chopped_Wood_Aux"] == aux) {
                if (CheckLeaves(tree, *Bpos, player->getDimensionId())) {
                    RetTrigger = true;
                    TreeCutting(tree,thi,player,Bpos,true);
                    break;
                }
            }
        }
    }

    // 检查矿物
    for (auto& name : config["Digging"]) {
        if (name == thi->getTypeName()) {
            if (name == "minecraft:lit_redstone_ore") {
                //RetTrigger = true;
                CheckMinerals(thi, player, Bpos, name,"minecraft:redstone_ore",true);
                break;
            }
            else if (name == "minecraft:lit_deepslate_redstone_ore") {
                //RetTrigger = true;
                CheckMinerals(thi, player, Bpos, name, "minecraft:deepslate_redstone_ore",true);
                break;
            }
            else {
                
                CheckMinerals(thi, player, Bpos, name, "",true);
                break;
            }

        }
    }
    if (RetTrigger){
        RetTrigger = false;
        playerMap[player] = player->getHandSlot();
    }
}


// 这个是 Block_PlayerDestroy 的调用者
THook(void*, "<lambda_8f42c031e5eb7245475425b96fd66358>::operator()", uintptr_t* a1, ItemStack& item) {
    Player* p = *reinterpret_cast<Player**>(*a1 + 8);
    void* ret = original(a1, item);
    
    // 这个 +160的偏移， 没有找到定义, 但是近几个版本改偏移没变,如果后续有必要确定该偏移
    // 则搜索 Block::playerDestroy 的调用处往下找 
    //if (v32)
    //{
    //    ItemStack::operator=(a2, (struct ItemStackBase*)v30);
    //}
    //else
    //{
    //    ItemStack::ItemStack(a2, (const struct ItemStack*)v30);
    //    *((_BYTE*)a2 + 160) = 1;
    //}
    // 就可以确定改偏移，如果后续改动频繁 则考虑特征码
    if (playerMap.find(p) != playerMap.end()) {
        if (*((BYTE*)&item + 160)) {
            item = *playerMap[p];
        }
        else {// 这里应该表示 item被析构掉了
            SymCall("??0ItemStack@@QEAA@AEBV0@@Z", ItemStack&, ItemStack&, ItemStack&)(item, *playerMap[p]);
            *((BYTE*)&item + 160) = 1;
        }
        playerMap.erase(p);
    }
    return ret;
}


bool CheckUshortArray(json arr, unsigned short val) {
    if (!arr.is_array()) return false;
    for (int i = 0; i < arr.size(); i++) {
        //logger.debug("i:{},arr[i]:{},val:{}", i, arr[i], val);
        if (arr[i] == val) {
            return true;
        }
    }
    return false;
}

// 破坏的方块名称特殊值， 位置 维度
bool CheckLeaves(json tree,BlockPos Bpos, int dimid) {
    if (Bpos.y > 320) return false;     //建筑高度 320个方块
    bool CheckLeavesValue = false;

    for (int x = -1; x <= 1; x++) {
        for (int z = -1; z <= 1; z++) {
            auto nBpos = BlockPos(Bpos.x + x, Bpos.y + 1, Bpos.z + z);
            auto block = Level::getBlock(nBpos, dimid);
            auto blockname = block->getTypeName();
            if (blockname == tree["Chopped_Wood_type"]) {
                auto data = Block_getTileData(block);
                if (CheckUshortArray(tree["Covered_Wood_Auxs"], data)) {        //如果向上检查的木头是匹配的
                    //if (CheckLeaves(tree, BlockPos(Bpos.x, Bpos.y + 1, Bpos.z), dimid)) {
                    if (CheckLeaves(tree, BlockPos(nBpos), dimid)) {
                        return true;
                    }
                }
            }
            if (blockname == tree["Check_Leaves_type"]) {
                auto data = Block_getTileData(block);
                if (CheckUshortArray(tree["Check_Leaves_Auxs"], data)) {        //如果向上检查的树叶是匹配的
                    return true;
                }
            }
        }
    }
    return false;
}
//#include <mc/Item.hpp>
//正式砍树
void TreeCutting(json tree, Block* block, Player* player, BlockPos* Bpos, bool isBaseBlock) {
    if (Bpos->y > 320) return;
    if (!isBaseBlock) {
        ItemStack* item = player->getHandSlot();
        if (!item || item->isNull()) return;
        item->mineBlock(*block, Bpos->x, Bpos->y, Bpos->z, player);
        player->setSelectedItem(*item);
        //player->updateInventoryTransactions();
        OBlock_PlayerDestroy(block, player, Bpos);
        RetTrigger = true;
    }

    Level::setBlock(*Bpos, player->getDimensionId(), "minecraft:air", 0);

    for (int y = 0; y <= 1;y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            for (int z = -1; z <= 1; z++)
            {
                BlockPos* nBpos = new BlockPos(Bpos->x + x, Bpos->y + y, Bpos->z + z);
                Block* nblock = Level::getBlock(nBpos, player->getDimensionId());

                if (nblock->getTypeName() == tree["Chopped_Wood_type"])
                {
                    if (CheckUshortArray(tree["Covered_Wood_Auxs"], Block_getTileData(nblock))) {
                        TreeCutting(tree, nblock, player, nBpos, false);
                    }
                }
                delete nBpos;
            }
        }
    }

}

void CheckMinerals(Block* block ,Player* player, BlockPos* Bpos,std::string Bname,std::string compatible, bool isBaseBlock) {

    if (!isBaseBlock) {
        ItemStack* item = player->getHandSlot();
        if (!item || item->isNull()) return;
        item->mineBlock(*block, Bpos->x, Bpos->y, Bpos->z, player);
        player->setSelectedItem(*item);
        //player->updateInventoryTransactions();
        OBlock_PlayerDestroy(block, player, Bpos);
        RetTrigger = true;
    }

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
                    CheckMinerals(nblock, player, nBpos, Bname, compatible,false);
                }
                delete nBpos;
            }
        }
    }
}
/*
#include "MC/Item.hpp"

THook(bool, "?dispense@Item@@UEBA_NAEAVBlockSource@@AEAVContainer@@HAEBVVec3@@E@Z", Item* thi, BlockSource* a2, Container* a3, int a4, Vec3* a5, unsigned char a6)
{
    logger.info("发射物品:{}", thi->getFullItemName());
    logger.info("位置:{}", a5->toBlockPos().toString());
    //auto ret = original(thi, a2, a3, a4, a5, a6);
    //logger.info("返回:{}", ret);
    //return ret;
    return true;
}
*/

unsigned short Block_getTileData(Block* block) {
    //return block->getTileData();
    // 等待大佬改进
    auto tileData = dAccess<unsigned short, 8>(block);
    auto blk = &block->getLegacyBlock();

    if (((BlockLegacy*)blk)->toBlock(tileData) == (Block*)block)
        return tileData;

    for (unsigned short i = 0; i < 16; ++i) {
        if (i == tileData)
            continue;
        if (((BlockLegacy*)blk)->toBlock(i) == (Block*)block)
            return i;
    }
    //logger.error("Error in GetTileData");
    return 0;
}