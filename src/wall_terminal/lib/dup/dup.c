/** @file dup.c
 *  @brief  Created on: July 29, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <dup.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

dup_encoded_header_t dup_encode_header(dup_decoded_header_t header){
	return ( (header.command << 5) | header.device_id );
}

dup_decoded_header_t dup_decode_header(dup_encoded_header_t header){
	return (dup_decoded_header_t){
		.device_id = header & 0x1F,
		.command = header >> 5
	};
}

uint8_t dup_encode_parameters_command_set_pwm(uint8_t pwm_channel, uint8_t pwm_percentage){
	return ( (pwm_channel << 7) | (pwm_percentage <= 100 ? pwm_percentage : 100) );
}

void dup_decode_parameters_command_set_pwm(uint8_t encoded_parameters, uint8_t *pwm_channel, uint8_t *pwm_percentage){

	if(pwm_channel == NULL || pwm_percentage == NULL)
		return;

	*pwm_channel = encoded_parameters >> 7;
	*pwm_percentage = encoded_parameters & 0x7F;
}
