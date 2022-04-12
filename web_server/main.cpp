#include "sim_server.h"

int main() {
    SimServer& sv = SimServer::GetInstance();
    sv.start(SIM_SERV_ADDR, SIM_SERV_PORT, 5);
}