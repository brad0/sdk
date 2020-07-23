/**
 * @file heartbeats.h
 * @brief
 * TODO: complete this
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#pragma once

#include "megaapi.h"
#include "types.h"
#include "mega.h"
#include <memory>

namespace mega
{

/**
 * @brief The HeartBeatSyncInfo class
 * This class holds the information that will be heartbeated
 */
class HeartBeatSyncInfo : public CommandListener
{
    class PendingTransferInfo
    {
    public:
        long long totalBytes = 0.;
        long long transferredBytes = 0.;
    };

public:

    enum Status
    {
        UPTODATE = 1,
        SYNCING = 2,
        PENDING = 3, //e.g. scanning, no actual transfers being made
        INACTIVE = 4, //sync is not active: a !Active status should have been sent through 'sp'
        UNKNOWN = 5,
    };

    HeartBeatSyncInfo(int tag, handle id);

    MEGA_DISABLE_COPY(HeartBeatSyncInfo)

    handle heartBeatId() const;

    Command *runningCommand() const;
    void setRunningCommand(Command *runningCommand);

    Status status() const;

    double progress() const;

    uint8_t pendingUps() const;

    uint8_t pendingDowns() const;

    m_time_t lastAction() const;

    mega::MegaHandle lastSyncedItem() const;

    void updateTransferInfo(MegaTransfer *transfer);
    void removePendingTransfer(MegaTransfer *transfer);
    void clearFinshedTransfers();

private:
    handle mHeartBeatId = UNDEF; //sync ID, from registration
    int mSyncTag = 0; //sync tag

    Status mStatus = UNKNOWN;

    long long mTotalBytes = 0;
    long long mTransferredBytes = 0;

    uint8_t mPendingUps = 0;
    uint8_t mPendingDowns = 0;

    std::map<int, std::unique_ptr<PendingTransferInfo>> mPendingTransfers;
    std::vector<std::unique_ptr<PendingTransferInfo>> mFinishedTransfers;

    mega::MegaHandle mLastSyncedItem; //last synced item

    m_time_t mLastAction = 0; //timestamps of the last action
    m_time_t mLastBeat = 0; //timestamps of the last beat

    Command *mRunningCommand = nullptr;

    void updateLastActionTime();

public:
    void onCommandToBeDeleted(Command *command) override;
    m_time_t lastBeat() const;
    void setLastBeat(const m_time_t &lastBeat);
    void setLastAction(const m_time_t &lastAction);
    void setStatus(const Status &status);
    void setPendingUps(uint8_t pendingUps);
    void setPendingDowns(uint8_t pendingDowns);
    void setLastSyncedItem(const mega::MegaHandle &lastSyncedItem);
    void setTotalBytes(long long value);
    void setTransferredBytes(long long value);
    int syncTag() const;
    void setHeartBeatId(const handle &heartBeatId);
};


class MegaHeartBeatMonitor : public MegaListener
{

    enum State
    {
        ACTIVE = 2, // working fine (enabled)
        FAILED = 3, // being deleted
        TEMPORARY_DISABLED = 4, // temporary disabled
        DISABLED = 5, //user disabled
        UNKNOWN = 6,
    };

    static constexpr int MAX_HEARBEAT_SECS_DELAY = 60*30; //max time to wait to update unchanged sync
public:
    explicit MegaHeartBeatMonitor(MegaClient * client);
    ~MegaHeartBeatMonitor() override;
    void beat(); //produce heartbeats!

private:
    std::map<int, std::shared_ptr<HeartBeatSyncInfo>> mHeartBeatedSyncs; //Map matching sync tag and HeartBeatSyncInfo
    mega::MegaClient *mClient = nullptr;
    std::map<int, int> mTransferToSyncMap; //Map matching transfer tag and sync tag

    std::deque<int> mPendingBackupPuts;

    void updateOrRegisterSync(MegaSync *sync);
    int getHBState (MegaSync *sync);
    int getHBSubstatus (MegaSync *sync);
    string getHBExtraData(MegaSync *sync);

    BackupType getHBType(MegaSync *sync);

    m_time_t mLastBeat = 0;
    std::shared_ptr<HeartBeatSyncInfo> getSyncHeartBeatInfoByTransfer(MegaTransfer *transfer);
    void calculateStatus(HeartBeatSyncInfo *hbs);
public:
    void reset();

    void setRegisteredId(handle id);

    void onSyncAdded(MegaApi *api, MegaSync *sync, int additionState) override;
    void onSyncDeleted(MegaApi *api, MegaSync *sync) override;
    void onSyncStateChanged(MegaApi *api, MegaSync *sync) override;

    void onTransferStart(MegaApi *api, MegaTransfer *transfer) override;
    void onTransferFinish(MegaApi *api, MegaTransfer *transfer, MegaError *error) override;
    void onTransferUpdate(MegaApi *api, MegaTransfer *transfer) override;

    void onGlobalSyncStateChanged(MegaApi *api) override {}
    void onSyncFileStateChanged(MegaApi *api, MegaSync *sync, std::string *localPath, int newState) override {}
    void onSyncEvent(MegaApi *api, MegaSync *sync, MegaSyncEvent *event) override {}
    void onSyncDisabled(MegaApi *api, MegaSync *sync) override {}
    void onSyncEnabled(MegaApi *api, MegaSync *sync) override {}

};
}

