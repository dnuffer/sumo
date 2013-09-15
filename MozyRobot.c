#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in1,    line_follower,  sensorLineFollower)
#pragma config(Sensor, in4,    potentiometer,  sensorPotentiometer)
#pragma config(Sensor, dgtl1,  limit_switch,   sensorTouch)
#pragma config(Sensor, dgtl2,  bumper_switch,  sensorTouch)
#pragma config(Sensor, dgtl8,  ultrasonic,     sensorSONAR_mm)
#pragma config(Sensor, dgtl10, encoder,        sensorQuadEncoder)
#pragma config(Sensor, I2C_1,  ,               sensorQuadEncoderOnI2CPort,    , AutoAssign)
#pragma config(Motor,  port1,           motor1,        tmotorVex393, PIDControl, encoder, encoderPort, I2C_1, 1000)
#pragma config(Motor,  port2,           sonar_rotate,  tmotorVex393, openLoop, encoder, encoderPort, dgtl10, 1000)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

long max(long x, long y)
{
	return x > y ? x : y;
}

long min(long x, long y)
{
	return x > y ? y : x;
}

long constrain(long value, long min_val, long max_val)
{
	return max(min_val, min(value, max_val));
}

void move_to_position(long setpoint)
{
	const long CLOSE_ENOUGH = 5;
	long prev_error = CLOSE_ENOUGH+1;
	long integral = 0;
	long prev_output = 0;
	long prev_derivative = 0;
	// Ziegler-Nichols: Ku = -100, Tu = 5
	// no overshoot.
	//const float Ku = -100.0;
	//const float Tu = 5.0;
	const float KP = -0.5; //0.6 * Ku; //-1.0;
	const float KI = -0.01;//-0.01; // 2 * KP / Tu; //-0.01;
	const float KD = -50.0;//-5.0; //KP * Tu / 8; //-100.0;
	const long MAX_CHANGE = 25;
	const long MIN_OUTPUT = -100;
	const long MAX_OUTPUT = 100;
	const long EFFECTIVE_MIN = 20;
	const float MAX_INTEGRAL = MAX_OUTPUT / KI * 2;
	while (abs(prev_error) > CLOSE_ENOUGH || abs(prev_output) > EFFECTIVE_MIN || abs(prev_derivative) > 0)
	{
		long encoding = nMotorEncoder[motor1];

		long error = setpoint - encoding;

		if (abs(error) > CLOSE_ENOUGH && integral < MAX_INTEGRAL)
			integral += error;

		long derivative = (error - prev_error);

		writeDebugStreamLine("encoding: %d   integral: %d    derivative: %d", encoding, integral, derivative);

		float output =
				KP * error
				+ KI * integral
				+ KD * derivative;

		writeDebugStreamLine("output before correction: %.0f", output);

		output = constrain(output,
				prev_output - MAX_CHANGE,
				prev_output + MAX_CHANGE);
		writeDebugStreamLine("output after correction 1: %.0f", output);

		if (output < -1)
			output = constrain(output, MIN_OUTPUT, -EFFECTIVE_MIN);
		else if (output > 1)
			output = constrain(output, EFFECTIVE_MIN, MAX_OUTPUT);
		else
			output = 0;

		writeDebugStreamLine("output after correction: %.0f", output);

		motor[motor1] = output;

		prev_error = error;
		prev_output = output;
		prev_derivative = derivative;
		wait1Msec(1);
	}

	motor[motor1] = 0;
}

task main()
{
	//Clear the encoders associated with the left and right motors
	nMotorEncoder[motor1] = 0;

	while (1)
	{
		for (int i = 0; i <= 300; i += 50)
		{
			move_to_position(i);
			wait1Msec(200);
		}
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
