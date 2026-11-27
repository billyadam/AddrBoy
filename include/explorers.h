#ifndef ADDRBOY_EXPLORERS_H
#define ADDRBOY_EXPLORERS_H
#include "model.h"
namespace addr
{
constexpr int qr_payload_capacity = 128;
int qr_choice_count(Network network);
const char* qr_choice_label(Network network, int choice);
// Choice zero is always the unchanged receiving address. Other choices are explorer URLs.
// Returns false for an invalid choice or a payload that does not fit.
bool qr_payload(Network network, int choice, const char* address, char output[qr_payload_capacity]);
}
#endif
