/**
* Copyright (c) 2021 OceanBase
* OceanBase CE is licensed under Mulan PubL v2.
* You can use this software according to the terms and conditions of the Mulan PubL v2.
* You may obtain a copy of Mulan PubL v2 at:
*          http://license.coscl.org.cn/MulanPubL-2.0
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PubL v2 for more details.
*/

#ifndef OCEANBASE_SQL_ENGINE_EXPR_OB_EXPR_CHECK_LOCATION_ACCESS_
#define OCEANBASE_SQL_ENGINE_EXPR_OB_EXPR_CHECK_LOCATION_ACCESS_

#include "sql/engine/expr/ob_expr_operator.h"

namespace oceanbase
{
namespace sql
{
class ObExprCheckLocationAccess : public ObFuncExprOperator
{
public:
  explicit  ObExprCheckLocationAccess(common::ObIAllocator &alloc);
  virtual ~ObExprCheckLocationAccess();
  virtual int calc_result_type1(ObExprResType &type,
                                ObExprResType &type1,
                                common::ObExprTypeCtx &type_ctx) const;
  static int eval_check_location_access(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum);
  virtual int cg_expr(ObExprCGCtx &op_cg_ctx,
                      const ObRawExpr &raw_expr,
                      ObExpr &rt_expr) const override;
private:
  DISALLOW_COPY_AND_ASSIGN(ObExprCheckLocationAccess);
};
}
}
#endif /* OCEANBASE_SQL_ENGINE_EXPR_OB_EXPR_CHECK_LOCATION_ACCESS_ */
