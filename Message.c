/* * This program was written by:
    @author Sanjay Shrikanth 
 * 
 * CSE13 E Lab 9
 *
 */
#include "Message.h"
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

//CMPE13 Support Library
#include "BOARD.h"

#define PAYLOAD_CHA "CHA"
#define PAYLOAD_ACC "ACC"
#define PAYLOAD_REV "REV"
#define PAYLOAD_SHO "SHO"
#define PAYLOAD_RES "RES"

#define PAYLOAD_IDENTIFIER_LEN 3

#define PARAM_SEPARATOR ","     //payload possible characters
#define START_DELIM '$'
#define CHECKSUM_DELIM '*'
#define END_DELIM '\n'
#define PARAM_SEPARATOR_CHAR ','

#define HEX_NUM_START '0'      //hex chars
#define HEX_NUM_END '9'
#define HEX_LETTER_START 'A'
#define HEX_LETTER_END 'F'
#define NUM_BASE 16           //base of the checksum number (for conversion)

#define MAX_NUM_PARAMS 3

typedef enum {               //this allows for the decode to switch states cleanly
    WAIT_DELIMITER,
    RECORD_PAYLOAD,
    RECORD_CHECKSUM
} DecodeState;


static DecodeState state = WAIT_DELIMITER;           //begin state for the decode
                       
static char currentPayload[MESSAGE_MAX_PAYLOAD_LEN];  //static variables that represent decoding payload, checksum
static char currentChecksum[MESSAGE_CHECKSUM_LEN]; 
static unsigned int payloadCount = 0, checksumCount = 0; //allow to iterate through the decode payload

static char cha[] = PAYLOAD_CHA;    //these are all strings of the types of message(for comparison)
static char acc[] = PAYLOAD_ACC;
static char rev[] = PAYLOAD_REV;
static char sho[] = PAYLOAD_SHO;
static char res[] = PAYLOAD_RES;

static MessageType checkPayload(char pay[PAYLOAD_IDENTIFIER_LEN]);  //sees if the messagetype is valid
static uint8_t isHexChar(char n);   
static void reset();  //resets the fields

/**
 * Given a payload string, calculate its checksum
 * 
 * @param payload       //the string whose checksum we wish to calculate
 * @return   //The resulting 8-bit checksum 
 */
uint8_t Message_CalculateChecksum(const char* payload) {
    int len = strlen(payload);
    if (len > MESSAGE_MAX_PAYLOAD_LEN) {
        return 0;
    }
    uint8_t checksum = 0;
    int i;
    for (i = 0; i < len; i++) {
        checksum ^= payload[i];
    }
    return checksum;
}

/**
 * ParseMessage() converts a message string into a BB_Event.  The payload and
 * checksum of a message are passed into ParseMessage(), and it modifies a
 * BB_Event struct in place to reflect the contents of the message.
 * 
 * @param payload       //the payload of a message
 * @param checksum      //the checksum (in string form) of  a message,
 *                          should be exactly 2 chars long, plus a null char
 * @param message_event //A BB_Event which will be modified by this function.
 *                      //If the message could be parsed successfully,
 *                          message_event's type will correspond to the message type and 
 *                          its parameters will match the message's data fields.
 *                      //If the message could not be parsed,
 *                          message_events type will be BB_EVENT_ERROR
 * 
 * @return STANDARD_ERROR if:
 *              the payload does not match the checksum
 *              the checksum string is not two characters long
 *              the message does not match any message template
 *          SUCCESS otherwise
 * 
 * Please note!  sscanf() has a couple compiler bugs that make it a very
 * unreliable tool for implementing this function. * 
 */
