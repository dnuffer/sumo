#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in1,    line_follower,  sensorLineFollower)
#pragma config(Sensor, in4,    potentiometer,  sensorPotentiometer)
#pragma config(Sensor, dgtl1,  button1,        sensorTouch)
#pragma config(Sensor, dgtl2,  button2,        sensorTouch)
#pragma config(Sensor, dgtl3,  button3,        sensorTouch)
#pragma config(Sensor, dgtl8,  ultrasonic,     sensorSONAR_mm)
#pragma config(Sensor, I2C_1,  sonar_rotate,   sensorQuadEncoderOnI2CPort,    , AutoAssign)
#pragma config(Motor,  port1,           sonar_rotate,  tmotorVex393, openLoop, encoder, encoderPort, I2C_1, 1000)
#pragma config(Motor,  port2,           left,          tmotorVex393HighSpeed, openLoop)
#pragma config(Motor,  port3,           right,         tmotorVex393HighSpeed, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#define MAX_INT 32767

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define CONSTRAIN(value, min_val, max_val) (MAX(min_val, MIN(value, max_val)))

typedef struct
{
	float CLOSE_ENOUGH;
	float KP;
	float KI;
	float KD;
	long MAX_CHANGE;
	long MIN_OUTPUT;
	long MAX_OUTPUT;
	long EFFECTIVE_MIN;
	float MAX_INTEGRAL;
} pid_controller_t;

typedef struct
{
	long prev_error;
	float integral;
	long prev_output;
	float prev_derivative;
	long prev_time;
} pid_state_t;

void init_pid_state(pid_state_t* s, pid_controller_t* controller)
{
	s->prev_error = controller->CLOSE_ENOUGH + 1;
	s->integral = 0;
	s->prev_output = 0;
	s->prev_derivative = 0;
	s->prev_time = 0;
}

long update_pid_controller(pid_controller_t* controller, pid_state_t* state, long setpoint, long measured_value)
{
	long time = nPgmTime;
	long elapsed_msec = time - state->prev_time;

	// If no measurable time has passed, we can't divide by zero, so return the last output value and don't update anything
	if (elapsed_msec <= 0)
	{
		writeDebugStreamLine("elapsed_msec == 0!");
		return state->prev_output;
	}

	long error = setpoint - measured_value;

	if (abs(error) > controller->CLOSE_ENOUGH && state->integral < controller->MAX_INTEGRAL)
		state->integral += error * elapsed_msec;

	float derivative = (float)(error - state->prev_error) / elapsed_msec;

	//writeDebugStreamLine("measured_value: %d   integral: %.2f    derivative: %.2f", measured_value, state->integral, derivative);

	float output =
			controller->KP * error
			+ controller->KI * state->integral
			+ controller->KD * derivative;

	//writeDebugStreamLine("output before correction: %.0f", output);

	output = CONSTRAIN(output,
			state->prev_output - controller->MAX_CHANGE,
			state->prev_output + controller->MAX_CHANGE);
	//writeDebugStreamLine("output after correction 1: %.0f", output);

	if (output < -1)
		output = CONSTRAIN(output, controller->MIN_OUTPUT, -controller->EFFECTIVE_MIN);
	else if (output > 1)
		output = CONSTRAIN(output, controller->EFFECTIVE_MIN, controller->MAX_OUTPUT);
	else
		output = 0;

	//writeDebugStreamLine("output after correction: %d", (long)output);

	state->prev_error = error;
	state->prev_output = output;
	state->prev_derivative = derivative;
	state->prev_time = time;

	return output;
}

const int SONAR_BINS = 6;
int sonar_readings[SONAR_BINS];
int straight_ahead = 0;

const float ENCODER_COUNTS_PER_REV = 627.2;
const float ENCODER_COUNTS_PER_BIN = ENCODER_COUNTS_PER_REV / SONAR_BINS;

long mod(long x, long y)
{
	if (x < 0)
	{
		x += y * ((-x / y) + 1);
		//return x % y + y;
	}
	return x % y;
}

