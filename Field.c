
#include "Field.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

//CMPE13 Support Library
#include "BOARD.h"


//CE13 standard libraries:
#include "Uart1.h"

#define MAX_AI_BOAT_TRIES 100


static uint8_t typeToNumSquares(BoatType b);
static int typeToTypeSquare(BoatType b);
static uint8_t isBoat(uint8_t loc);

static uint8_t guesses[FIELD_ROWS][FIELD_COLS];

/**
 * This function is optional, but recommended.   It prints a representation of both
 * fields, similar to the OLED display.
 * @param f The field to initialize.
 * @param p The data to initialize the entire field to, should be a member of enum
 *                     SquareStatus.
 */
void FieldPrint_UART(Field *own_field, Field * opp_field) {
    int i, j;
    for (i = 0; i < FIELD_ROWS; i++) {
        for (j = 0; j < FIELD_COLS; j++) {
            printf("%d ", own_field->grid[i][j]);
        }
        printf("\n");
    }

    for (i = 0; i < FIELD_ROWS; i++) {
        for (j = 0; j < FIELD_COLS; j++) {
            printf("%d ", opp_field->grid[i][j]);
        }
        printf("\n");
    }
}

/**
 * FieldInit() will initialize two passed field structs for the beginning of play.
 * Each field's grid should be filled with the appropriate SquareStatus (
 * FIELD_SQUARE_EMPTY for your own field, FIELD_SQUARE_UNKNOWN for opponent's).
 * Additionally, your opponent's field's boatLives parameters should be filled
 *  (your own field's boatLives will be filled when boats are added)
 * 
 * FieldAI_PlaceAllBoats() should NOT be called in this function.
 * 
 * @param own_field     //A field representing the agents own ships
 * @param opp_field     //A field representing the opponent's ships
 */
void FieldInit(Field *own_field, Field * opp_field) {

    int i, j;
    for (i = 0; i < FIELD_ROWS; i++) {
        for (j = 0; j < FIELD_COLS; j++) {
            guesses[i][j] = 0;
        }
    }

    int myRow, myCol, oppRow, oppCol;

    for (myRow = 0; myRow < FIELD_ROWS; myRow++) {
        for (myCol = 0; myCol < FIELD_COLS; myCol++) {
            own_field->grid[myRow][myCol] = FIELD_SQUARE_EMPTY;
        }
    }

    for (oppRow = 0; oppRow < FIELD_ROWS; oppRow++) {
        for (oppCol = 0; oppCol < FIELD_COLS; oppCol++) {
            opp_field->grid[oppRow][oppCol] = FIELD_SQUARE_UNKNOWN;
        }
    }

    opp_field->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
    opp_field->largeBoatLives = FIELD_BOAT_SIZE_LARGE;
    opp_field->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;
    opp_field->smallBoatLives = FIELD_BOAT_SIZE_SMALL;
}

/**
 * Retrieves the value at the specified field position.
 * @param f     //The Field being referenced
 * @param row   //The row-component of the location to retrieve
 * @param col   //The column-component of the location to retrieve
 * @return  FIELD_SQUARE_INVALID if row and col are not valid field locations
 *          Otherwise, return the status of the referenced square 
 */
SquareStatus FieldGetSquareStatus(const Field *f, uint8_t row, uint8_t col) {
    if (row > FIELD_ROWS - 1 || row < 0 || col > FIELD_COLS - 1 || col < 0) {
        return FIELD_SQUARE_INVALID;
    }
    return f->grid[row][col];
}

/**
 * This function provides an interface for setting individual locations within a Field struct. This
 * is useful when FieldAddBoat() doesn't do exactly what you need. For example, if you'd like to use
 * FIELD_SQUARE_CURSOR, this is the function to use.
 * 
 * @param f The Field to modify.
 * @param row The row-component of the location to modify
 * @param col The column-component of the location to modify
 * @param p The new value of the field location
 * @return The old value at that field location
 */
