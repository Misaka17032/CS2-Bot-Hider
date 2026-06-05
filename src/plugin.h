// plugin.h
//
// Metamod:Source plugin entry

#pragma once

#include <ISmmPlugin.h>
#include <playerslot.h>
#include <tier1/utlvector.h>

class CServerSideClient;
class INetworkGameClient;
class CCSPlayerController;
class ConCommandRef;
class CCommandContext;
class CCommand;
enum ENetworkDisconnectionReason : int;

namespace cs2bh
{

    class HiderPlugin : public ISmmPlugin, public IMetamodListener
    {
    public:
        // ISmmPlugin
        bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
        bool Unload(char *error, size_t maxlen) override;

        const char *GetAuthor() override { return "XBribo"; }
        const char *GetName() override { return "CS2-Bot-Hider"; }
        const char *GetDescription() override { return "Bot persona/steamid/ping hider"; }
        const char *GetURL() override { return ""; }
        const char *GetLicense() override { return "GPLv3"; }
        const char *GetVersion() override { return "0.1.4"; }
        const char *GetDate() override { return __DATE__; }
        const char *GetLogTag() override { return "BOTHIDER"; }

        // IMetamodListener
        void OnLevelInit(char const *pMapName, char const *, char const *, char const *, bool, bool) override;
        void OnLevelShutdown() override;

        // Hook entry points
        void Hook_OnClientConnected_Post(CPlayerSlot slot, const char *pszName, uint64 xuid,
                                         const char *pszNetworkID, const char *pszAddress,
                                         bool bFakePlayer);
        void Hook_ClientPutInServer_Post(CPlayerSlot slot, char const *pszName, int type, uint64 xuid);
        void Hook_ClientDisconnect_Pre(CPlayerSlot slot, ENetworkDisconnectionReason reason,
                                       const char *pszName, uint64 xuid, const char *pszNetworkID);
        CPlayerSlot Hook_CreateFakeClient_Pre(const char *netname);
        CUtlVector<INetworkGameClient *> *Hook_StartChangeLevel_Pre(
            const char *mapName, const char *landmark, void *changelevelState);
        void Hook_GameFrame_Post(bool simulating, bool bFirstTick, bool bLastTick);

        // ICvar::DispatchConCommand  — restore bot identity before the engine and processes a kick
        void Hook_DispatchConCommand_Pre(ConCommandRef cmd, const CCommandContext &ctx,
                                         const CCommand &args);
        void Hook_DispatchConCommand_Post(ConCommandRef cmd, const CCommandContext &ctx,
                                          const CCommand &args);

        // CUtlString::Set resolved from tier0.dll at Load
        using CUtlStringSetFn = void (*)(void * /*CUtlString this*/, const char *);
        CUtlStringSetFn m_pUtlStringSet = nullptr;

        // Toggle disguise globally; re-applies or restores m_bFakePlayer on all managed slots
        void SetDisguiseEnabled(bool enabled);

        // Clean-rebuild bots on same-map rematch: restore identities, bot_kick, re-fill to bot_quota
        void RebuildBots();

        // Two-phase rematch cleanup (avoids the cooldown bot-churn race):
        // match end → restore flags, bot_kick all, hold bot_quota at 0
        void KickAllBots();
        // match begin → restore bot_quota to the value saved at the previous match end
        void RefillBots();

    private:
        void *m_pHookedGameServer = nullptr;
        int m_StartChangeLevelHookId = 0;
        bool m_bSelfDisabled = false;
        unsigned int m_TickCounter = 0; // throttles per-tick idle-timer reset
        // ! Master disguise switch: false on bot-manager-driven maps (e.g. aim_*) so bots still spawn
        bool m_bDisguiseEnabled = true;
        // ! Set while rebuilding bots on a disguise toggle so our own kick handlers skip
        bool m_bRebuilding = false;
        // ! bot_quota captured at match-end KickAllBots, restored at match-begin RefillBots
        int m_SavedQuota = 0;
        // ! Set across the whole match-end kickid storm so our own kick hooks fully skip;
        //   cleared at match-begin RefillBots and on level change
        bool m_bSuppressKickHooks = false;
    };

    extern HiderPlugin g_Plugin;

} // namespace cs2bh

PLUGIN_GLOBALVARS();
