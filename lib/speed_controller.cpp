/*!*******************************************************************************************
 *  \file       speed_controller.cpp
 *  \brief      This file contains the implementation of the Speed PID controller with speed output.
 *  \authors    Miguel Fernández Cortizas
 *              Pedro Arias Pérez
 *              David Pérez Saura
 *              Rafael Pérez Seguí
 *
 *  \copyright  Copyright (c) 2022 Universidad Politécnica de Madrid
 *              All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************************/

#include "speed_controller.hpp"

namespace speed_controller
{
  SpeedController::SpeedController()
  {
    updateGains_();
  };

  bool SpeedController::setParameter(const std::string &param, const double &value)
  {
    if (parameters_.count(param) == 1)
    {
      parameters_[param] = value;
      updateGains_();
      return true;
    }
    return false;
  };

  bool SpeedController::getParameter(const std::string &param, double &value)
  {
    if (parameters_.count(param) == 1)
    {
      value = parameters_[param];
      return true;
    }
    return false;
  };

  bool SpeedController::isParameter(const std::string &param)
  {
    return parameters_.count(param) == 1;
  };

  bool SpeedController::setParametersList(const std::vector<std::pair<std::string, double>> &parameter_list)
  {
    for (auto &param : parameter_list)
    {
      if (parameters_.count(param.first) == 1)
      {
        parameters_[param.first] = param.second;
      }
      else
      {
        return false;
      }
    }
    updateGains_();
    return true;
  };

  std::vector<std::pair<std::string, double>> SpeedController::getParametersList()
  {
    std::vector<std::pair<std::string, double>> list;
    for (auto &param : parameters_)
    {
      list.push_back({param.first, param.second});
    }
    return list;
  };

  void SpeedController::updateGains_()
  {
    antiwindup_cte_ = parameters_["antiwindup_cte"];
    alpha_ = parameters_["alpha"];

    position_Kp_lin_mat_ = Vector3d(
                               parameters_["position_following.position_Kp.x"],
                               parameters_["position_following.position_Kp.y"],
                               parameters_["position_following.position_Kp.z"])
                               .asDiagonal();

    position_Ki_lin_mat_ = Vector3d(
                               parameters_["position_following.position_Ki.x"],
                               parameters_["position_following.position_Ki.y"],
                               parameters_["position_following.position_Ki.z"])
                               .asDiagonal();

    position_Kd_lin_mat_ = Vector3d(
                               parameters_["position_following.position_Kd.x"],
                               parameters_["position_following.position_Kd.y"],
                               parameters_["position_following.position_Kd.z"])
                               .asDiagonal();

    traj_Kp_lin_mat_ = Vector3d(
                           parameters_["trajectory_following.position_Kp.x"],
                           parameters_["trajectory_following.position_Kp.y"],
                           parameters_["trajectory_following.position_Kp.z"])
                           .asDiagonal();

    traj_Ki_lin_mat_ = Vector3d(
                           parameters_["trajectory_following.position_Ki.x"],
                           parameters_["trajectory_following.position_Ki.y"],
                           parameters_["trajectory_following.position_Ki.z"])
                           .asDiagonal();

    traj_Kd_lin_mat_ = Vector3d(
                           parameters_["trajectory_following.position_Kd.x"],
                           parameters_["trajectory_following.position_Kd.y"],
                           parameters_["trajectory_following.position_Kd.z"])
                           .asDiagonal();

    yaw_ang_mat_ = Vector3d(
                      parameters_["yaw_speed_controller.Kp"],
                      parameters_["yaw_speed_controller.Ki"],
                      parameters_["yaw_speed_controller.Kd"]);

    // speed_Kp_lin_mat_ = Vector3d(
    //                            parameters_["speed_following.speed_Kp.x"],
    //                            parameters_["speed_following.speed_Kp.y"],
    //                            parameters_["speed_following.speed_Kp.z"])
    //                            .asDiagonal();

    // speed_Ki_lin_mat_ = Vector3d(
    //                            parameters_["speed_following.speed_Ki.x"],
    //                            parameters_["speed_following.speed_Ki.y"],
    //                            parameters_["speed_following.speed_Ki.z"])
    //                            .asDiagonal();

    // speed_Kd_lin_mat_ = Vector3d(
    //                            parameters_["speed_following.speed_Kd.x"],
    //                            parameters_["speed_following.speed_Kd.y"],
    //                            parameters_["speed_following.speed_Kd.z"])
    //                            .asDiagonal();

    return;
  };