SquareStatus FieldSetSquareStatus(Field *f, uint8_t row, uint8_t col, SquareStatus p) {
    if (row > FIELD_ROWS - 1 || row < 0 || col > FIELD_COLS - 1 || col < 0) {
        return FIELD_SQUARE_INVALID;
    }
    SquareStatus old = f->grid[row][col];
    f->grid[row][col] = p;
    return old;
}

/**
 * FieldAddBoat() places a single ship on the player's field based on arguments 2-5. Arguments 2, 3
 * represent the x, y coordinates of the pivot point of the ship.  Argument 4 represents the
 * direction of the ship, and argument 5 is the length of the ship being placed. 
 * 
 * All spaces that
 * the boat would occupy are checked to be clear before the field is modified so that if the boat
 * can fit in the desired position, the field is modified as SUCCESS is returned. Otherwise the
 * field is unmodified and STANDARD_ERROR is returned. There is no hard-coded limit to how many
 * times a boat can be added to a field within this function.
 * 
 * In addition, this function should update the appropriate boatLives parameter of the field.
 *
 * So this is valid test code:
 * {
 *   Field myField;
 *   FieldInit(&myField,FIELD_SQUARE_EMPTY);
 *   FieldAddBoat(&myField, 0, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_SMALL);
 *   FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_MEDIUM);
 *   FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_HUGE);
 *   FieldAddBoat(&myField, 0, 6, FIELD_BOAT_DIRECTION_SOUTH, FIELD_BOAT_TYPE_SMALL);
 * }
 *
 * should result in a field like:
 *      0 1 2 3 4 5 6 7 8 9
 *     ---------------------
 *  0 [ 3 3 3 . . . 3 . . . ]
 *  1 [ 4 4 4 4 . . 3 . . . ]
 *  2 [ . . . . . . 3 . . . ]
 *  3 [ . . . . . . . . . . ]
 *  4 [ . . . . . . . . . . ]
 *  5 [ . . . . . . . . . . ]
 *     
 * @param f The field to grab data from.
 * @param row The row that the boat will start from, valid range is from 0 and to FIELD_ROWS - 1.
 * @param col The column that the boat will start from, valid range is from 0 and to FIELD_COLS - 1.
 * @param dir The direction that the boat will face once places, from the BoatDirection enum.
 * @param boatType The type of boat to place. Relies on the FIELD_SQUARE_*_BOAT values from the
 * SquareStatus enum.
 * @return SUCCESS for success, STANDARD_ERROR for failure
 */
uint8_t FieldAddBoat(Field *own_field, uint8_t row, uint8_t col, BoatDirection dir, BoatType boat_type) {
    if (row > FIELD_ROWS - 1 || row < 0 || col > FIELD_COLS - 1 || col < 0) {
        return STANDARD_ERROR;
    }

    int length = typeToNumSquares(boat_type);
    int typeBoat = typeToTypeSquare(boat_type);
    if (dir == FIELD_DIR_SOUTH) {
        if (row + (length - 1) > FIELD_ROWS - 1) {
            return STANDARD_ERROR;
        }

        int o;
        for (o = row; o < row + length; o++) {
            if (own_field->grid[o][col] > FIELD_SQUARE_EMPTY && own_field->grid[o][col] < FIELD_SQUARE_HUGE_BOAT + 1) {
                return STANDARD_ERROR;
            }
        }

        int i;
        for (i = row; i < row + length; i++) {
            own_field->grid[i][col] = typeBoat;
        }
    } else if (dir == FIELD_DIR_EAST) {

        if (col + (length - 1) > FIELD_COLS - 1) {
            return STANDARD_ERROR;
        }

        int k;
        for (k = col; k < col + length; k++) {
            if (own_field->grid[row][k] > FIELD_SQUARE_EMPTY && own_field->grid[row][k] < FIELD_SQUARE_HUGE_BOAT + 1) {
                return STANDARD_ERROR;
            }
        }

        int j;
        for (j = col; j < col + length; j++) {
            own_field->grid[row][j] = typeBoat;
        }
    } else {
        return STANDARD_ERROR;
    }

    if (boat_type == FIELD_BOAT_TYPE_SMALL) {
        own_field->smallBoatLives = FIELD_BOAT_SIZE_SMALL;
    } else if (boat_type == FIELD_BOAT_TYPE_MEDIUM) {
        own_field->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;
    } else if (boat_type == FIELD_BOAT_TYPE_LARGE) {
        own_field->largeBoatLives = FIELD_BOAT_SIZE_LARGE;
    } else {
        own_field->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
    }
    return SUCCESS;
}

