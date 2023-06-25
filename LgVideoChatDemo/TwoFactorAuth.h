#pragma once

#define TFA_SUCCESS 0
#define TFA_FAIL_WRONG 1
#define TFA_FAIL_TIME_OUT 2
#define RANDOM_LENTH 9
#define RECEVER_LENTH 50
void SendTFA(char* receiver);
int ReadTFA(char* receiver, char* input_token);