  // Return velocity control command
  Vector3d SpeedController::computePositionControl(
      const UAV_state &state,
      const Control_ref &ref,
      const double &dt)
  {
    // Compute the proportional contribution (position error)
    Vector3d position_error = ref.pos - state.pos;
    Vector3d p_position_error_contribution = position_Kp_lin_mat_ * position_error;

    // Store the error for the next iteration
    static Vector3d last_position_error_ = position_error;
    static Vector3d filtered_d_position_error_ = position_error;

    // Compute the derivative contribution of the error filtered with a first order filter (position derivate)
    Vector3d position_error_incr = (position_error - last_position_error_);
    filtered_d_position_error_ = alpha_ * position_error_incr + (1.0 - alpha_) * filtered_d_position_error_;

    // Compute the derivate contribution (velocity error)
    Vector3d d_position_error_contribution = position_Kd_lin_mat_ * filtered_d_position_error_ / dt;

    // Update de acumulated error
    position_accum_error_ += position_error * dt;

    // Compute anti-windup. Limit integral contribution
    for (short j = 0; j < 3; j++)
    {
      float antiwindup_value = antiwindup_cte_ / position_Ki_lin_mat_.diagonal()[j];
      position_accum_error_[j] = (position_accum_error_[j] > antiwindup_value) ? antiwindup_value : position_accum_error_[j];
      position_accum_error_[j] = (position_accum_error_[j] < -antiwindup_value) ? -antiwindup_value : position_accum_error_[j];
    }

    // Compute de integral contribution (position integrate)
    Vector3d i_position_error_contribution = position_Ki_lin_mat_ * position_accum_error_;

    // Compute desired speed
    Vector3d desired_speed = p_position_error_contribution + d_position_error_contribution + i_position_error_contribution;
    return desired_speed;

    // Control_ref desired_speed_ref = ref;
    // desired_speed_ref.vel = desired_speed;
    // return computeSpeedControl(
    //     state,
    //     desired_speed_ref,
    //     dt);
  };


  Vector3d SpeedController::computeTrayectoryControl(
      const UAV_state &state,
      const Control_ref &ref,
      const double &dt)
  {
    // return ref.vel + traj_Kp_ * (ref.pos - state.pos);

    // Compute the proportional contribution (position)
    Vector3d position_error = ref.pos - state.pos;
    Vector3d p_error_contribution = traj_Kp_lin_mat_ * position_error;

    // Compute de derivative error contribution (velocity)
    Vector3d velocity_error = ref.vel - state.vel;
    Vector3d d_error_contribution = traj_Kd_lin_mat_ * velocity_error;

    // Update de acumulated error
    traj_position_accum_error_ += position_error;

    // Compute anti-windup. Limit integral contribution
    for (short j = 0; j < 3; j++)
    {
      float antiwindup_value = antiwindup_cte_ / traj_Ki_lin_mat_.diagonal()[j];
      traj_position_accum_error_[j] = (traj_position_accum_error_[j] > antiwindup_value) ? antiwindup_value : traj_position_accum_error_[j];
      traj_position_accum_error_[j] = (traj_position_accum_error_[j] < -antiwindup_value) ? -antiwindup_value : traj_position_accum_error_[j];
    }

    // Compute de integral contribution (position integrate)
    Vector3d i_error_contribution = traj_Ki_lin_mat_ * traj_position_accum_error_;

    // Compute the error speed contribution
    return (p_error_contribution + d_error_contribution + i_error_contribution);
  };

