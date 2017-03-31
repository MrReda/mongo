/**
 *    Copyright (C) 2015 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/s/commands/scatter_gather_from_shards.h"

#include "mongo/s/client/shard_registry.h"
#include "mongo/s/grid.h"

namespace mongo {
namespace {

class ClusterRepairDatabaseCmd : public Command {
public:
    ClusterRepairDatabaseCmd() : Command("repairDatabase") {}

    bool slaveOk() const override {
        return true;
    }
    bool adminOnly() const override {
        return false;
    }

    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {
        ActionSet actions;
        actions.addAction(ActionType::repairDatabase);
        out->push_back(Privilege(ResourcePattern::forDatabaseName(dbname), actions));
    }

    virtual bool supportsWriteConcern(const BSONObj& cmd) const override {
        return false;
    }

    bool run(OperationContext* opCtx,
             const std::string& dbName,
             BSONObj& cmdObj,
             int options,
             std::string& errmsg,
             BSONObjBuilder& output) override {
        // Target all shards.
        std::vector<AsyncRequestsSender::Request> requests;
        std::vector<ShardId> shardIds;
        Grid::get(opCtx)->shardRegistry()->getAllShardIds(&shardIds);
        for (auto&& shardId : shardIds) {
            requests.emplace_back(std::move(shardId), cmdObj);
        }

        auto swResults = gatherResults(opCtx, dbName, cmdObj, options, requests, &output);
        return appendCommandStatus(output, swResults.getStatus());
    }

} clusterRepairDatabaseCmd;

}  // namespace
}  // namespace mongo