/**
 * @file sqlite.h
 * @brief SQLite DB access layer
 *
 * (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#ifdef USE_SQLITE
#ifndef DBACCESS_CLASS
#define DBACCESS_CLASS SqliteDbAccess

#include <sqlite3.h>

// Include ICU headers
#include <unicode/uchar.h>

#include "mega/db.h"

namespace mega {

class MEGA_API SqliteDbTable : public DbTable
{
protected:
    sqlite3* db = nullptr;
    LocalPath dbfile;
    FileSystemAccess *fsaccess;

    sqlite3_stmt* pStmt = nullptr;
    sqlite3_stmt* mDelStmt = nullptr;
    sqlite3_stmt* mPutStmt = nullptr;

    // handler for DB errors ('interrupt' is true if caller can be interrupted by CancelToken)
    void errorHandler(int sqliteError, const std::string& operation, bool interrupt);

public:
    void rewind() override;
    bool next(uint32_t*, string*) override;
    bool get(uint32_t, string*) override;
    bool put(uint32_t, char*, unsigned) override;
    bool del(uint32_t) override;
    void truncate() override;
    void begin() override;
    void commit() override;
    void abort() override { doAbort(); }
    void remove() override;

    SqliteDbTable(PrnGen &rng, sqlite3*, FileSystemAccess &fsAccess, const LocalPath &path, const bool checkAlwaysTransacted, DBErrorCallback dBErrorCallBack);
    virtual ~SqliteDbTable();

    bool inTransaction() const override { return doInTransaction(); }

private:
    void doAbort();
    bool doInTransaction() const;
};

/**
 * This class implements DbTable iface (by deriving SqliteDbTable), and additionally
 * implements DbTableNodes iface too, so it allows to manage `nodes` table.
 */
class MEGA_API SqliteAccountState : public SqliteDbTable, public DBTableNodes
{
public:
    // Access to table `nodes`
    bool getNode(mega::NodeHandle nodehandle, NodeSerialized& nodeSerialized) override;
    bool getNodesByOrigFingerprint(const std::string& fingerprint, std::vector<std::pair<NodeHandle, NodeSerialized>> &nodes) override;
    bool getRootNodes(std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes) override;
    bool getNodesWithSharesOrLink(std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes, ShareType_t shareType) override;
    bool getChildren(NodeHandle parentHandle, std::vector<std::pair<NodeHandle, NodeSerialized>>& children, CancelToken cancelFlag) override;
    bool getChildrenFromType(NodeHandle parentHandle, nodetype_t nodeType, std::vector<std::pair<NodeHandle, NodeSerialized>>& children, mega::CancelToken cancelFlag) override;
    uint64_t getNumberOfChildren(NodeHandle parentHandle) override;
    // If a cancelFlag is passed, it must be kept alive until this method returns.
    bool searchForNodesByName(const std::string& name, std::vector<std::pair<NodeHandle, NodeSerialized>> &nodes, CancelToken cancelFlag) override;
    bool searchForNodesByNameNoRecursive(const std::string& name, std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes, NodeHandle parentHandle, CancelToken cancelFlag) override;
    bool searchInShareOrOutShareByName(const std::string& name, std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes, ShareType_t shareType, CancelToken cancelFlag) override;
    bool getNodesByFingerprint(const std::string& fingerprint, std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes) override;
    bool getNodeByFingerprint(const std::string& fingerprint, mega::NodeSerialized& node) override;
    bool getRecentNodes(unsigned maxcount, m_time_t since, std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes) override;
    bool getFavouritesHandles(NodeHandle node, uint32_t count, std::vector<mega::NodeHandle>& nodes) override;
    bool childNodeByNameType(NodeHandle parentHanlde, const std::string& name, nodetype_t nodeType, std::pair<NodeHandle, NodeSerialized>& node) override;
    bool getNodeSizeTypeAndFlags(NodeHandle node, m_off_t& size, nodetype_t& nodeType, uint64_t &oldFlags) override;
    bool isAncestor(mega::NodeHandle node, mega::NodeHandle ancestor, CancelToken cancelFlag) override;
    uint64_t getNumberOfNodes() override;
    uint64_t getNumberOfChildrenByType(NodeHandle parentHandle, nodetype_t nodeType) override;
    bool getNodesByMimetype(MimeType_t mimeType, std::vector<std::pair<mega::NodeHandle, mega::NodeSerialized> >& nodes, Node::Flags requiredFlags, Node::Flags excludeFlags, CancelToken cancelFlag) override;
    bool getNodesByMimetypeExclusiveRecursive(MimeType_t mimeType, std::vector<std::pair<NodeHandle, NodeSerialized>>& nodes, Node::Flags requiredFlags, Node::Flags excludeFlags, Node::Flags excludeRecursiveFlags, NodeHandle anscestorHandle, CancelToken cancelFlag) override;
    bool put(Node* node) override;
    bool put(uint32_t index, char* data, unsigned len) override { return SqliteDbTable::put(index, data, len); }
    bool remove(mega::NodeHandle nodehandle) override;
    bool removeNodes() override;