  // Return velocity control command
  Vector3d SpeedController::computeSpeedControl(
      const UAV_state &state,
      const Control_ref &ref,
      const double &dt)
  {
    // Compute the proportional contribution (speed error)
    Vector3d speed_error = ref.vel - state.vel;
    Vector3d p_speed_error_contribution = speed_Kp_lin_mat_ * speed_error;

    // Store the error for the next iteration
    static Vector3d last_speed_error_ = speed_error;
    static Vector3d filtered_d_speed_error_ = speed_error;

    // Compute the derivative contribution of the error filtered with a first order filter (speed derivate)
    Vector3d speed_error_incr = (speed_error - last_speed_error_);
    filtered_d_speed_error_ = alpha_ * speed_error_incr + (1.0 - alpha_) * filtered_d_speed_error_;

    // Compute the derivate contribution (velocity error)
    Vector3d d_speed_error_contribution = speed_Kd_lin_mat_ * filtered_d_speed_error_ / dt;

    // Update de acumulated error
    speed_accum_error_ += speed_error * dt;

    // Compute anti-windup. Limit integral contribution
    for (short j = 0; j < 3; j++)
    {
      float antiwindup_value = antiwindup_cte_ / speed_Ki_lin_mat_.diagonal()[j];
      speed_accum_error_[j] = (speed_accum_error_[j] > antiwindup_value) ? antiwindup_value : speed_accum_error_[j];
      speed_accum_error_[j] = (speed_accum_error_[j] < -antiwindup_value) ? -antiwindup_value : speed_accum_error_[j];
    }

    // Compute de integral contribution (speed integrate)
    Vector3d i_speed_error_contribution = speed_Ki_lin_mat_ * speed_accum_error_;

    // Compute desired acceleration
    return (p_speed_error_contribution + d_speed_error_contribution + i_speed_error_contribution);
  };

  double angleMinError(
    const double &yaw_angle_state,
    const double &yaw_angle_ref)
  {
    // Wrap angles to [-pi, pi]
    
    double yaw_angle_ref_wrap = yaw_angle_ref;
    if (yaw_angle_ref_wrap < -M_PI)
    {
        yaw_angle_ref_wrap += 2.0 * M_PI;
    }
    else if (yaw_angle_ref_wrap > M_PI)
    {
        yaw_angle_ref_wrap -= 2.0 * M_PI;
    }

    double yaw_angle_state_wrap = yaw_angle_state;
    if (yaw_angle_state_wrap < -M_PI)
    {
        yaw_angle_state_wrap += 2.0 * M_PI;
    }
    else if (yaw_angle_state_wrap > M_PI)
    {
        yaw_angle_state_wrap -= 2.0 * M_PI;
    }
    
    // Compute yaw angle error in rad
    double yaw_angle_diff = yaw_angle_ref_wrap - yaw_angle_state;

    // Wrap angle error to [-pi, pi]
    if (yaw_angle_diff < -M_PI)
    {
        return yaw_angle_diff + 2.0 * M_PI;
    }
    else if (yaw_angle_diff > M_PI)
    {
        return yaw_angle_diff - 2.0 * M_PI;
    }
    return yaw_angle_diff;
  }

  // Return yaw speed in rad/s to reach a reference yaw angle
  double SpeedController::computeYawSpeed(
        const double &yaw_angle_state,
        const double &yaw_angle_ref,
        const double &dt)
  {

    // Compute the proportional error (yaw error)
    double yaw_p_error = angleMinError(yaw_angle_state, yaw_angle_ref);

    // Store the error for the next iteration
    static double last_yaw_p_error_ = yaw_p_error;
    static double filtered_d_yaw_error_ = yaw_p_error;

    // Compute the error of the derivative filtered with a first order filter (yaw derivate)
    double yaw_error_incr = (yaw_p_error - last_yaw_p_error_);
    filtered_d_yaw_error_ = alpha_ * yaw_error_incr + (1.0 - alpha_) * filtered_d_yaw_error_;

    // Update de acumulated error
    yaw_accum_error_ += yaw_p_error * dt;

    // Compute anti-windup. Limit integral contribution
    float antiwindup_value = antiwindup_cte_ / yaw_ang_mat_[1];
    yaw_accum_error_ = (yaw_accum_error_ >  antiwindup_value) ?  antiwindup_value : yaw_accum_error_;
    yaw_accum_error_ = (yaw_accum_error_ < -antiwindup_value) ? -antiwindup_value : yaw_accum_error_;

    // Compute desired acceleration
    return yaw_ang_mat_[0] * yaw_p_error + yaw_ang_mat_[1] * yaw_accum_error_ + yaw_ang_mat_[2] * filtered_d_yaw_error_;
  };

  void SpeedController::resetError()
  {
    position_accum_error_.setZero();
    traj_position_accum_error_.setZero();
    speed_accum_error_.setZero();
    yaw_accum_error_ = 0.0;
    return;
  };

}