/**
 * This function registers an attack at the gData coordinates on the provided field. This means that
 * 'f' is updated with a FIELD_SQUARE_HIT or FIELD_SQUARE_MISS depending on what was at the
 * coordinates indicated in 'gData'. 'gData' is also updated with the proper HitStatus value
 * depending on what happened AND the value of that field position BEFORE it was attacked. Finally
 * this function also reduces the lives for any boat that was hit from this attack.
 * @param f The field to check against and update.
 * @param gData The coordinates that were guessed. The result is stored in gData->result as an
 *               output.  The result can be a RESULT_HIT, RESULT_MISS, or RESULT_***_SUNK.
 * @return The data that was stored at the field position indicated by gData before this attack.
 */
SquareStatus FieldRegisterEnemyAttack(Field *own_field, GuessData *opp_guess) {
    uint8_t fieldLoc = own_field->grid[opp_guess->row][opp_guess->col];
    if (isBoat(fieldLoc) == FALSE) {
        own_field->grid[opp_guess->row][opp_guess->col] = FIELD_SQUARE_MISS;
        opp_guess->result = RESULT_MISS;
    } else {
        if (fieldLoc == FIELD_SQUARE_SMALL_BOAT) {
            own_field->smallBoatLives--;
            if (own_field->smallBoatLives == 0) {
                opp_guess->result = RESULT_SMALL_BOAT_SUNK;
            } else {
                opp_guess->result = RESULT_HIT;
            }
        } else if (fieldLoc == FIELD_SQUARE_MEDIUM_BOAT) {
            own_field->mediumBoatLives--;
            if (own_field->mediumBoatLives == 0) {
                opp_guess->result = RESULT_MEDIUM_BOAT_SUNK;
            } else {
                opp_guess->result = RESULT_HIT;
            }
        } else if (fieldLoc == FIELD_SQUARE_LARGE_BOAT) {
            own_field->largeBoatLives--;
            if (own_field->largeBoatLives == 0) {
                opp_guess->result = RESULT_LARGE_BOAT_SUNK;
            } else {
                opp_guess->result = RESULT_HIT;
            }
        } else if (fieldLoc == FIELD_SQUARE_HUGE_BOAT) {
            own_field->hugeBoatLives--;
            if (own_field->hugeBoatLives == 0) {
                opp_guess->result = RESULT_HUGE_BOAT_SUNK;
            } else {
                opp_guess->result = RESULT_HIT;
            }
        }
        own_field->grid[opp_guess->row][opp_guess->col] = FIELD_SQUARE_HIT;
    }
    return fieldLoc;
}

/**
 * This function updates the FieldState representing the opponent's game board with whether the
 * guess indicated within gData was a hit or not. If it was a hit, then the field is updated with a
 * FIELD_SQUARE_HIT at that position. If it was a miss, display a FIELD_SQUARE_EMPTY instead, as
 * it is now known that there was no boat there. The FieldState struct also contains data on how
 * many lives each ship has. Each hit only reports if it was a hit on any boat or if a specific boat
 * was sunk, this function also clears a boats lives if it detects that the hit was a
 * RESULT_*_BOAT_SUNK.
 * @param f The field to grab data from.
 * @param gData The coordinates that were guessed along with their HitStatus.
 * @return The previous value of that coordinate position in the field before the hit/miss was
 * registered.
 */
