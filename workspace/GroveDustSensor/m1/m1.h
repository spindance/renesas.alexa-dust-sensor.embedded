/*
 * m1.h
 *
 *  Created on: May 3, 2017
 *      Author: demo
 */

#ifndef M1_H_
#define M1_H_


// Error Codes
enum {
    M1_SUCCESS = 0,
    M1_ERROR_INVALID_URL,
    M1_ERROR_UNABLE_TO_CONNECT,
    M1_ERROR_UNABLE_TO_DISCONNECT,
    M1_ERROR_NOT_CONNECTED,
    M1_ERROR_ALREADY_CONNECTED,
    M1_ERROR_NULL_PAYLOAD,
    M1_ERROR_UNABLE_TO_PUBLISH,
    M1_ERROR_NULL_CALLBACK,
    M1_ERROR_BAD_CREDENTIALS,
};


typedef struct {
    char user_id[12];
    char password[64];
} user_credentials_t;


typedef struct {
    char apikey[49];
    char proj_id[12];
} project_credentials_t;

#endif /* M1_H_ */
