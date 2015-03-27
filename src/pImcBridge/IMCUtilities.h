#include <DUNE/DUNE.hpp>
using DUNE_NAMESPACES;

#define IVP_PLAN_ID 		"follow_IVP"
#define WPT_PLAN_ID 		"follow_WPT"
#define MOOS_LOITER_RADIUS 	20
#define MOOS_TIMEOUT 		10

PlanControl * createStartPlanMessage(std::string plan_id) {
  PlanControl * msg = new PlanControl();
  msg->type = PlanControl::PC_REQUEST;
  msg->op = PlanControl::PC_START;
  msg->plan_id = plan_id;
  return msg;
}

PlanControl * createStopPlanMessage() {
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

Reference * createWptReference(double latDegrees, double lonDegrees, double speed, double depth) {
  Reference * ref = new Reference();
  DesiredZ *z = new DesiredZ();
  DesiredSpeed *s = new DesiredSpeed();

  ref->flags = Reference::FLAG_LOCATION | Reference::FLAG_SPEED | Reference::FLAG_Z;
  s->speed_units = SUNITS_METERS_PS;
  s->value = speed;

  z->z_units = (depth >= 0)? Z_DEPTH : Z_ALTITUDE;
  z->value   = (depth >= 0)? depth : -depth;

  ref->z.set(z);
  ref->speed.set(s);

  ref->lat = Angles::radians(latDegrees);
  ref->lon = Angles::radians(lonDegrees);

  return ref;
}

Reference * createIVPReference(double curLatDegs, double curLonDegs, double headingDegs, double speed, double depth) {
  Reference * ref = new Reference();
  DesiredZ *z = new DesiredZ();
  DesiredSpeed *s = new DesiredSpeed();

  ref->flags = Reference::FLAG_LOCATION | Reference::FLAG_SPEED | Reference::FLAG_Z;
  s->speed_units = SUNITS_METERS_PS;
  s->value = speed;

  z->z_units = (depth >= 0)? Z_DEPTH : Z_ALTITUDE;
  z->value   = (depth >= 0)? depth : -depth;

  ref->z.set(z);
  ref->speed.set(s);

  double lat = Angles::radians(curLatDegs);
  double lon = Angles::radians(curLonDegs);
  double distance = speed * 60;
  double x = std::cos(Angles::radians(headingDegs)) * distance;
  double y = std::sin(Angles::radians(headingDegs)) * distance;
  WGS84::displace(x, y, &lat, &lon);

  ref->lat = lat;
  ref->lon = lon;
  return ref;
}

PlanControl * createIVPPlan(int local_id) {
  PlanControl * msg = new PlanControl();
  msg->type = PlanControl::PC_REQUEST;
  msg->op = PlanControl::PC_START;
  msg->plan_id = IVP_PLAN_ID;

  FollowReference * ref = new FollowReference();
  ref->control_src = local_id;
  ref->control_ent = 255;
  ref->timeout = MOOS_TIMEOUT;
  ref->loiter_radius = MOOS_LOITER_RADIUS;

  msg->arg.set(ref);

  return msg;
}

PlanControl * createWPTPlan(int local_id) {
  PlanControl * msg = new PlanControl();
  msg->type = PlanControl::PC_REQUEST;
  msg->op = PlanControl::PC_START;
  msg->plan_id = WPT_PLAN_ID;

  FollowReference * ref = new FollowReference();
  ref->control_src = local_id;
  ref->control_ent = 255;
  ref->timeout = MOOS_TIMEOUT;
  ref->loiter_radius = MOOS_LOITER_RADIUS;

  msg->arg.set(ref);

  return msg;
}
