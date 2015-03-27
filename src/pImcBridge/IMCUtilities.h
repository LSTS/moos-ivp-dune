#include <DUNE/DUNE.hpp>
using DUNE_NAMESPACES;

#define MOOS_PLAN_ID 		"follow_MOOS"
#define MOOS_LOITER_RADIUS 	20
#define MOOS_TIMEOUT 		10

PlanControl * createStartPlanMessage(std::string plan_id) {
  PlanControl * msg = new PlanControl();
  msg->type = PlanControl::PC_REQUEST;
  msg->op = PlanControl::PC_START;
  msg->plan_id = plan_id;
  return msg;
}

PlanControl * createStopPlanMessage(std::string plan_id) {
  PlanControl * msg = new PlanControl();
  msg->type = PlanControl::PC_REQUEST;
  msg->op = PlanControl::PC_STOP;
  return msg;
}

Abort * createAbortMessage() {
  return new Abort();
}

Heartbeat * createHeartBeatMessage() {
  return new Heartbeat();
}

PlanControl * createStartFollowRef(int local_id) {
  PlanControl * msg = new PlanControl();
  msg->type = PlanControl::PC_REQUEST;
  msg->op = PlanControl::PC_START;
  msg->plan_id = MOOS_PLAN_ID;

  FollowReference * ref = new FollowReference();
  ref->control_src = local_id;
  ref->timeout = MOOS_TIMEOUT;
  ref->loiter_radius = MOOS_LOITER_RADIUS;

  msg->arg.set(ref);

  return msg;
}