    void updateCounter(NodeHandle nodeHandle, const std::string& nodeCounterBlob) override;
    void updateCounterAndFlags(NodeHandle nodeHandle, uint64_t flags, const std::string& nodeCounterBlob) override;
    void createIndexes() override;

    void remove() override;
    SqliteAccountState(PrnGen &rng, sqlite3*, FileSystemAccess &fsAccess, const mega::LocalPath &path, const bool checkAlwaysTransacted, DBErrorCallback dBErrorCallBack);
    void finalise();
    virtual ~SqliteAccountState();

    // Callback registered by some long-time running queries, so they can be canceled
    // If the progress callback returns non-zero, the operation is interrupted
    static int progressHandler(void *);
    static void userRegexp(sqlite3_context* context, int argc, sqlite3_value** argv);
    static int icuLikeCompare(const uint8_t *zPattern,   /* LIKE pattern */
            const uint8_t *zString,    /* The UTF-8 string to compare against */
            const UChar32 uEsc         /* The escape character */
          );

private:
    // Iterate over a SQL query row by row and fill the map
    // Allow at least the following containers:
    bool processSqlQueryNodes(sqlite3_stmt *stmt, std::vector<std::pair<mega::NodeHandle, mega::NodeSerialized>>& nodes);

    // if add a new sqlite3_stmt update finalise()
    sqlite3_stmt* mStmtPutNode = nullptr;
    sqlite3_stmt* mStmtUpdateNode = nullptr;
    sqlite3_stmt* mStmtUpdateNodeAndFlags = nullptr;
    sqlite3_stmt* mStmtTypeAndSizeNode = nullptr;
    sqlite3_stmt* mStmtGetNode = nullptr;
    sqlite3_stmt* mStmtChildren = nullptr;
    sqlite3_stmt* mStmtChildrenFromType = nullptr;
    sqlite3_stmt* mStmtNumChildren = nullptr;
    sqlite3_stmt* mStmtNodeByName = nullptr;
    sqlite3_stmt* mStmtNodeByNameNoRecursive = nullptr;
    sqlite3_stmt* mStmtInShareOutShareByName = nullptr;
    sqlite3_stmt* mStmtNodeByMimeType = nullptr;
    sqlite3_stmt* mStmtNodeByMimeTypeExcludeRecursiveFlags = nullptr;
    sqlite3_stmt* mStmtNodesByFp = nullptr;
    sqlite3_stmt* mStmtNodeByFp = nullptr;
    sqlite3_stmt* mStmtNodeByOrigFp = nullptr;
    sqlite3_stmt* mStmtChildNode = nullptr;
    sqlite3_stmt* mStmtIsAncestor = nullptr;
    sqlite3_stmt* mStmtNumChild = nullptr;
    sqlite3_stmt* mStmtRecents = nullptr;
    sqlite3_stmt* mStmtFavourites = nullptr;

    // how many SQLite instructions will be executed between callbacks to the progress handler
    // (tests with a value of 1000 results on a callback every 1.2ms on a desktop PC)
    static const int NUM_VIRTUAL_MACHINE_INSTRUCTIONS = 1000;
};

class MEGA_API SqliteDbAccess : public DbAccess
{
    LocalPath mRootPath;

public:
    explicit SqliteDbAccess(const LocalPath& rootPath);

    ~SqliteDbAccess();

    LocalPath databasePath(const FileSystemAccess& fsAccess,
                           const string& name,
                           const int version) const;

    // Note: for proper adjustment of legacy versions, 'sctable' should be the first DB to be opened
    // In this way, when it's called with other DB (statusTable, tctable, ...), DbAccess::currentDbVersion has been
    // updated to new value
    bool checkDbFileAndAdjustLegacy(FileSystemAccess& fsAccess, const string& name, const int flags, LocalPath& dbPath) override;

    SqliteDbTable* open(PrnGen &rng, FileSystemAccess& fsAccess, const string& name, const int flags, DBErrorCallback dBErrorCallBack) override;

    DbTable* openTableWithNodes(PrnGen &rng, FileSystemAccess& fsAccess, const string& name, const int flags, DBErrorCallback dBErrorCallBack) override;

    bool probe(FileSystemAccess& fsAccess, const string& name) const override;

    const LocalPath& rootPath() const override;

private:
    bool openDBAndCreateStatecache(sqlite3 **db, FileSystemAccess& fsAccess, const string& name, mega::LocalPath &dbPath, const int flags);
    bool renameDBFiles(mega::FileSystemAccess& fsAccess, mega::LocalPath& legacyPath, mega::LocalPath& dbPath);
    void removeDBFiles(mega::FileSystemAccess& fsAccess, mega::LocalPath& dbPath);
};

} // namespace

#endif
#endif

