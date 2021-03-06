#include "1257Robot.h"

using namespace std;

template<typename T>
std::string ToString(T t) //convert something else to a string
{
	std::stringstream ss; // Declare string stream
	ss << t; // Insert numerical value into stream
	string res = ""; // Declare std::string to store the number string
	ss >> res; // Extract number as std::string
	return res; // Return the result string
}

Team1257Robot::Team1257Robot(): // Initialization of objects based on connection ports
	Left(0), Right(1), Center(2), Lift(3), // PWM
	Stick1(0), Stick2(1), //Driver Station
	dSolenoid(4, 5), canburglar(0, 1),// PCM
	bottomlimit(0), toplimit(1), // Digital IO
	angle(0) // Analog Input
{

}

void Team1257Robot::TeleopInit() //This runs once at the beginning of teleop.
{
	//CameraServer::GetInstance()->StartAutomaticCapture("cam1"); // Automatically send camera images
}

//This runs in a loop during teleop mode.
void Team1257Robot::TeleopPeriodic() //NOTE: THE RIGHT MOTOR IS FLIPPED, SO TO HAVE THEM GO IN THE SAME DIRECTION, ONE OF THEM MUST BE NEGATED
{
	double sf = .8;//scale factor for scaling drive values
	double strafesf = 1; //scale factor for the strafe wheel since it is weaker

	//1ST CONTROLLER
	if (Stick1.GetRawButton(5) && !Stick1.GetRawButton(6)) // Back button; just one, not both
	{
		/*Safety switches - these need to be clicked to do anything
		 * just one bumper, not both
		 * which bumper is clicked determines which analog is move forward/back/left/right
		 * Here - if the left bumper is clicked, the left analog is is move forward/back/left/right
		 * the right analog is pivot
		 */

		Center.Set(-(float)accel(Stick1, 0, strafe, strafesf, 0.2)); //smoothly set the speed of the strafe to the left/right values of the left analog
		if(!Stick1.GetRawAxis(4) || dAbs(Stick1.GetRawAxis(4)) < .2)
			/*
			 * account for imperfections in the analog and prevent extremely small pushes of the analog from affecting the robot
			 * make sure that the pivot analog isn't being pushed - if so, give priority to the turning analog
			 */
		{
			Left.Set(-(float)accel(Stick1, 1, forwardback, sf)); //smoothly set the value of the left motor
				//to the negative of left analog forward/back (Look at the note above for why)
			Right.Set((float)accel(Stick1, 1, forwardback, sf)); //smoothly set the value of the right motor to
				//the left analog forward/back
		}
		else //turn
		{
			Left.Set((float)accel(Stick1, 4, turn, sf)); //smoothly set the value of the left motor to the
				//left analog forward/back
			Right.Set((float)accel(Stick1, 4, turn, sf)); //smoothly set the value of the right motor to the
				//left analog forward/back
		}
	}
	else if (!Stick1.GetRawButton(5) && Stick1.GetRawButton(6))
	{
		/*Safety switches - these need to be clicked to do anything
		 * just one bumper, not both
		 * which bumper is clicked determines which analog is move forward/back/left/right
		 * Here - if the right bumper is clicked, the right analog is is move forward/back/left/right
		 * the left analog is pivot
		 */
		Center.Set(-(float)accel(Stick1, 4, strafe, strafesf, 0.2)); //smoothly set the speed of the strafe to the value of the right analog
		if(!Stick1.GetRawAxis(0) || dAbs(Stick1.GetRawAxis(0)) < .2)
			/*
			 * account for imperfections in the analog and prevent extremely small pushes of the analog from affecting the robot
			 * make sure that the pivot analog isn't being pushed - if so, give priority to the turning analog
			 */
		{
			Left.Set(-(float)accel(Stick1, 5, forwardback, sf)); //smoothly set the value of the left motor
			//to the negative of right analog forward/back (Look at the note above for why)
			Right.Set((float)accel(Stick1, 5, forwardback, sf)); //smoothly set the value of the right motor to
			//the right analog forward/back
		}
		else //turn
		{
			Left.Set((float)accel(Stick1, 0, turn, sf)); //smoothly set the value of the left motor to the
			//right analog forward/back
			Right.Set((float)accel(Stick1, 0, turn, sf)); //smoothly set the value of the right motor to the
			//right analog forward/back
		}

	}
	else //safety switches not clicked -> DON'T MOVE
	{
		Left.Set(0);
		Right.Set(0);
		Center.Set(0);
	}

	//2ND CONTROLLER

	if(Stick2.GetRawButton(1))
		lsignore = true;
	else if(Stick2.GetRawButton(2))
		lsignore = false;
	if (Stick2.GetRawButton(5)) //Left bumper clicked
	{
		dSolenoid.Set(DoubleSolenoid::kForward); //open the arms
	}
	else if(Stick2.GetRawButton(6)) //Right bumper clicked
	{
		dSolenoid.Set(DoubleSolenoid::kReverse); //close the arms
	}
	else //if neither
		dSolenoid.Set(DoubleSolenoid::kOff); //DON'T DO ANYTHING!
	if(Stick2.GetRawButton(4))
	{
		if(Stick2.GetRawAxis(1) < -.5)
			canburglar.Set(DoubleSolenoid::kForward);
		else if(Stick2.GetRawAxis(1) > .5)
			canburglar.Set(DoubleSolenoid::kReverse);
		else
			canburglar.Set(DoubleSolenoid::kOff);
	}
	double liftval = Stick2.GetRawAxis(3) - Stick2.GetRawAxis(2); //get the value of the left and right triggers together

	if(((!bottomlimit.Get() || liftval > 0) && (!toplimit.Get() || liftval < 0)) || lsignore)
		/*
		 * neither the top nor bottom limit switches are clicked
		 * if it is at the bottom limit but wants to move up, allow it to move
		 * if it is at the top limit but wants to move down, allow it to move
		 */
		Lift.Set(liftval); //move the lift
	else
		Lift.Set(0); //DON'T MOVE
}