void move_to_position(long setpoint)
{
	pid_controller_t controller;
	controller.CLOSE_ENOUGH = 15;
	controller.KP = -0.6; //0.6 * Ku; //-1.0;
	controller.KI = -0.01;//-0.01; // 2 * KP / Tu; //-0.01;
	controller.KD = -45.0;//-5.0; //KP * Tu / 8; //-100.0;
	controller.MAX_CHANGE = 75;
	controller.MAX_OUTPUT = 50;
	//controller.MAX_OUTPUT = 127;
	controller.MIN_OUTPUT = -controller.MAX_OUTPUT;
	controller.EFFECTIVE_MIN = 20;
	controller.MAX_INTEGRAL = controller.MAX_OUTPUT / controller.KI * 2;

	pid_state_t state;
	init_pid_state(&state, &controller);

	while (abs(state.prev_error) > controller.CLOSE_ENOUGH)
	{
		long output = update_pid_controller(&controller, &state, setpoint, nMotorEncoder[sonar_rotate]);

		motor[sonar_rotate] = output;

		int reading = SensorValue[ultrasonic];
		long encoder_reading = nMotorEncoder[sonar_rotate];
		const int MIN_SONAR_DISTANCE = 30;
		if (reading > MIN_SONAR_DISTANCE)
			sonar_readings[mod(encoder_reading / ENCODER_COUNTS_PER_BIN, SONAR_BINS)] = reading;
		wait1Msec(10);
	}

	motor[sonar_rotate] = 0;
}

void reset_readings()
{
	for (int i = 0; i < SONAR_BINS; i++)
	{
		sonar_readings[i] = MAX_INT;
	}
}

int find_closest_sonar_bin()
{
	int min_idx = 0;
	for (int i = 1; i < SONAR_BINS; i++)
	{
		if (sonar_readings[i] < sonar_readings[min_idx])
			min_idx = i;
	}
	return min_idx;
}

void print_closest()
{
	int min_idx = find_closest_sonar_bin();
	writeDebugStreamLine("closest is at %d deg, distance of %d mm", min_idx * 360/SONAR_BINS, sonar_readings[min_idx]);
}

#define	Calibrating_Sonar 0
#define Follow_Closest_Object 1
#define Do_Nothing 2
int robot_mode = Do_Nothing;

int get_do_nothing_mode_btn()
{
	// vexRT[Btn6D]
	return SensorValue[button1];
}

int get_calibrating_mode_btn()
{
	// vexRT[Btn6U]
	return SensorValue[button2];
}

int get_follow_closest_object_mode_btn()
{
	// vexRT[Btn7U]
	return SensorValue[button3];
}

void check_and_switch_mode()
{
	if (get_do_nothing_mode_btn())
	{
		while (get_do_nothing_mode_btn())
			wait1Msec(1);
		robot_mode = Do_Nothing;
		writeDebugStreamLine("Switching to Do_Nothing mode");
	}
	else if (get_calibrating_mode_btn())
	{
		while (get_calibrating_mode_btn())
			wait1Msec(1);
		robot_mode = Calibrating_Sonar;
		// PlaySound(soundBeepBeep);
		writeDebugStreamLine("Switching to calibrate_sonar_mode");
	}
	else if (get_follow_closest_object_mode_btn())
	{
		while (get_follow_closest_object_mode_btn())
			wait1Msec(1);
		robot_mode = Follow_Closest_Object;
		writeDebugStreamLine("Switching to Follow_Closest_Object mode");
	}
}

#define MOTOR_POWER 127
void turn_left()
{
	motor[left] = 0;//-MOTOR_POWER;
	motor[right] = MOTOR_POWER;
}

void turn_right()
{
	motor[left] = MOTOR_POWER;
	motor[right] = 0;//-MOTOR_POWER;
}

void go_straight()
{
	motor[left] = 0;//MOTOR_POWER;
	motor[right] = 0;//MOTOR_POWER;
}

void stop_moving()
{
	motor[left] = 0;
	motor[right] = 0;
}

