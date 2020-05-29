#ifndef _FISHINOHOMEAUTO_H
#define FISHINOHOMEAUTO

// appliances definitions
typedef struct
{
	// appliance id
	char *id;
	
	// appliance type (digital output = 0, digital input = 1, analog output = 2, analog input = 3)
	uint8_t kind:2;
	
	// i/o pin connected to appliance
	uint8_t io:6;
	
} Appliance;

#endif