bool ran = false; // Variable to ensure that Autonomous only runs once

void Team1257Robot::AutonomousInit() //This runs once at the beginning of autonomous
{
	ran = false; // Makes sure that "ran" is set to false initially
	angle.Reset();

	auto_robot = SmartDashboard::GetBoolean("AUTO_ROBOT", true);
	auto_tote = SmartDashboard::GetBoolean("AUTO_TOTE", false);
	auto_container = SmartDashboard::GetBoolean("AUTO_CONTAINER", true);
	auto_start = SmartDashboard::GetBoolean("AUTO_START", true);
}
#define GEARBOX_CHANGE .75
void Team1257Robot::AutonomousPeriodic() //This runs in a loop during autonomous mode
{
	if(!ran) // Only run autonomous while the "ran" variable is false
	{
		//if(!bottomlimit.Get() && !toplimit.Get()) // Makes sure that neither switch is tripped
		//{
			if(auto_start)
			{
				if(auto_container || auto_tote)
				{
					dSolenoid.Set(DoubleSolenoid::kReverse); // Close arms
					Wait(.2); // Wait for .2 seconds
					Lift.Set(.9 * GEARBOX_CHANGE); // Lift arms with containers grasped
					Wait(1.75); // Continue for 1.75 seconds
					Wait(1);
					Lift.Set(0);// Stop lifting; high enough
				}
				if(auto_tote && auto_container)
				{
					Right.Set(-.5); // Move forward
					Left.Set(.5); // Move forward
					Wait(.85); // Do so for .85 seconds
					Right.Set(0); // STOP!
					Left.Set(0); // STOP!
					Lift.Set(-.55 * GEARBOX_CHANGE); // Set container down, then keep lowering to below tote
					Wait(0.5); // Do so for .5 seconds
					dSolenoid.Set(DoubleSolenoid::kForward); // Open the arms
					Right.Set(.3); // Move backwards
					Left.Set(-.3); // To avoid having the arms get caught on tote
					Wait(.4); //keep going back for .4 seconds
					Right.Set(0); // Stop moving forward
					Left.Set(0); // Stop moving forward
					Center.Set(-.5); // Slide into place to pick up tote
					Wait(.5); // Do so for .5 seconds
					Center.Set(0); // STOP
					Left.Set(0); // THE
					Right.Set(0); // DRIVE
					Lift.Set(-.55 * GEARBOX_CHANGE); // Just a reminder that the elevator should STILL be going down
					Wait(3); // JUST 3 MORE SECONDS
					dSolenoid.Set(DoubleSolenoid::kReverse); // Close the arms to grasp the tote
					Lift.Set(.35 * GEARBOX_CHANGE); // Move the elevator up to lift the tote and container just a bit
					Wait(1); // A second should be long enough

					PIDangle(0, 0, 0, -90, .5);
				}
				if(auto_robot)
				{
					Right.Set(-.65); // Move forward
					Left.Set(.65); // Into AUTO Zone
					Wait(2.2); // Hold high speed for 1.6 seconds
					Right.Set(-0.5); // Reduce speed
					Left.Set(0.5); // to .5 output
					Wait(.3); // hold for just .3 seconds
					Right.Set(-0.3); // Now reduce again
					Left.Set(0.3); // to .3 output
					Wait(.4); // Hold for .4 seconds
					Right.Set(-0.1); // Slow again
					Left.Set(0.1); // To .1 speed (may not actually move robot, just to help slow to a stop)
					Wait(.5); // Hold for .5 seconds
					Right.Set(0); // Stop the robot
					Left.Set(0); // In the AUTO ZONE
					Lift.Set(0); // AUTO is finished
				}
			}

			else if(!auto_start)
			{
				if(auto_container)
				{
					//Canburglar?
				}
				if(auto_robot)
				{	Right.Set(-.65); // Move forward
					Left.Set(.65); // Into AUTO Zone
					Wait(1.4); // Hold high speed for 1.6 seconds
					Right.Set(-0.5); // Reduce speed
					Left.Set(0.5); // to .5 output
					Wait(.3); // hold for just .3 seconds
					Right.Set(-0.3); // Now reduce again
					Left.Set(0.3); // to .3 output
					Wait(.4); // Hold for .4 seconds
					Right.Set(-0.1); // Slow again
					Left.Set(0.1); // To .1 speed (may not actually move robot, just to help slow to a stop)
					Wait(.5); // Hold for .5 seconds
					Right.Set(0); // Stop the robot
					Left.Set(0); // In the AUTO ZONE
					Lift.Set(0); // AUTO is finished
				}
			}
		//}
	else
		Lift.Set(0); // Make sure elevator is stopped
	ran = true; // End loop after just one iteration
	}
	// After ran = true, although AutonomousPeriodic continues to run, nothing happens
	// This code is only in AutonomousPeriodic to separate it from sensor and automode initialization
}

