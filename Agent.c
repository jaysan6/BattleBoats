//CSE 13E Lab 9 Written by Victor Sowa 5/29/2020
// This is the Agent source file
#include <stdio.h>
#include <stdlib.h>

#include "Agent.h"
#include "Negotiation.h"
#include "Field.h"
#include "Oled.h"
#include "FieldOled.h"

//this struct contains all the info for the agent to run
typedef struct {
    AgentState state;
    int turnCount;
    NegotiationData A;
    NegotiationData hashA;
    NegotiationData B;
    Message message;
    Field own_field;
    Field opp_field;
    int hashVerify;
    NegotiationOutcome coinFlip;
    GuessData own_guessData;
    GuessData opp_guessData;
    FieldOledTurn turn;
} AgentData;

static AgentData agentData;
static char oled_string[200] = "";
/**
 * The Init() function for an Agent sets up everything necessary for an agent before the game
 * starts.  At a minimum, this requires:
 *   -setting the start state of the Agent SM.
 *   -setting turn counter to 0
 * If you are using any other persistent data in Agent SM, that should be reset as well.
 * 
 * It is not advised to call srand() inside of AgentInit.  
 *  */
void AgentInit(void)
{
    agentData.state = AGENT_STATE_START;
    agentData.turnCount = 0;
    agentData.turn = FIELD_OLED_TURN_NONE;
    agentData.hashVerify = 0;
    agentData.message.type = MESSAGE_NONE;
    agentData.coinFlip = HEADS;
    agentData.A = 0;
    agentData.B = 0;
    agentData.hashA = 0;
    agentData.message.param0 = 0;
    agentData.message.param1 = 0;
    agentData.message.param2 = 0;
    agentData.opp_guessData.row = 0;
    agentData.opp_guessData.col = 0;
    agentData.opp_guessData.result = RESULT_MISS;
    agentData.own_guessData.row = 0;
    agentData.own_guessData.col = 0;
    agentData.own_guessData.result = RESULT_MISS;
}

/**
 * AgentRun evolves the Agent state machine in response to an event.
 * 
 * @param  The most recently detected event
 * @return Message, a Message struct to send to the opponent. 
 * 
 * If the returned Message struct is a valid message
 * (that is, not of type MESSAGE_NONE), then it will be
 * passed to the transmission module and sent via UART.
 * This is handled at the top level! AgentRun is ONLY responsible 
 * for generating the Message struct, not for encoding or sending it.
 */
