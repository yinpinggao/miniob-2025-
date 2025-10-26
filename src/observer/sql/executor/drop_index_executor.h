/***************************************************************
 *                                                             *
 * @Author      : Codex                                        *
 * @Date        : 2025/10/26                                   *
 * @Description : DropIndexExecutor header file                *
 *                                                             *
 ***************************************************************/

#pragma once

#include "common/rc.h"

class SQLStageEvent;

/**
 * @brief 删除索引的执行器
 * @ingroup Executor
 */
class DropIndexExecutor
{
public:
  DropIndexExecutor()          = default;
  virtual ~DropIndexExecutor() = default;

  RC execute(SQLStageEvent *sql_event);
};