void Team1257Robot::TestInit() // Test code
{
	angle.Reset();

	SmartDashboard::PutBoolean("AUTO_ROBOT", true);
	SmartDashboard::PutBoolean("AUTO_TOTE", false);
	SmartDashboard::PutBoolean("AUTO_CONTAINER", true);
	SmartDashboard::PutBoolean("AUTO_START", true);

	PIDangle(0, 0, 0, 90, .5);
}

void Team1257Robot::TestPeriodic() //This runs in a loop during test mode.
{

}

bool Team1257Robot::TestAngle(int theta) // Returns false until the robot has completed a turn of measure "angle"
{
	SmartDashboard::PutString("DB/String 0", ToString(angle.GetAngle()));
	float a = angle.GetAngle(); //get angle from gyro
	if((int)dAbs(a) < theta) //if the magnitude is less than the target angle, return true
	{
 		return true;//Return true.
	}

	else //if not
	{
		return false;//Return false.
	}
}

void Team1257Robot::PIDangle(float p, float i, float d, float setpoint, float speedmax)
{
	Timer timer;
	timer.Start();
	if(!p)
		p = (speedmax/setpoint) * 1.25;
	if(setpoint < 0)
		p *= -1;
	angle.Reset();
	PIDController angleturn(p, i, d, &angle, &Right);
	angleturn.SetSetpoint(setpoint);
	angleturn.Enable();
	while(dAbs(angleturn.GetError()) > 15)
	{
		Left.Set(Right.Get());
		if(timer.Get() > 5)
			break;
	}
	angleturn.Disable();
	angleturn.Reset();
	angle.Reset();
}

double Team1257Robot::accel(Joystick& Stick, int axis, double& current, double sf, double inc) //smoothly speed up to target speed
{
	double raw = Stick.GetRawAxis(axis);
	if(raw > current && raw > 0) // If speeding up, increment it there, instead of sudden jerk
		current += inc;
	if(raw < current && raw < 0) // Same as above, taking into account negative joyStick1 values
		current -= inc;
	if(dAbs(raw) < dAbs(current)) // If the target speed is lesser, reduce to that instantly, like for stopping
		current = raw;
	if(dAbs(raw) < .2) //Taking into account SLIGHTLY off-centered axes
		current = 0;
	return (current * sf);
}

inline double Team1257Robot::dAbs(double x) //absolute value for double
{
	if(x >= 0)//If x is positive
		return x;//Leave it the way it is.
	else//If it's negative
		return -x;//Negate it to make it positive.
}

START_ROBOT_CLASS(Team1257Robot); //Actually run the robot
