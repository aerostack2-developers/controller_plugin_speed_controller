#ifndef __DF_PLUGIN_H__
#define __DF_PLUGIN_H__

// Std libraries
#include <array>
#include <iostream>
#include <memory>
#include <rclcpp/logging.hpp>
#include <unordered_map>
#include <vector>

// #include "as2_control_command_handlers/acro_control.hpp"
#include "as2_msgs/msg/thrust.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"

#include "controller_plugin_base/controller_base.hpp"

#include "speed_controller.hpp"

namespace controller_plugin_speed_controller
{
  using Vector3d = Eigen::Vector3d;
  using SpeedController = speed_controller::SpeedController;
  using UAV_state = speed_controller::UAV_state;
  using Control_ref = speed_controller::Control_ref;

  struct Control_flags
  {
    bool parameters_read;
    bool state_received;
    bool ref_received;
  };

  class SCPlugin : public controller_plugin_base::ControllerBase
  {
  public:
    SCPlugin(){};
    ~SCPlugin(){};

  public:
    void ownInitialize() override;
    void updateState(const nav_msgs::msg::Odometry &odom) override;

    void updateReference(const geometry_msgs::msg::PoseStamped &ref) override;
    void updateReference(const geometry_msgs::msg::TwistStamped &ref) override;
    void updateReference(const trajectory_msgs::msg::JointTrajectoryPoint &ref) override;

    void computeOutput(geometry_msgs::msg::PoseStamped &pose,
                       geometry_msgs::msg::TwistStamped &twist,
                       as2_msgs::msg::Thrust &thrust) override;

    bool setMode(const as2_msgs::msg::ControlMode &mode_in,
                 const as2_msgs::msg::ControlMode &mode_out) override;

    rcl_interfaces::msg::SetParametersResult parametersCallback(const std::vector<rclcpp::Parameter> &parameters);
  };
};

#endif