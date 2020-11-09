#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// board information
#define BOARD_SIZE 8
#define EMPTY 0
#define MY_FLAG 2
#define MY_KING 4
#define ENEMY_FLAG 1
#define ENEMY_KING 3

// bool
typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define MAX_STEP 15
#define MAX_WAYS 50
#define MAX_DEPTH 120

#define START "START"
#define PLACE "PLACE"
#define TURN "TURN"
#define END "END"

#define POSI_INFI 2147483647
#define NEGA_INFI -2147483647

#define SCORE_PAWN 100
#define SCORE_KING 200

//value of chess formation
#define COLUMN 1

#define BREAK_TIME 1400

typedef struct tagStep
{
	int x[MAX_STEP];
	int y[MAX_STEP];
	int numStep;
}Step;
typedef struct tagRoute
{
	int numMove;
	Step step[MAX_DEPTH];
}Route;

char board[BOARD_SIZE][BOARD_SIZE] = { 0 };
int myFlag;
int searchDepth;

DWORD startTime;
int timeUp = FALSE;

Route firstRoute;

//value of position
char myPawnScore[BOARD_SIZE][BOARD_SIZE] = {
	{0,1,0,9,0,1,0,1},
	{1,0,3,0,3,0,3,0},
	{0,4,0,6,0,6,0,3},
	{6,0,7,0,7,0,5,0},
	{0,8,0,8,0,8,0,9},
	{7,0,8,0,8,0,8,0},
	{0,8,0,8,0,8,0,7},
	{0,0,0,0,0,0,0,0}
};
char myKingScore[BOARD_SIZE][BOARD_SIZE] = {
	{0,10,0,10,0,10,0,10},
	{20,0,20,0,20,0,20,0},
	{0,30,0,30,0,30,0,30},
	{40,0,40,0,40,0,40,0},
	{0,40,0,40,0,40,0,40},
	{30,0,30,0,30,0,30,0},
	{0,20,0,20,0,20,0,20},
	{0,0,0,0,0,0,0,0}
};
char otherPawnScore[BOARD_SIZE][BOARD_SIZE];
char otherKingScore[BOARD_SIZE][BOARD_SIZE];

BOOL isInBound(int x, int y)
{
	return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

void rotateCommand(Step* cmd)
{
	if (myFlag == ENEMY_FLAG)
	{
		for (int i = 0; i < cmd->numStep; i++)
		{
			cmd->x[i] = BOARD_SIZE - 1 - cmd->x[i];
			cmd->y[i] = BOARD_SIZE - 1 - cmd->y[i];
		}
	}
}

void tryToMove(int x, int y, const char boardMove[BOARD_SIZE][BOARD_SIZE], Step* pMove, int* pLegalMoveNum)
{
	int newX, newY;
	int moveDirMy[4][2] = { {1, -1}, {1, 1}, {-1, -1}, {-1, 1} };
	int moveDirEnemy[4][2] = { {-1, -1}, {-1, 1}, {1, -1}, {1, 1} };
	int moveDir[4][2];
	if (boardMove[x][y] == ENEMY_FLAG)
	{
		memcpy(moveDir, moveDirEnemy, sizeof(moveDir));
	}
	else
	{
		memcpy(moveDir, moveDirMy, sizeof(moveDir));
	}
	for (int i = 0; i < (boardMove[x][y] + 1) / 2 + 1; i++)
	{
		newX = x + moveDir[i][0];
		newY = y + moveDir[i][1];
		if (isInBound(newX, newY) && boardMove[newX][newY] == EMPTY)
		{
			(pMove + *pLegalMoveNum)->numStep = 2;
			(pMove + *pLegalMoveNum)->x[0] = x;
			(pMove + *pLegalMoveNum)->y[0] = y;
			(pMove + *pLegalMoveNum)->x[1] = newX;
			(pMove + *pLegalMoveNum)->y[1] = newY;
			(*pLegalMoveNum)++;
		}
	}
}

void tryToJump(int x, int y, int currentStep, char boardJump[BOARD_SIZE][BOARD_SIZE], Step* pLongestJump, Step* pJump, int* pLegalJumpNum, int chess)
{
	int jumpDir[4][2] = { {2, -2}, {2, 2}, {-2, -2}, {-2, 2} };
	int newX, newY, midX, midY;
	char tmpFlag;
	pJump->x[currentStep] = x;
	pJump->y[currentStep] = y;
	pJump->numStep++;
	for (int i = 0; i < 4; i++)
	{
		newX = x + jumpDir[i][0];
		newY = y + jumpDir[i][1];
		midX = (x + newX) / 2;
		midY = (y + newY) / 2;
		if (isInBound(newX, newY) && (boardJump[midX][midY] == 3 - chess || boardJump[midX][midY] == 5 - chess) && (boardJump[newX][newY] == EMPTY))
		{
			boardJump[newX][newY] = boardJump[x][y];
			boardJump[x][y] = EMPTY;
			tmpFlag = boardJump[midX][midY];
			boardJump[midX][midY] = EMPTY;
			tryToJump(newX, newY, currentStep + 1, boardJump, pLongestJump, pJump, pLegalJumpNum, chess);
			boardJump[x][y] = boardJump[newX][newY];
			boardJump[newX][newY] = EMPTY;
			boardJump[midX][midY] = tmpFlag;
		}
	}
	if (pJump->numStep > pLongestJump->numStep)
	{
		*pLegalJumpNum = 0;
		memset(pLongestJump, 0, sizeof(Step) * MAX_WAYS);
		memcpy(pLongestJump + *pLegalJumpNum, pJump, sizeof(Step));
		(*pLegalJumpNum)++;
	}
	else if (pJump->numStep == pLongestJump->numStep && pJump->numStep != 1)
	{
		memcpy(pLongestJump + *pLegalJumpNum, pJump, sizeof(Step));
		(*pLegalJumpNum)++;
	}
	pJump->x[currentStep] = 0;
	pJump->y[currentStep] = 0;
	pJump->numStep--;
}

void place(Step cmd)
{
	int midX, midY, curFlag;
	curFlag = board[cmd.x[0]][cmd.y[0]];

	//place the chess
	for (int i = 0; i < cmd.numStep - 1; i++)
	{
		board[cmd.x[i]][cmd.y[i]] = EMPTY;
		board[cmd.x[i + 1]][cmd.y[i + 1]] = curFlag;
		if (abs(cmd.x[i] - cmd.x[i + 1]) == 2)
		{
			midX = (cmd.x[i] + cmd.x[i + 1]) / 2;
			midY = (cmd.y[i] + cmd.y[i + 1]) / 2;
			board[midX][midY] = EMPTY;
		}
	}

	//pawn become king
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (board[0][i] == ENEMY_FLAG)
		{
			board[0][i] = ENEMY_KING;
		}
		if (board[BOARD_SIZE - 1][i] == MY_FLAG)
		{
			board[BOARD_SIZE - 1][i] = MY_KING;
		}
	}
}

