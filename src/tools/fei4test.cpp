#include <iostream>
#include "SpecController.h"
#include "TxCore.h"
#include "Fei4.h"

int main(void) {
    SpecController spec(0);
    TxCore tx(&spec);
    Fei4 fe(&tx, 0, 0);
    fe.sendConfig();
}
