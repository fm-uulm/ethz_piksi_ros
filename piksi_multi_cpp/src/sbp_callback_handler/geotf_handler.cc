#include "piksi_multi_cpp/sbp_callback_handler/geotf_handler.h"

#include <functional>

namespace piksi_multi_cpp {
namespace s = std::placeholders;

GeoTfHandler::GeoTfHandler(const ros::NodeHandle& nh,
                           const std::shared_ptr<sbp_state_t>& state)
    : base_pos_llh_handler_{std::bind(&GeoTfHandler::callbackToBasePosLlh, this,
                                      s::_1, s::_2),
                            SBP_MSG_BASE_POS_LLH, state},
      pos_llh_handler_{
          std::bind(&GeoTfHandler::callbackToPosLlh, this, s::_1, s::_2),
          SBP_MSG_POS_LLH, state},
      nh_(nh) {
  geotf_.initFromRosParam();
  if (geotf_.hasFrame("enu")) reset_position_ = ResetEnuOrigin::kNo;
  geotf_.addFrameByEPSG("ecef", 4978);
  geotf_.addFrameByEPSG("wgs84", 4326);

  set_enu_origin_srv_ = nh_.advertiseService(
      "set_enu_origin", &GeoTfHandler::setEnuOriginCallback, this);
  set_enu_from_base_srv_ =
      nh_.advertiseService("set_enu_origin_from_base_station",
                           &GeoTfHandler::setEnuOriginFromBaseStation, this);
  set_enu_from_current_srv_ =
      nh_.advertiseService("set_enu_origin_from_current_pos",
                           &GeoTfHandler::setEnuOriginFromCurrentPos, this);
}

void GeoTfHandler::setEnuOriginWgs84(const double lat, const double lon,
                                     const double alt) {
  geotf_.removeFrame("enu");
  geotf_.addFrameByENUOrigin("enu", lat, lon, alt);
  reset_position_ = ResetEnuOrigin::kNo;
}

void GeoTfHandler::callbackToBasePosLlh(const msg_base_pos_llh_t& msg,
                                        const uint8_t len) {
  if (reset_position_ != ResetEnuOrigin::kFromBase) return;
  setEnuOriginWgs84(msg.lat, msg.lon, msg.height);
}

void GeoTfHandler::callbackToPosLlh(const msg_pos_llh_t& msg,
                                    const uint8_t len) {
  if (reset_position_ != ResetEnuOrigin::kFromCurrentPos) return;
  setEnuOriginWgs84(msg.lat, msg.lon, msg.height);
}

bool GeoTfHandler::setEnuOriginCallback(
    piksi_rtk_msgs::EnuOrigin::Request& req,
    piksi_rtk_msgs::EnuOrigin::Response& res) {
  setEnuOriginWgs84(req.lat, req.lon, req.alt);
  return true;
}

bool GeoTfHandler::setEnuOriginFromBaseStation(std_srvs::Empty::Request& req,
                                               std_srvs::Empty::Response& res) {
  reset_position_ = ResetEnuOrigin::kFromBase;
  return true;
}

bool GeoTfHandler::setEnuOriginFromCurrentPos(std_srvs::Empty::Request& req,
                                              std_srvs::Empty::Response& res) {
  reset_position_ = ResetEnuOrigin::kFromCurrentPos;
  return true;
}

}  // namespace piksi_multi_cpp