void initAI(int me)
{
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int y = 0; y < BOARD_SIZE; y++)
		{
			otherPawnScore[x][y] = myPawnScore[BOARD_SIZE - 1 - x][BOARD_SIZE - 1 - y];
			otherKingScore[x][y] = myKingScore[BOARD_SIZE - 1 - x][BOARD_SIZE - 1 - y];
		}
	}
}

int columnScore(int x, int y, const char boardColumn[BOARD_SIZE][BOARD_SIZE], int chess)
{
	int score = 0;
	if (boardColumn[x - 1][y + 1] == chess && boardColumn[x + 1][y - 1] == chess &&
		boardColumn[x + 1][y + 1] == EMPTY && boardColumn[x - 1][x + 1] == EMPTY)
	{
		score += COLUMN;
	}
	else if (boardColumn[x + 1][y + 1] == chess && boardColumn[x - 1][x + 1] == chess &&
		boardColumn[x - 1][y + 1] == EMPTY && boardColumn[x + 1][y - 1] == EMPTY)
	{
		score += COLUMN;
	}
	return score;
}

int evaluate(const char boardEvaluate[BOARD_SIZE][BOARD_SIZE])
{
	DWORD curTime;
	int meScore = 0;
	int otherScore = 0;
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int y = 0; y < BOARD_SIZE; y++)
		{
			switch (boardEvaluate[x][y])
			{
			case ENEMY_FLAG:
				otherScore = otherScore + SCORE_PAWN + otherPawnScore[x][y] + columnScore(x, y, boardEvaluate, ENEMY_FLAG);
				break;
			case MY_FLAG:
				meScore = meScore + SCORE_PAWN + myPawnScore[x][y] + columnScore(x, y, boardEvaluate, MY_FLAG);
				break;
			case ENEMY_KING:
				otherScore = otherScore + SCORE_KING + otherKingScore[x][y];
				break;
			case MY_KING:
				meScore = meScore + SCORE_KING + myKingScore[x][y];
				break;
			default:
				break;
			}
		}
	}

	//if time is up, immediately break
	curTime = GetTickCount();
	if (curTime - startTime > BREAK_TIME)
	{
		timeUp = TRUE;
	}
	return meScore - otherScore;
}