Message AgentRun(BB_Event event)
{
    switch(agentData.state)
    {
        case AGENT_STATE_START:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                //reset all data
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
                agentData.state = AGENT_STATE_START;
                agentData.turn = FIELD_OLED_TURN_NONE;
            }
            else if (event.type == BB_EVENT_START_BUTTON)
            {
                //generate A
                agentData.A = (NegotiationData)rand();
                //generate #a
                agentData.hashA = NegotiationHash(agentData.A);
                //send CHA
                agentData.message.type = MESSAGE_CHA;
                agentData.message.param0  = agentData.hashA; //This should send #a
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                
                //initialize fields
                FieldInit(&agentData.own_field, &agentData.opp_field);
                //place boats
                FieldAIPlaceAllBoats(&agentData.own_field);
                OledClear(0);
                sprintf(oled_string, "Challenging!\nhash_a:%d\nA:%d", agentData.hashA, agentData.A);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.state = AGENT_STATE_CHALLENGING;
            }
            else if (event.type == BB_EVENT_CHA_RECEIVED)
            {
                //store hashA
                agentData.hashA = event.param0;
                //printf("%d", agentData.hashA);
                //generate B
                agentData.B = (NegotiationData)rand();
                //send ACC
                agentData.message.type = MESSAGE_ACC;
                agentData.message.param0  = agentData.B; //This should send B
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                
                //initialize fields
                FieldInit(&agentData.own_field, &agentData.opp_field);
                //place boats
                FieldAIPlaceAllBoats(&agentData.own_field);
                
                OledClear(0);
                sprintf(oled_string, "ACCEPTING\nhash_a:%d\nB:%d", agentData.hashA, agentData.B);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.state = AGENT_STATE_ACCEPTING;
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string, "BB_EVENT_ERROR in the\nAGENT_STATE_START_STATE\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
            
        case AGENT_STATE_CHALLENGING:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //reset all data
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
                agentData.state = AGENT_STATE_START;
            }
            else if(event.type == BB_EVENT_ACC_RECEIVED)
            {
                //send REV
                agentData.message.type = MESSAGE_REV;
                agentData.message.param0  = agentData.A; //the reveals A
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.coinFlip = NegotiateCoinFlip(agentData.A, agentData.B);
                if(agentData.coinFlip == HEADS)
                {
                    agentData.state = AGENT_STATE_WAITING_TO_SEND;
                    agentData.turn = FIELD_OLED_TURN_MINE;
                }
                else if(agentData.coinFlip == TAILS)
                {
                    agentData.state = AGENT_STATE_DEFENDING;
                    agentData.turn = FIELD_OLED_TURN_THEIRS;
                }
                FieldOledDrawScreen(&agentData.own_field, &agentData.opp_field, agentData.turn, 
                                    agentData.turnCount);
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string, "BB_EVENT_ERROR in the\nAGENT_STATE_CHALLENGING\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
                
        case AGENT_STATE_ACCEPTING:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //reset all data
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
                agentData.state = AGENT_STATE_START;
            }
            else if(event.type == BB_EVENT_REV_RECEIVED)
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.A = event.param0;
                //printf("%d", agentData.A);
                agentData.hashVerify = NegotiationVerify(agentData.A, agentData.hashA);
                if(agentData.hashVerify == FALSE)
                {
                    OledClear(0);
                    sprintf(oled_string, "CHEATING DETECTED!\nhash_a:%d\nA:%d", agentData.hashA, 
                            agentData.A);
                    OledDrawString(oled_string);
                    OledUpdate();
                    agentData.A = 0;
                    agentData.hashA = 0;
                    agentData.B = 0;
                    agentData.state = AGENT_STATE_END_SCREEN;
                }
                else
                {
                    agentData.coinFlip = NegotiateCoinFlip(agentData.A, agentData.B);
                
                    if(agentData.coinFlip == HEADS)
                    {
                        agentData.state = AGENT_STATE_DEFENDING;
                        agentData.message.type = MESSAGE_NONE;
                        agentData.message.param0  = 0;
                        agentData.message.param1  = 0;
                        agentData.message.param2  = 0;
                        agentData.turn = FIELD_OLED_TURN_THEIRS;
                    }
                    else if(agentData.coinFlip == TAILS)
                    {
                        //decide guess
                        agentData.own_guessData = FieldAIDecideGuess(&agentData.opp_field);
                        //send SHO
                        agentData.state = AGENT_STATE_ATTACKING;
                        agentData.message.type = MESSAGE_SHO;
                        agentData.message.param0  = agentData.own_guessData.row; //shot data
                        agentData.message.param1  = agentData.own_guessData.col; //shot data
                        agentData.message.param2  = agentData.own_guessData.result;
                        agentData.turn = FIELD_OLED_TURN_MINE;
                    }
                    FieldOledDrawScreen(&agentData.own_field, &agentData.opp_field, agentData.turn, 
                                    agentData.turnCount);
                }
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string, "BB_EVENT_ERROR in the\nAGENT_STATE_ACCEPTING\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
            
        case AGENT_STATE_ATTACKING:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //reset all data
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                agentData.state = AGENT_STATE_START;
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
            }
            else if(event.type == BB_EVENT_RES_RECEIVED)
            {
                //update record of enemy field
                agentData.own_guessData.row = event.param0;
                agentData.own_guessData.col = event.param1;
                agentData.own_guessData.result = event.param2;
//                printf("%d", agentData.own_guessData.result);
                FieldUpdateKnowledge(&agentData.opp_field, &agentData.own_guessData);
                if(FieldGetBoatStates(&agentData.opp_field) == 0x0)
                {
                    OledClear(0);
                    OledDrawString("\n  YOU WIN!\n\nPress BTN1 to restart");
                    OledUpdate();
                    agentData.state = AGENT_STATE_END_SCREEN;
                    agentData.message.type = MESSAGE_NONE;
                    agentData.message.param0 = 0;
                    agentData.message.param1 = 0;
                    agentData.message.param2 = 0;
                    agentData.state = AGENT_STATE_END_SCREEN;
                    return agentData.message;
                }
                else
                {
                    agentData.state = AGENT_STATE_DEFENDING;
                }            
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                FieldOledDrawScreen(&agentData.own_field, &agentData.opp_field, agentData.turn, 
                                    agentData.turnCount);
                agentData.turn = FIELD_OLED_TURN_THEIRS;
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string, "BB_EVENT_ERROR in the\nAGENT_STATE_ATTACKING\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
            
        case AGENT_STATE_DEFENDING:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //reset all data
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                agentData.state = AGENT_STATE_START;
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
            }
            else if(event.type == BB_EVENT_SHO_RECEIVED)
            {
                //update own field
                agentData.opp_guessData.row = event.param0;
                agentData.opp_guessData.col = event.param1;
                agentData.opp_guessData.result = 0;
                
                FieldRegisterEnemyAttack(&agentData.own_field, &agentData.opp_guessData);
                //send res
                if(FieldGetBoatStates(&agentData.own_field) == 0x0)
                {
                    OledClear(0);
                    OledDrawString("\n  YOU LOSE!\n\nPress BTN1 to restart");
                    OledUpdate();
                    agentData.message.type = MESSAGE_NONE;
                    agentData.message.param0 = 0;
                    agentData.message.param1 = 0;
                    agentData.message.param2 = 0;
                    agentData.state = AGENT_STATE_END_SCREEN;
                    return agentData.message;
                }
                else
                {
                    agentData.state = AGENT_STATE_WAITING_TO_SEND;
                }
                //send RES
                agentData.message.type = MESSAGE_RES;
                agentData.message.param0  = agentData.opp_guessData.row;
                agentData.message.param1  = agentData.opp_guessData.col;
                agentData.message.param2  = agentData.opp_guessData.result;
                FieldOledDrawScreen(&agentData.own_field, &agentData.opp_field, agentData.turn, 
                                    agentData.turnCount);
                agentData.turn = FIELD_OLED_TURN_MINE;
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string, "BB_EVENT_ERROR in the\nAGENT_STATE_DEFENDING\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
            
        case AGENT_STATE_WAITING_TO_SEND:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //reset all data
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                agentData.state = AGENT_STATE_START;
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
            }
            else if(event.type == BB_EVENT_MESSAGE_SENT)
            {
                agentData.turnCount ++;
                //decide guess
                agentData.own_guessData = FieldAIDecideGuess(&agentData.opp_field);
                //send SHO
                agentData.state = AGENT_STATE_ATTACKING;
                agentData.message.type = MESSAGE_SHO;
                agentData.message.param0  = agentData.own_guessData.row;
                agentData.message.param1  = agentData.own_guessData.col;
                agentData.message.param2  = 0;
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string,"BB_EVENT_ERROR in the\nAGENT_STATE_WAITING_TO_SEND\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
            
        case AGENT_STATE_END_SCREEN:
            if(event.type == BB_EVENT_RESET_BUTTON)
            {
                //reset all data
                //display newgame message
                OledClear(0);
                OledDrawString("START\n\nReady for a new game?\nPress BTN4 to");
                OledUpdate();
                agentData.state = AGENT_STATE_START;
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
                agentData.turnCount = 0;
                agentData.A = 0;
                agentData.B = 0;
                agentData.hashA = 0;
            }
            else if(event.type == BB_EVENT_ERROR)
            {
                //display appropriate error message
                OledClear(0);
                sprintf(oled_string, "BB_EVENT_ERROR in the\nAGENT_STATE_END_SCREEN\nstate :%d.", 
                        (int)event.param0);
                OledDrawString(oled_string);
                OledUpdate();
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            else
            {
                agentData.message.type = MESSAGE_NONE;
                agentData.message.param0  = 0;
                agentData.message.param1  = 0;
                agentData.message.param2  = 0;
            }
            break;
    }
    return agentData.message;
}

/** * 
 * @return Returns the current state that AgentGetState is in.  
 * 
 * This function is very useful for testing AgentRun.
 */
AgentState AgentGetState(void)
{
    return agentData.state;
}

/** * 
 * @param Force the agent into the state given by AgentState
 * 
 * This function is very useful for testing AgentRun.
 */
void AgentSetState(AgentState newState)
{
    agentData.state = newState;
}