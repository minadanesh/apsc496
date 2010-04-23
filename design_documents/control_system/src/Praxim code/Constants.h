//Positioning Mode:
//  Position Control = -1
//  Position Profile Control = 1

#define Mode	-1


//MOTOR CONSTANTS:
#define continuousCurrentLimit  1000
#define outputCurrentLimit	    500

#define minPositionLimit	    80000
#define maxPositionLimit	    180000

//Position Control
#define position_Pgain	    	100
#define position_Igain	    	10
#define position_Dgain    		200

//Profile Control
#define maxProfileVelocity    	25000
#define profileVelocity    		5000
#define profileAcceleration    	200000
#define profileDeceleration    	200000
#define maxFollowingError    	200000


#define motorCpr			135168

#define cprA 10000
#define cprB 1440
#define cpiC 2000//914

#define X_stretch	50.0		// X-stretch for ellipse
#define Y_stretch	20.0		// Y-stretch for ellipse
#define Z_stretch	40.0		// Z-stretch for ellipse
#define X_base    	0		// X-base point to center of ellipse
#define Y_base    	60.0		// Y-base point to center of ellipse
#define Z_base 		70.0		// Z-base point to center of ellipse
#define flatHeight	60.0
#define sineHeight	10.0

#define blockerOffset 21868
#define motorOffset	90217

/************************************/
/*  2D								*/
/************************************/

//#define dimension		2
//#define offsetA    	3170
//#define offsetB 		233
//#define offsetC		0


/************************************/
/*  3D								*/
/************************************/

#define dimension	3

#define offsetA		7855
#define offsetB 	370
#define offsetC		888