void sortStep(Step* pStep, int depth, int legalStepNum)
{
	BOOL same = FALSE;
	Step temp;
	for (int i = 0; i < legalStepNum; i++)
	{
		if ((pStep + i)->numStep == firstRoute.step[searchDepth - depth].numStep)
		{
			same = TRUE;
			for (int j = 0; j < (pStep + i)->numStep; j++)
			{
				if ((pStep + i)->x[j] != firstRoute.step[searchDepth - depth].x[j] ||
					(pStep + i)->y[j] != firstRoute.step[searchDepth - depth].y[j])
				{
					same = FALSE;
					break;
				}
			}
			if (same == TRUE)
			{
				temp = *pStep;
				*pStep = *(pStep + i);
				*(pStep + i) = temp;
				break;
			}
		}
	}
}

void findStep(const char boardStep[BOARD_SIZE][BOARD_SIZE], Step* pStep, int* pLegalJumpNum, int* pLegalMoveNum, int chess)
{
	Step stepTemp;
	memset(&stepTemp, 0, sizeof(stepTemp));
	pStep->numStep = 1;
	char boardFind[BOARD_SIZE][BOARD_SIZE];
	memcpy(boardFind, boardStep, sizeof(boardFind));

	//try to jump first
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int y = 0; y < BOARD_SIZE; y++)
		{
			if (boardFind[x][y] > 0 && (boardFind[x][y] == chess || boardFind[x][y] == chess + 2)) {
				tryToJump(x, y, 0, boardFind, pStep, &stepTemp, pLegalJumpNum, chess);
			}
		}
	}

	//if there is not a jump, move
	if (*pLegalJumpNum == 0)
	{
		for (int x = 0; x < BOARD_SIZE; x++)
		{
			for (int y = 0; y < BOARD_SIZE; y++)
			{
				if (boardFind[x][y] > 0 && (boardFind[x][y] == chess || boardFind[x][y] == chess + 2))
				{
					tryToMove(x, y, boardFind, pStep, pLegalMoveNum);
				}
			}
		}
	}
}

void makeJump(char boardMakeJump[BOARD_SIZE][BOARD_SIZE], Step* pMakeJump)
{
	int midX, midY, startX, startY, endX, endY;
	startX = pMakeJump->x[0];
	startY = pMakeJump->y[0];
	endX = pMakeJump->x[pMakeJump->numStep - 1];
	endY = pMakeJump->y[pMakeJump->numStep - 1];
	boardMakeJump[endX][endY] = boardMakeJump[startX][startY];
	boardMakeJump[startX][startY] = EMPTY;
	for (int i = 0; i < pMakeJump->numStep - 1; i++)
	{
		midX = (pMakeJump->x[i] + pMakeJump->x[i + 1]) / 2;
		midY = (pMakeJump->y[i] + pMakeJump->y[i + 1]) / 2;
		boardMakeJump[midX][midY] = EMPTY;
	}
	if (boardMakeJump[endX][endY] == MY_FLAG && endX == BOARD_SIZE - 1)
	{
		boardMakeJump[endX][endY] = MY_KING;
	}
	else if (boardMakeJump[endX][endY] == ENEMY_FLAG && endX == 0)
	{
		boardMakeJump[endX][endY] = ENEMY_KING;
	}
}

void makeMove(char boardMakeMove[BOARD_SIZE][BOARD_SIZE], Step* pMakeMove)
{
	int startX, startY, endX, endY;
	startX = pMakeMove->x[0];
	startY = pMakeMove->y[0];
	endX = pMakeMove->x[1];
	endY = pMakeMove->y[1];
	boardMakeMove[endX][endY] = boardMakeMove[startX][startY];
	boardMakeMove[startX][startY] = EMPTY;
	if (boardMakeMove[endX][endY] == MY_FLAG && endX == BOARD_SIZE - 1)
	{
		boardMakeMove[endX][endY] = MY_KING;
	}
	else if (boardMakeMove[endX][endY] == ENEMY_FLAG && endX == 0)
	{
		boardMakeMove[endX][endY] = ENEMY_KING;
	}
}

void makeStep(char boardStep[BOARD_SIZE][BOARD_SIZE], int legalJumpNum, Step* pMakeStep)
{
	if (legalJumpNum != 0)
	{
		makeJump(boardStep, pMakeStep);
	}
	else
	{
		makeMove(boardStep, pMakeStep);
	}
}

