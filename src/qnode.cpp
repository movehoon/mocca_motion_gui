/**
 * @file /src/qnode.cpp
 *
 * @brief Ros communication central!
 *
 * @date February 2011
 **/

/*****************************************************************************
** Includes
*****************************************************************************/

#include <ros/ros.h>
#include <ros/network.h>
#include <string>
#include <std_msgs/String.h>
#include <std_msgs/Int8.h>
#include <sstream>
#include "../include/mocca_motion_gui/qnode.hpp"
#include <mocca_motion_gui/MoccaMotionAction.h>
#include <actionlib/client/simple_action_client.h>
#include <boost/thread.hpp>

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace mocca_motion_gui {

typedef actionlib::SimpleActionClient<mocca_motion_gui::MoccaMotionAction> Client;

/*****************************************************************************
** Implementation
*****************************************************************************/

QNode::QNode(int argc, char** argv ) :
	init_argc(argc),
	init_argv(argv)
	{}

QNode::~QNode() {
    if(ros::isStarted()) {
      ros::shutdown(); // explicitly needed since we use ros::start();
      ros::waitForShutdown();
    }
	wait();
}

bool QNode::init() {
	ros::init(init_argc,init_argv,"mocca_motion_gui");
	if ( ! ros::master::check() ) {
		return false;
	}
	ros::start(); // explicitly needed since our nodehandle is going out of scope.
	ros::NodeHandle n;
	// Add your ros communications here.
	// chatter_publisher = n.advertise<std_msgs::String>("chatter", 1000);
	publisher_moccaTorque = n.advertise<std_msgs::Int8>("mocca_robot_torque", 10);
	start();
	return true;
}

bool QNode::init(const std::string &master_url, const std::string &host_url) {
	std::map<std::string,std::string> remappings;
	remappings["__master"] = master_url;
	remappings["__hostname"] = host_url;
	ros::init(remappings,"mocca_motion_gui");
	if ( ! ros::master::check() ) {
		return false;
	}
	ros::start(); // explicitly needed since our nodehandle is going out of scope.
	ros::NodeHandle n;
	// Add your ros communications here.
	// chatter_publisher = n.advertise<std_msgs::String>("chatter", 1000);
	publisher_moccaTorque = n.advertise<std_msgs::Int8>("mocca_robot_torque", 10);
	start();
	return true;
}

void QNode::torque(int enable) {
	std_msgs::Int8 msg;
	msg.data = enable;
	publisher_moccaTorque.publish(msg);
	ros::spinOnce();
}

void QNode::playMotion(std::string jsonString) {
	printf("playMotion: %s", jsonString.c_str());
	std::cout << "action server connected" << std::endl;
	motionData = jsonString;
	state_playMotion = 1;
	// Fill in goal here
}

void QNode::run() {
	ros::Rate loop_rate(1);
	int count = 0;
	Client client("/mocca_motion", true); // true -> don't need ros::spin()
	client.waitForServer();
	while ( ros::ok() ) {
		switch (state_playMotion) {
			case 0:
				break;
			case 1:
			{
				mocca_motion_gui::MoccaMotionGoal goal;
				goal.motion_data = motionData;
				client.sendGoal(goal);
				state_playMotion = 2;
				client.waitForResult(ros::Duration(5.0));
				break;
			}
			case 2:
			{
				if (client.getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
					state_playMotion = 0;
					printf("Motion done");
				}
				printf("Current State: %s\n", client.getState().toString().c_str());
				break;
			}
		}
		// std_msgs::String msg;
		// std::stringstream ss;
		// ss << "hello world " << count;
		// msg.data = ss.str();
		// chatter_publisher.publish(msg);
		// log(Info,std::string("I sent: ")+msg.data);
		ros::spinOnce();
		// loop_rate.sleep();
		// ++count;
	}
	std::cout << "Ros shutdown, proceeding to close the gui." << std::endl;
	Q_EMIT rosShutdown(); // used to signal the gui for a shutdown (useful to roslaunch)
}


void QNode::log( const LogLevel &level, const std::string &msg) {
	logging_model.insertRows(logging_model.rowCount(),1);
	std::stringstream logging_model_msg;
	switch ( level ) {
		case(Debug) : {
				ROS_DEBUG_STREAM(msg);
				logging_model_msg << "[DEBUG] [" << ros::Time::now() << "]: " << msg;
				break;
		}
		case(Info) : {
				ROS_INFO_STREAM(msg);
				logging_model_msg << "[INFO] [" << ros::Time::now() << "]: " << msg;
				break;
		}
		case(Warn) : {
				ROS_WARN_STREAM(msg);
				logging_model_msg << "[INFO] [" << ros::Time::now() << "]: " << msg;
				break;
		}
		case(Error) : {
				ROS_ERROR_STREAM(msg);
				logging_model_msg << "[ERROR] [" << ros::Time::now() << "]: " << msg;
				break;
		}
		case(Fatal) : {
				ROS_FATAL_STREAM(msg);
				logging_model_msg << "[FATAL] [" << ros::Time::now() << "]: " << msg;
				break;
		}
	}
	QVariant new_row(QString(logging_model_msg.str().c_str()));
	logging_model.setData(logging_model.index(logging_model.rowCount()-1),new_row);
	Q_EMIT loggingUpdated(); // used to readjust the scrollbar
}

}  // namespace mocca_motion_gui