SquareStatus FieldUpdateKnowledge(Field *opp_field, const GuessData *own_guess) {
    uint8_t oppLoc = opp_field->grid[own_guess->row][own_guess->col];
    if (own_guess->result > RESULT_MISS) {
        if (own_guess->result == RESULT_SMALL_BOAT_SUNK) {
            opp_field->smallBoatLives = 0;
        }
        if (own_guess->result == RESULT_MEDIUM_BOAT_SUNK) {
            opp_field->mediumBoatLives = 0;
        }
        if (own_guess->result == RESULT_LARGE_BOAT_SUNK) {
            opp_field->largeBoatLives = 0;
        }
        if (own_guess->result == RESULT_HUGE_BOAT_SUNK) {
            opp_field->hugeBoatLives = 0;
        }
        opp_field->grid[own_guess->row][own_guess->col] = FIELD_SQUARE_HIT;
    } else {
        opp_field->grid[own_guess->row][own_guess->col] = FIELD_SQUARE_EMPTY;
    }
    return oppLoc;
}

/**
 * This function returns the alive states of all 4 boats as a 4-bit bitfield (stored as a uint8).
 * The boats are ordered from smallest to largest starting at the least-significant bit. So that:
 * 0b00001010 indicates that the small boat and large boat are sunk, while the medium and huge boat
 * are still alive. See the BoatStatus enum for the bit arrangement.
 * @param f The field to grab data from.
 * @return A 4-bit value with each bit corresponding to whether each ship is alive or not.
 */
uint8_t FieldGetBoatStates(const Field *f) {
    uint8_t boatState = 0;
    if (f->smallBoatLives > 0) {
        boatState |= FIELD_BOAT_STATUS_SMALL;
    }

    if (f->mediumBoatLives > 0) {
        boatState |= FIELD_BOAT_STATUS_MEDIUM;
    }

    if (f->largeBoatLives > 0) {
        boatState |= FIELD_BOAT_STATUS_LARGE;
    }

    if (f->hugeBoatLives > 0) {
        boatState |= FIELD_BOAT_STATUS_HUGE;
    }

    return boatState;
}

/**
 * This function is responsible for placing all four of the boats on a field.
 * 
 * @param f         //agent's own field, to be modified in place.
 * @return SUCCESS if all boats could be placed, STANDARD_ERROR otherwise.
 * 
 * This function should never fail when passed a properly initialized field!
 */