int negaMax(int depth, int alpha, int beta, const char boardLast[BOARD_SIZE][BOARD_SIZE], Route* pRoute, int chess)
{
	Route route;
	Step stepFind[MAX_WAYS];
	int legalJumpNum = 0, legalMoveNum = 0, legalStepNum = 0;
	int score;
	char boardNext[BOARD_SIZE][BOARD_SIZE];
	memcpy(boardNext, boardLast, sizeof(boardNext));

	//if we reach the bottom
	if (depth == 0)
	{
		pRoute->numMove = 0;
		return evaluate((const char(*)[BOARD_SIZE])boardNext);
	}

	//find a way
	findStep((const char(*)[BOARD_SIZE])boardNext, stepFind, &legalJumpNum, &legalMoveNum, chess);
	legalStepNum = legalJumpNum > legalMoveNum ? legalJumpNum : legalMoveNum;

	//if there is a way
	if (legalStepNum != 0)
	{

		//try firstRoute first
		sortStep(stepFind, depth, legalStepNum);

		//search the next depth
		for (int i = 0; i < legalStepNum; i++)
		{
			memcpy(boardNext, boardLast, sizeof(boardNext));
			makeStep(boardNext, legalJumpNum, stepFind + i);
			score = -negaMax(depth - 1, -beta, -alpha, (const char(*)[BOARD_SIZE])boardNext, &route, 3 - chess);

			//if time is up, immediately break
			if (timeUp == TRUE)
			{
				return 0;
			}

			//negaMax
			if (score >= beta)
			{
				return beta;
			}
			if (score > alpha)
			{
				alpha = score;

				//Generate the firstRoute
				pRoute->step[0] = stepFind[i];
				memcpy(pRoute->step + 1, route.step, route.numMove * sizeof(Step));
				pRoute->numMove = route.numMove + 1;
			}
		}
		return alpha;
	}

	//if there is no way to go, you lose
	else
	{
		return NEGA_INFI + 1;
	}
}

void iterativeDeepening(const char boardIterDeep[BOARD_SIZE][BOARD_SIZE])
{
	int curScore;
	Route tempFirstRoute;

	//gradually increase the depth you search
	for (searchDepth = 2; searchDepth <= MAX_DEPTH; searchDepth += 2)
	{
		memset(&tempFirstRoute, 0, sizeof(Route));
		curScore = negaMax(searchDepth, NEGA_INFI, POSI_INFI, boardIterDeep, &tempFirstRoute, MY_FLAG);
		if (timeUp == TRUE)
		{
			break;
		}
		memcpy(&firstRoute, &tempFirstRoute, sizeof(Route));
		if (curScore == POSI_INFI - 1)
		{
			break;
		}
	}
}

Step aiTurn(const char boardTurn[BOARD_SIZE][BOARD_SIZE], int me)
{
	startTime = GetTickCount();
	Step command =
	{
		.x = {0},
		.y = {0},
		.numStep = 0
	};
	iterativeDeepening(boardTurn);
	memcpy(&command, &(firstRoute.step[0]), sizeof(Step));
	memset(&firstRoute, 0, sizeof(firstRoute));
	timeUp = FALSE;
	return command;
}

void start(int flag)
{
	memset(board, 0, sizeof(board));
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 8; j += 2)
		{
			board[i][j + (i + 1) % 2] = MY_FLAG;
		}
	}
	for (int i = 5; i < 8; i++)
	{
		for (int j = 0; j < 8; j += 2)
		{
			board[i][j + (i + 1) % 2] = ENEMY_FLAG;
		}
	}
	initAI(flag);
}

void turn()
{
	Step command = aiTurn((const char(*)[BOARD_SIZE])board, myFlag);
	place(command);
	rotateCommand(&command);
	printf("%d", command.numStep);
	for (int i = 0; i < command.numStep; i++)
	{
		printf(" %d,%d", command.x[i], command.y[i]);
	}
	printf("\n");
	fflush(stdout);
}

void end(int x)
{
	exit(0);
}

void loop()
{
	char tag[10] = { 0 };
	Step command =
	{
		.x = {0},
		.y = {0},
		.numStep = 0
	};
	int status;
	while (TRUE)
	{
		memset(tag, 0, sizeof(tag));
		scanf("%s", tag);
		if (strcmp(tag, START) == 0)
		{
			scanf("%d", &myFlag);
			start(myFlag);
			printf("OK\n");
			fflush(stdout);
		}
		else if (strcmp(tag, PLACE) == 0)
		{
			scanf("%d", &command.numStep);
			for (int i = 0; i < command.numStep; i++)
			{
				scanf("%d,%d", &command.x[i], &command.y[i]);
			}
			rotateCommand(&command);
			place(command);
		}
		else if (strcmp(tag, TURN) == 0)
		{
			turn();
		}
		else if (strcmp(tag, END) == 0)
		{
			scanf("%d", &status);
			end(status);
		}
	}
}

int main(int argc, char* argv[])
{
	loop();
	return 0;
}