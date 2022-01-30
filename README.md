# CarParkManagementSystem
In this project me and my collegue Elisa Composta realized an automated car park managemenet system using MSP432 and its boosterpack (Texas Instruments) , to control the number of cars entering a car park to reduce useless traffic and to avoid people wasting their time searching for a (non-existing) park.

## **WORKING SCHEME**
Car are allowed to enter only if there are available spaces, and their access is controlled by a bar: when no more spaces are available the bar doesn’t open.
A car is sensed when the light sensor reveals the reduction of light.
If the park is not full (depending on a counter value also visible to the user on the LCD display) the car can enter, so the bar (controlled by a servo motor) opens, and the buzzer notifies to the user that it’s moving with an acoustic signal. There is also a visible response: when the bar is opening the LED turns to white, and when it’s completely open it becomes green.
When the car enters (the light gets back to its nominal value) the bar closes; when it’s moving, the user is notified by the buzzer and the LED as previously explained.
All the time the bar is closed, even when the park is full, the LED is red.
All the phases of this system are written on the LCD screen.

## **SOFTWARE DESCRIPTION**
Light sensor: we periodically sample the light value and compare it with the one that we took when the application was launched; when the difference passes a threshold, the others operations can start.
LEDs: we simply turn on and turn off them (mixing colors to make the white) when necessary.
Display: There are some strings that are fixed and never change during the course of the program (like title and the sub-title) and others can change depending on the state of the system (like the counter, that is decreased every time a car enters, or the last string that notify the state of the bar to the user/when the park is full). Each type of string has to be initialized (changing colors, font, size).
Piezo buzzer and servo motor: both of them are controlled by a PWM, which is different because they use different CC registers and duty cycle value. We instantiate 2 different structs and set their fields with values found experimentally for a good sound of the buzzer, and the current 90° degrees angle of the servo motor (bar). Buzzer makes sounds when the correlated PWM duty cycle is different from 0, and is silent otherwise. We did the same for moving servo motor, by changing the duty cycle of its PWM (SERVO_MOTOR_START and SERVO_MOTOR_END).

## **PROBLEMS ENCOUNTERED DURING TESTING**
The main problems were correlated to the use of servo motor and buzzer. There were some conflicts between their PWM because they share the same timer. After many trials and some advice from our colleagues we decided to switch off the servo motor when it’s not used. It this way we solved the problem and also, we cut down energy waste when the system is not used (e.g., when there isn’t any car for many hours).
Another problem we encountered was that all the operations are inside an infinite while loop, but some operations need to be run only once per car. Solution: control this “critical section” of code with a boolean (“isWaiting”), implementing a sort of mutex. In this way we also avoided calling functions multiple times that didn’t require it, which would have been unnecessarily expensive for the CPU.
Of course, there were also other minor problems, but we do not mention them.

## **CONCLUSIONS**
To improve this system, it would be possible to add an analogue system to count car leaving the park and freeing spaces. It would have the same behaviour but increasing a shared counter.
