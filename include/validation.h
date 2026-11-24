#ifndef ADDRBOY_VALIDATION_H
#define ADDRBOY_VALIDATION_H
#include "model.h"
namespace addr
{
// A successful validation writes the canonical address. Failure leaves output untouched.
// Input and output may alias. Empty input is invalid (removal is a separate action).
const char* validate_address(Network network, const char* input, char output[address_capacity]);
bool valid_wallet(const Wallet& wallet);
// Label for a valid BTC address: "N. SegWit" (bc1q), "Taproot" (bc1p), "P2SH" (3...) or "Legacy" (1...).
const char* bitcoin_kind(const char* address);
}
#endif