int Message_ParseMessage(const char* payload,
        const char* checksum_string, BB_Event * message_event) {

    message_event->param0 = 0;
    message_event->param1 = 0;
    message_event->param2 = 0;

    int p_len = strlen(payload);
    if (p_len > MESSAGE_MAX_PAYLOAD_LEN) {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    char message[PAYLOAD_IDENTIFIER_LEN + 1];
    int i;
    for (i = 0; i < PAYLOAD_IDENTIFIER_LEN; i++) {   //extracts the 3 chars of the payload for examination
        message[i] = payload[i];
    }

    message[i + 1] = '\0';

    MessageType m = checkPayload(message);
    if (m == MESSAGE_ERROR) {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    uint8_t payloadCM = Message_CalculateChecksum(payload);

    if (strlen(checksum_string) != MESSAGE_CHECKSUM_LEN) {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    char *ptr;
    uint8_t paramCM = strtol(checksum_string, &ptr, NUM_BASE);  //converts the checksum to a hex number

    if (payloadCM != paramCM) {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }
    int j, k;
    char params[p_len - PAYLOAD_IDENTIFIER_LEN];   
    for (k = 0, j = PAYLOAD_IDENTIFIER_LEN + 1; j < p_len && k < p_len - PAYLOAD_IDENTIFIER_LEN; j++, k++) {
        params[k] = payload[j];    //this extracts the payload fields
    }

    char *token;
    token = strtok(params, PARAM_SEPARATOR);
    message_event->param0 = atoi(token);
    token = strtok(NULL, PARAM_SEPARATOR);     //checks to see if this is the only parameter or not

    if (token == NULL) {
        if (m == MESSAGE_CHA) {
            message_event->type = BB_EVENT_CHA_RECEIVED;
        } else if (m == MESSAGE_ACC) {
            message_event->type = BB_EVENT_ACC_RECEIVED;
        } else if (m == MESSAGE_REV) {
            message_event->type = BB_EVENT_REV_RECEIVED;
        } else {
            message_event->type = BB_EVENT_ERROR;
            return STANDARD_ERROR;
        }
        return SUCCESS;
    }

    message_event->param1 = (uint16_t) atoi(token);   //extracts second parameter

    token = strtok(NULL, PARAM_SEPARATOR);

    if (token == NULL) {
        if (m == MESSAGE_SHO) {
            message_event->type = BB_EVENT_SHO_RECEIVED;
        } else {
            message_event->type = BB_EVENT_ERROR;
            return STANDARD_ERROR;
        }
        return SUCCESS;
    }

    message_event->param2 = (uint16_t) atoi(token);   //extracts third parameter
    message_event->type = BB_EVENT_RES_RECEIVED;

    token = strtok(NULL, PARAM_SEPARATOR);
    if (token != NULL) {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    } else {   //handles too many fields
        if (m != MESSAGE_RES) {
            message_event->type = BB_EVENT_ERROR;
            return STANDARD_ERROR;
        }
    }
    return SUCCESS;
}

/**
 * Encodes the coordinate data for a guess into the string `message`. This string must be big
 * enough to contain all of the necessary data. The format is specified in PAYLOAD_TEMPLATE_COO,
 * which is then wrapped within the message as defined by MESSAGE_TEMPLATE. 
 * 
 * The final length of this
 * message is then returned. There is no failure mode for this function as there is no checking
 * for NULL pointers.
 * 
 * @param message            The character array used for storing the output. 
 *                              Must be long enough to store the entire string,
 *                              see MESSAGE_MAX_LEN.
 * @param message_to_encode  A message to encode
 * @return                   The length of the string stored into 'message_string'.
                             Return 0 if message type is MESSAGE_NONE.
 */
int Message_Encode(char *message_string, Message message_to_encode) {
    char payload[MESSAGE_MAX_PAYLOAD_LEN];
    uint8_t checksum;
    if (message_to_encode.type == MESSAGE_NONE) {
        return 0;
    } else if (message_to_encode.type == MESSAGE_CHA) {
        sprintf(payload, PAYLOAD_TEMPLATE_CHA, message_to_encode.param0);
        checksum = Message_CalculateChecksum(payload);
        sprintf(message_string, MESSAGE_TEMPLATE, payload, checksum);
    } else if (message_to_encode.type == MESSAGE_ACC) {
        sprintf(payload, PAYLOAD_TEMPLATE_ACC, message_to_encode.param0);
        checksum = Message_CalculateChecksum(payload);
        sprintf(message_string, MESSAGE_TEMPLATE, payload, checksum);
    } else if (message_to_encode.type == MESSAGE_REV) {
        sprintf(payload, PAYLOAD_TEMPLATE_REV, message_to_encode.param0);
        checksum = Message_CalculateChecksum(payload);
        sprintf(message_string, MESSAGE_TEMPLATE, payload, checksum);
    } else if (message_to_encode.type == MESSAGE_SHO) {
        sprintf(payload, PAYLOAD_TEMPLATE_SHO, message_to_encode.param0, message_to_encode.param1);
        checksum = Message_CalculateChecksum(payload);
        sprintf(message_string, MESSAGE_TEMPLATE, payload, checksum);
    } else if (message_to_encode.type == MESSAGE_RES) {
        sprintf(payload, PAYLOAD_TEMPLATE_RES, message_to_encode.param0, message_to_encode.param1, message_to_encode.param2);
        checksum = Message_CalculateChecksum(payload);
        sprintf(message_string, MESSAGE_TEMPLATE, payload, checksum);
    }
    return strlen(message_string);
}

/**
 * Message_Decode reads one character at a time.  If it detects a full NMEA message,
 * it translates that message into a BB_Event struct, which can be passed to other 
 * services.
 * 
 * @param char_in - The next character in the NMEA0183 message to be decoded.
 * @param decoded_message - a pointer to a message struct, used to "return" a message
 *                          if char_in is the last character of a valid message, 
 *                              then decoded_message
 *                              should have the appropriate message type.
 *                          if char_in is the last character of an invalid message,
 *                              then decoded_message should have an ERROR type.
 *                          otherwise, it should have type NO_EVENT.
 * @return SUCCESS if no error was detected
 *         STANDARD_ERROR if an error was detected
 * 
 * note that ANY call to Message_Decode may modify decoded_message.
 */
int Message_Decode(unsigned char char_in, BB_Event * decoded_message_event) {
    switch (state) {
        case WAIT_DELIMITER:
            if (char_in == START_DELIM) {
                state = RECORD_PAYLOAD;
            }
            break;
        case RECORD_PAYLOAD:
            if (char_in == CHECKSUM_DELIM) {
                state = RECORD_CHECKSUM;
                break;
            }
            if (payloadCount > MESSAGE_MAX_PAYLOAD_LEN - 1) {   //if the string's payload is too long
                decoded_message_event->type = BB_EVENT_ERROR;
                state = WAIT_DELIMITER;
                reset();
                return STANDARD_ERROR;
            }
            if (char_in == START_DELIM || char_in == END_DELIM) {  //if a delimiter is reached early
                decoded_message_event->type = BB_EVENT_ERROR;
                state = WAIT_DELIMITER;
                reset();
                return STANDARD_ERROR;
            }
            currentPayload[payloadCount] = char_in;
            payloadCount++;                      //increments the index of the payload string
            break;
        case RECORD_CHECKSUM:
            if (char_in == END_DELIM) {
                int res = Message_ParseMessage(currentPayload, currentChecksum, decoded_message_event);
                state = WAIT_DELIMITER;
                if (res == SUCCESS) {
                    reset();
                    break;
                } else {
                    decoded_message_event->type = BB_EVENT_ERROR;
                    reset();
                    return STANDARD_ERROR;
                }
            }

            if (checksumCount > MESSAGE_CHECKSUM_LEN - 1) {   //checks if the checksum length is too long
                decoded_message_event->type = BB_EVENT_ERROR;
                state = WAIT_DELIMITER;
                return STANDARD_ERROR;
            }

            if (isHexChar(char_in) == TRUE) {
                currentChecksum[checksumCount] = char_in;
                checksumCount++;
            } else {
                decoded_message_event->type = BB_EVENT_ERROR;
                state = WAIT_DELIMITER;
                return STANDARD_ERROR;
            }
            break;
    }
    return SUCCESS;
}

/*
 * this function sees if the payload identifier is valid or not
 */
static MessageType checkPayload(char pay[PAYLOAD_IDENTIFIER_LEN + 1]) {
    if (strcmp(pay, cha) == 0) {
        return MESSAGE_CHA;
    } else if (strcmp(pay, acc) == 0) {
        return MESSAGE_ACC;
    } else if (strcmp(pay, rev) == 0) {
        return MESSAGE_REV;
    } else if (strcmp(pay, sho) == 0) {
        return MESSAGE_SHO;
    } else if (strcmp(pay, res) == 0) {
        return MESSAGE_RES;
    } else {
        int i;
        if (strcmp(pay, sho) != 0) {
            for (i = 0; i < PAYLOAD_IDENTIFIER_LEN; i++) {
                if (pay[i] != sho[i]) {
                    return MESSAGE_ERROR;
                }
            }
            return MESSAGE_SHO;
        }
        if (strcmp(pay, rev) != 0) {
            for (i = 0; i < PAYLOAD_IDENTIFIER_LEN; i++) {
                if (pay[i] != rev[i]) {
                    return MESSAGE_ERROR;
                }
            }
            return MESSAGE_REV;
        }
        return MESSAGE_ERROR;
        ;
    }
}

/*
 this function checks if a char is a valid hex character (0-9, A-F)
 */
static uint8_t isHexChar(char n) {
    if (((n >= HEX_NUM_START) && (n <= HEX_NUM_END)) || ((n >= HEX_LETTER_START) && (n <= HEX_LETTER_END))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * this function resets all the static fields
 */
static void reset() {
    payloadCount = 0;
    checksumCount = 0;

    int i, k;

    for (k = 0; k < MESSAGE_MAX_PAYLOAD_LEN; k++) {
        if (currentPayload[k] != '\0') {
            currentPayload[k] = '\0';
        }
    }

    for (i = 0; i < MESSAGE_CHECKSUM_LEN; i++) {
        if (currentChecksum[i] != '\0') {
            currentChecksum[i] = '\0';
        }
    }
}