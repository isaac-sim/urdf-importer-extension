#!/bin/bash

source /opt/ros/melodic/setup.bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# add to ROS package path
export ROS_PACKAGE_PATH=${SCRIPT_DIR}/../..:$ROS_PACKAGE_PATH

echo "Using ROS_PACKAGE_PATH ${ROS_PACKAGE_PATH}"

rosrun xacro xacro --inorder -o panda_arm.urdf panda_arm.urdf.xacro
rosrun xacro xacro --inorder -o panda_arm_hand.urdf panda_arm_hand.urdf.xacro