uint8_t FieldAIPlaceAllBoats(Field *own_field) {
    uint8_t success = STANDARD_ERROR;
    BoatType b = FIELD_BOAT_TYPE_HUGE;
    BoatDirection dir, oldDir, notdir;
    uint8_t randRow = 0, randCol = 0;

    int numTries = 0;
    dir = rand() & 0x01;
    notdir = dir;
    while (success == STANDARD_ERROR) { //

        randRow = rand() % FIELD_ROWS;
        randCol = rand() % FIELD_COLS;
        if (dir == 1) {
            dir = 0;
        } else {
            dir++;
        }

        success = FieldAddBoat(own_field, randRow, randCol, dir, b);

        numTries++;
        if (numTries > MAX_AI_BOAT_TRIES) {
            return STANDARD_ERROR;
        }
    }
    own_field->smallBoatLives = FIELD_BOAT_SIZE_SMALL;

    oldDir = dir;
    success = STANDARD_ERROR;
    b--;
    numTries = 0;
    dir = !oldDir;
    while (success == STANDARD_ERROR) { //MEDIUM
        randRow = rand() % FIELD_ROWS;
        randCol = rand() % FIELD_COLS;

        success = FieldAddBoat(own_field, randRow, randCol, dir, b);

        numTries++;
        if (numTries > MAX_AI_BOAT_TRIES) {
            return STANDARD_ERROR;
        }
    }

    own_field->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;

    oldDir = dir;
    success = STANDARD_ERROR;
    b--;
    numTries = 0;
    dir = !notdir;
    while (success == STANDARD_ERROR) { //LARGE
        randRow = rand() % FIELD_ROWS;
        randCol = rand() % FIELD_COLS;
        if (dir == 1) {
            dir = 0;
        } else {
            dir++;
        }

        success = FieldAddBoat(own_field, randRow, randCol, dir, b);

        numTries++;
        if (numTries > MAX_AI_BOAT_TRIES) {
            return STANDARD_ERROR;
        }
    }

    own_field->largeBoatLives = FIELD_BOAT_SIZE_LARGE;

    oldDir = dir;
    success = STANDARD_ERROR;
    b--;
    numTries = 0;
    dir = !oldDir;
    while (success == STANDARD_ERROR) { //HUGE
        randRow = rand() % FIELD_ROWS;
        randCol = rand() % FIELD_COLS;

        success = FieldAddBoat(own_field, randRow, randCol, dir, b);

        numTries++;
        if (numTries > MAX_AI_BOAT_TRIES) {
            return STANDARD_ERROR;
        }
    }

    own_field->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
    return SUCCESS;
}

/**
 * Given a field, decide the next guess.
 *
 * This function should not attempt to shoot a square which has already been guessed.
 *
 * You may wish to give this function static variables.  If so, that data should be
 * reset when FieldInit() is called.
 * 
 * @param f an opponent's field.
 * @return a GuessData struct whose row and col parameters are the coordinates of the guess.  The 
 *           result parameter is irrelevant.
 */
GuessData FieldAIDecideGuess(const Field *opp_field) {
    GuessData g;
    uint8_t randRow = 0, randCol = 0;
    uint8_t open = FALSE;

    while (open == FALSE) {
        randRow = rand() % FIELD_ROWS;
        randCol = rand() % FIELD_COLS;
        if (opp_field->grid[randRow][randCol] != FIELD_SQUARE_HIT && guesses[randRow][randCol] != 1) {
            g.row = randRow;
            g.col = randCol;
            open = TRUE;
            guesses[randRow][randCol] = 1;
        }
    }
    return g;
}

static uint8_t typeToNumSquares(BoatType b) {
    uint8_t num = 0;

    if (b == FIELD_BOAT_TYPE_SMALL) {
        num = FIELD_BOAT_SIZE_SMALL;
    } else if (b == FIELD_BOAT_TYPE_MEDIUM) {
        num = FIELD_BOAT_SIZE_MEDIUM;
    } else if (b == FIELD_BOAT_TYPE_LARGE) {
        num = FIELD_BOAT_SIZE_LARGE;
    } else {
        num = FIELD_BOAT_SIZE_HUGE;
    }
    return num;
}

static int typeToTypeSquare(BoatType b) {
    int status = FIELD_SQUARE_EMPTY;

    if (b == FIELD_BOAT_TYPE_SMALL) {
        status = FIELD_SQUARE_SMALL_BOAT;
    } else if (b == FIELD_BOAT_TYPE_MEDIUM) {
        status = FIELD_SQUARE_MEDIUM_BOAT;
    } else if (b == FIELD_BOAT_TYPE_LARGE) {
        status = FIELD_SQUARE_LARGE_BOAT;
    } else {
        status = FIELD_SQUARE_HUGE_BOAT;
    }
    return status;
}

static uint8_t isBoat(uint8_t loc) {
    if (loc == FIELD_SQUARE_SMALL_BOAT || loc == FIELD_SQUARE_MEDIUM_BOAT || loc == FIELD_SQUARE_LARGE_BOAT || loc == FIELD_SQUARE_HUGE_BOAT) {
        return TRUE;
    } else {
        return FALSE;
    }
}