/*
 * Copyright 2021 Stefano Grossi, University of Modena and Reggio Emilia, Italy
 * Copyright 2021 Lorenzo Sabattini, University of Modena and Reggio Emilia, Italy
 * Copyright 2021 Federico Pratissoli, University of Modena and Reggio Emilia, Italy
 * Copyright 2021 Beatrice Capelli, University of Modena and Reggio Emilia, Italy
 * Copyright 2021 Giuseppe Silano, Czech Technical University in Prague, Czech Republic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thread>
#include <chrono>
#include <math.h>

#include "rotors_gazebo/Quaternion.h"
#include "rotors_gazebo/transform_datatypes.h"
#include "rotors_gazebo/parameters_ros.h"
#include "rotors_gazebo/Matrix3x3.h"

#include <Eigen/Core>
#include <mav_msgs/conversions.h>
#include <mav_msgs/default_topics.h>
#include <mav_msgs/DroneState.h>
#include <ros/ros.h>
#include <std_srvs/Empty.h>
#include <nav_msgs/Odometry.h>
#include <ros/console.h>
#include <time.h>
#include <trajectory_msgs/MultiDOFJointTrajectory.h>

#define SAMPLING_TIME  10e-3       /* SAMPLING CONTROLLER TIME [s] - 100Hz */
#define START_SIMULATION_TIME 3   /* TIME GAZEBO NEEDS TO INITIALIZE THE ENVIRONMENT */

int main(int argc, char** argv) {

  ros::init(argc, argv, "hovering_lee");
  ros::NodeHandle nh;
  // Create a private node handle for accessing node parameters.
  ros::NodeHandle nh_private("~");

  ros::Publisher trajectory_pub =
      nh.advertise<mav_msgs::DroneState>(
          mav_msgs::default_topics::DRONE_STATE, 10);
  ROS_INFO("Start hovering example lee.");

  double xd, yd, zd;
  double x_b1, y_b1, z_b1;

  std_srvs::Empty srv;
  bool unpaused = ros::service::call("/gazebo/unpause_physics", srv);
  unsigned int i = 0;

  // Trying to unpause Gazebo for 10 seconds.
  while (i <= 10 && !unpaused) {
    ROS_INFO("Wait for 1 second before trying to unpause Gazebo again.");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    unpaused = ros::service::call("/gazebo/unpause_physics", srv);
    ++i;
  }

  if (!unpaused) {
    ROS_FATAL("Could not wake up Gazebo.");
    return -1;
  } else {
    ROS_INFO("Unpaused the Gazebo simulation.");
  }

  // Wait for 3 seconds to let the Gazebo GUI show up.
  if (ros::Time::now().toSec() < START_SIMULATION_TIME){
    ros::Duration(START_SIMULATION_TIME).sleep();
  }

  // Trajectory message
  mav_msgs::DroneState trajectory_msg;
  mav_msgs::EigenDroneState eigen_reference;
  double j = 1;

  while(j <=200){
      xd = 0;
      yd = 0;
      zd = j/200 + 0.015;
      j++;

      x_b1 = 1;
      y_b1 = 0;
      z_b1 = 0;

      trajectory_msg.header.stamp = ros::Time::now();
      eigen_reference.position_W = Eigen::Vector3f(xd, yd, zd);
      eigen_reference.orientation_W_B = Eigen::Quaterniond(0, x_b1, y_b1, z_b1);
      mav_msgs::eigenDroneFromStateToMsg(&eigen_reference, trajectory_msg);

      // Debug eigen_reference
      ROS_DEBUG("Publishing position from msg: [%f, %f, %f].", eigen_reference.position_W[0], eigen_reference.position_W[1],
        eigen_reference.position_W[2]);
      ROS_DEBUG("Publishing orientation: [%f, %f, %f].", eigen_reference.orientation_W_B.x(), eigen_reference.orientation_W_B.y(),
        eigen_reference.orientation_W_B.z());

      // Debug trajectory_message
      ROS_DEBUG("Publishing waypoint from msg: [%f, %f, %f].", trajectory_msg.position.x, trajectory_msg.position.y,
        trajectory_msg.position.z);

      trajectory_pub.publish(trajectory_msg);

      ros::Duration(SAMPLING_TIME).sleep();
  }

  ros::spin();

  return 0;
}