task main()
{
	//Clear the encoders associated with the left and right motors
	nMotorEncoder[sonar_rotate] = 0;

	pid_controller_t controller;
	controller.CLOSE_ENOUGH = 15;
	controller.KP = -0.6; //0.6 * Ku; //-1.0;
	controller.KI = -0.01;//-0.01; // 2 * KP / Tu; //-0.01;
	controller.KD = -45.0;//-5.0; //KP * Tu / 8; //-100.0;
	controller.MAX_CHANGE = 75;
	controller.MAX_OUTPUT = 50;
	//controller.MAX_OUTPUT = 127;
	controller.MIN_OUTPUT = -controller.MAX_OUTPUT;
	controller.EFFECTIVE_MIN = 20;
	controller.MAX_INTEGRAL = controller.MAX_OUTPUT / controller.KI * 2;

	pid_state_t state;
	init_pid_state(&state, &controller);

	int sonar_destination = 0;

	while (1)
	{
		check_and_switch_mode();
		switch (robot_mode)
		{
			case Do_Nothing:
				stop_moving();
				motor[sonar_rotate] = 0;
				break;
			case Calibrating_Sonar:
				stop_moving();
				reset_readings();
				move_to_position(720); // really 627.2 counts per revolution in high-torque configuration, but play in the mechanism, it doesn't actually go the whole distance, so compensate a bit.
				//http://www.robotc.net/wiki/Tutorials/Programming_with_the_new_VEX_Integrated_Encoder_Modules
				move_to_position(0);
				straight_ahead = find_closest_sonar_bin();
				writeDebugStreamLine("got calibration of %d", straight_ahead);
				move_to_position(ENCODER_COUNTS_PER_BIN * (straight_ahead + 0.5));
				robot_mode = Do_Nothing;
				break;

			case Follow_Closest_Object:
				// keep moving the sonar

				// turn the robot towards the closest object
				int closest_bin = find_closest_sonar_bin();
				writeDebugStreamLine("closest_bin: %d", closest_bin);
				int degrees_to_turn = (closest_bin - straight_ahead) * (360/SONAR_BINS);
				writeDebugStreamLine("degrees_to_turn: %d", degrees_to_turn);
				while (degrees_to_turn > 180)
				{
					degrees_to_turn -= 360;
				}
				while (degrees_to_turn < -180)
				{
					degrees_to_turn += 360;
				}
				writeDebugStreamLine("degrees_to_turn: %d", degrees_to_turn);

				if (degrees_to_turn < 360/SONAR_BINS)
				{
					turn_left();
				}
				else if (degrees_to_turn > 360/SONAR_BINS)
				{
					turn_right();
				}
				else
				{
					go_straight();
				}

				long encoder_reading = nMotorEncoder[sonar_rotate];
				if (encoder_reading > 0 - controller.CLOSE_ENOUGH && encoder_reading < 0 + controller.CLOSE_ENOUGH)
					sonar_destination = 720;
				else if (encoder_reading > 720 - controller.CLOSE_ENOUGH && encoder_reading < 720 + controller.CLOSE_ENOUGH)
					sonar_destination = 0;

				long output = update_pid_controller(&controller, &state, sonar_destination, nMotorEncoder[sonar_rotate]);

				motor[sonar_rotate] = output;

				int reading = SensorValue[ultrasonic];
				const int MIN_SONAR_DISTANCE = 30;
				if (reading > MIN_SONAR_DISTANCE)
					sonar_readings[mod(encoder_reading / ENCODER_COUNTS_PER_BIN, SONAR_BINS)] = reading;
				//wait1Msec(1);

				break;

			default:
				return;
		}

		// spins the sonar
		//reset_readings();
		//move_to_position(720); // really 627.2 counts per revolution in high-torque configuration, but play in the mechanism, it doesn't actually go the whole distance, so compensate a bit.
		//http://www.robotc.net/wiki/Tutorials/Programming_with_the_new_VEX_Integrated_Encoder_Modules

		//print_closest();
		//move_to_position(0);
		//print_closest();
		/*
		move_to_position(1000);
		wait1Msec(2000);
		move_to_position(0);
		wait1Msec(2000);
		wait1Msec(500);
		move_to_position(5100);
		wait1Msec(500);
		move_to_position(-100);
		wait1Msec(500);
		move_to_position(-200);
		wait1Msec(500);
		*/

		/*
		//While less than 1000 encoder counts of the right motor
		while(abs(nMotorEncoder[motor1]) < 200)
		{
			//Move forward at half power
			motor[motor1] = 100;
			//motor[leftMotor]	= 63;
		}

		motor[motor1] = 0;
	  //Clear the encoders associated with the left and right motors
		//nMotorEncoder[motor1] = 0;
		//nMotorEncoder[leftMotor] = 0;

		wait1Msec(500);

		//While less than 1000 encoder counts of the right motor
		int sign = nMotorEncoder[motor1] > 0 ? 1 : -1;
		while(sign * nMotorEncoder[motor1] > 0)
		{
			//Move in reverse at half power
			motor[motor1] = -100;
			//motor[leftMotor]	= -63;
		}

		motor[motor1] = 0;
		*/
		/*
		startMotor(motor1, 40);
		untilRotations(1.0/10.0, I2C_1);
		stopMotor(motor1);
		wait(0.2);

		startMotor(motor1, 40);
		untilRotations(1.0/10.0, I2C_1);
		stopMotor(motor1);
		wait(0.2);

		startMotor(motor1, -40);
		untilRotations(1.0/10.0, I2C_1);
		stopMotor(motor1);
		wait(0.2);

		startMotor(motor1, -40);
		untilRotations(1.0/10.0, I2C_1);
		stopMotor(motor1);
		wait(0.2);
		*/
	}


}
